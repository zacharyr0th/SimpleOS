#include "ipc/signal.h"
#include "kernel/process.h"
#include "drivers/terminal.h"
#include "kernel/scheduler.h"
#include "kernel/syscall.h"
#include "lib/string.h"

// External process table for lookup
extern process_t* process_table[];

// PID of the current foreground process (the job the terminal sends Ctrl+C /
// Ctrl+Z to). 0 means "no foreground job" (e.g. shell sitting at its prompt),
// in which case terminal-generated signals are dropped rather than delivered to
// an arbitrary process. Set by the shell via SYS_SETFG around each command.
static int foreground_pid = 0;

void signal_set_foreground(int pid) {
    foreground_pid = pid;
}

// Deliver a terminal-generated signal to the foreground job: the foreground
// process plus its direct children (a minimal process-group emulation, enough
// for "cmd" and "cmd args" plus any helpers it spawns). Never targets init.
void signal_foreground(int sig) {
    if (foreground_pid <= 1) {
        return;  // no foreground job, or it is init — drop the signal
    }
    signal_send(foreground_pid, sig);
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t* p = process_table[i];
        if (p && p->pid > 1 && (int)p->parent_pid == foreground_pid) {
            signal_send(p->pid, sig);
        }
    }
}

process_t* process_find_by_pid(uint32_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i] && process_table[i]->pid == pid) {
            return process_table[i];
        }
    }
    return NULL;
}

void signal_init_process(process_t* proc) {
    if (!proc) return;

    proc->pending_signals = 0;
    proc->signal_mask = 0;
    proc->in_signal_handler = 0;

    // Set all handlers to SIG_DFL
    for (int i = 0; i < NSIG; i++) {
        proc->signal_handlers[i] = SIG_DFL;
    }
}

void signal_init(void) {
    terminal_writestring("Signal subsystem initialized\n");
}

void signal_send(int pid, int sig) {
    if (sig < 1 || sig >= NSIG) {
        return;
    }

    process_t* proc = process_find_by_pid(pid);
    if (!proc) {
        return;
    }

    // Handle uncatchable signals immediately
    if (sig == SIGKILL) {
        process_make_zombie(proc, 128 + sig);
        return;
    }

    if (sig == SIGSTOP) {
        if (proc->state == PROCESS_STATE_RUNNING ||
            proc->state == PROCESS_STATE_READY) {
            proc->state = PROCESS_STATE_BLOCKED;
        }
        return;
    }

    if (sig == SIGCONT) {
        if (proc->state == PROCESS_STATE_BLOCKED) {
            proc->state = PROCESS_STATE_READY;
            extern void ready_queue_push(process_t* proc);
            ready_queue_push(proc);
        }
        // Clear any pending SIGSTOP/SIGTSTP
        proc->pending_signals &= ~((1 << SIGSTOP) | (1 << SIGTSTP));
        return;
    }

    // Set pending signal bit
    proc->pending_signals |= (1 << sig);

    // If process is blocked and not masked, wake it up
    if (proc->state == PROCESS_STATE_BLOCKED) {
        if (!(proc->signal_mask & (1 << sig))) {
            proc->state = PROCESS_STATE_READY;
            extern void ready_queue_push(process_t* proc);
            ready_queue_push(proc);
        }
    }
}

// Check if current process has pending signals
int signal_pending(void) {
    process_t* current = process_get_current();
    if (!current) return 0;

    // Check for unmasked pending signals
    uint32_t actionable = current->pending_signals & ~current->signal_mask;
    return actionable != 0;
}

// Handle pending signals for current process
void signal_handle(void) {
    process_t* current = process_get_current();
    if (!current) return;

    // Get actionable signals (pending and not masked)
    uint32_t actionable = current->pending_signals & ~current->signal_mask;
    if (actionable == 0) return;

    // Handle each pending signal. Default actions (terminate/stop/ignore) run
    // here in kernel context. Signals with a custom user handler are left
    // pending and delivered to user mode by signal_deliver_pending() on the
    // return-to-user path, where the interrupted register frame is available.
    for (int sig = 1; sig < NSIG; sig++) {
        if (!(actionable & (1 << sig))) continue;

        // Get handler
        sighandler_t handler = (sighandler_t)current->signal_handlers[sig];

        if (handler != SIG_DFL && handler != SIG_IGN) {
            // Custom handler: defer to user-mode delivery (keep it pending).
            continue;
        }

        // Clear the pending bit for the actions resolved here.
        current->pending_signals &= ~(1 << sig);

        if (handler == SIG_IGN) {
            // Ignored
            continue;
        }

        // Default action
        switch (sig) {
            case SIGINT:
            case SIGTERM:
                process_make_zombie(current, 128 + sig);
                schedule();
                return;

            case SIGTSTP:
                current->state = PROCESS_STATE_BLOCKED;
                schedule();
                return;

            case SIGCONT:
                // Already handled in signal_send
                break;

            default:
                // Most signals terminate by default
                process_make_zombie(current, 128 + sig);
                schedule();
                return;
        }
    }
}

// Deliver one pending custom-handler signal to user mode by building a signal
// frame on the user stack and redirecting the interrupted context to the
// handler. The handler returns into a small trampoline (placed on the user
// stack) that invokes SYS_SIGRETURN, which restores the saved context.
// Called on the return-to-user path (after a syscall or timer IRQ).
void signal_deliver_pending(registers_t* regs) {
    process_t* current = process_get_current();
    if (!current) return;

    // Only deliver when we are about to return to user mode (CPL 3), and only
    // one handler at a time (no nested delivery until sigreturn).
    if ((regs->cs & 0x3) != 0x3) return;
    if (current->in_signal_handler) return;

    uint32_t actionable = current->pending_signals & ~current->signal_mask;
    if (actionable == 0) return;

    for (int sig = 1; sig < NSIG; sig++) {
        if (!(actionable & (1 << sig))) continue;

        sighandler_t handler = (sighandler_t)current->signal_handlers[sig];
        if (handler == SIG_DFL || handler == SIG_IGN) {
            continue;  // resolved by signal_handle()
        }

        current->pending_signals &= ~(1 << sig);

        // Trampoline: mov $SYS_SIGRETURN, %eax ; int $0x80
        uint8_t tramp[7] = { 0xB8, (uint8_t)SYS_SIGRETURN, 0, 0, 0, 0xCD, 0x80 };
        uint32_t needed = sizeof(tramp) + 16;

        // Bail out (terminate) if the user stack can't hold the frame; writing
        // outside the mapped stack from ring 0 would fault the kernel.
        if (regs->esp <= current->stack_bottom + needed ||
            regs->esp > current->stack_top) {
            process_make_zombie(current, 128 + sig);
            schedule();
            return;
        }

        // Save the interrupted context for sigreturn.
        current->signal_saved_regs = *regs;
        current->in_signal_handler = 1;

        uint32_t usp = regs->esp;

        // Place the trampoline code on the stack (i386 has no NX without PAE).
        usp -= sizeof(tramp);
        usp &= ~0xFu;  // keep things aligned
        uint32_t tramp_addr = usp;
        memcpy((void*)tramp_addr, tramp, sizeof(tramp));

        // Build the handler call frame: [return = trampoline][sig].
        usp -= 4;
        *(uint32_t*)usp = (uint32_t)sig;
        usp -= 4;
        *(uint32_t*)usp = tramp_addr;

        regs->esp = usp;
        regs->eip = (uint32_t)handler;
        return;  // deliver one per return; the rest stay pending
    }
}

// Restore the context saved when a signal handler was entered (SYS_SIGRETURN).
void signal_return(registers_t* regs) {
    process_t* current = process_get_current();
    if (!current || !current->in_signal_handler) {
        return;
    }
    *regs = current->signal_saved_regs;
    current->in_signal_handler = 0;
}

// Set signal handler (sys_signal syscall)
sighandler_t sys_signal(int signum, sighandler_t handler) {
    if (signum < 1 || signum >= NSIG) {
        return SIG_DFL;
    }

    // SIGKILL and SIGSTOP cannot be caught
    if (signum == SIGKILL || signum == SIGSTOP) {
        return SIG_DFL;
    }

    process_t* current = process_get_current();
    if (!current) {
        return SIG_DFL;
    }

    sighandler_t old_handler = (sighandler_t)current->signal_handlers[signum];
    current->signal_handlers[signum] = (void*)handler;

    return old_handler;
}
