// Userspace self-test: exercises fork/wait, pipes across fork, and signals.
// Built into a SELFTEST kernel and run as init so the critical process and IPC
// paths can be verified headlessly (output is mirrored to COM1). Each check
// prints "SELFTEST: <name> PASS|FAIL"; a final "SELFTEST: COMPLETE p/n" line
// lets a harness assert the whole run reached the end.
#include "ulib.h"

#define SIGKILL 9
#define SIGTERM 15

static int tests_run = 0;
static int tests_passed = 0;

static void report(const char* name, int ok) {
    tests_run++;
    if (ok) tests_passed++;
    WRITE_STR(1, "SELFTEST: ");
    WRITE_STR(1, name);
    WRITE_STR(1, ok ? " PASS\n" : " FAIL\n");
}

// fork a child that exits with a known code; wait must reap it and surface
// the status.
static void test_fork_wait(void) {
    int pid = sys_fork();
    if (pid == 0) {
        sys_exit(42);
    }
    int status = -1;
    int r = sys_wait(&status);
    report("fork_wait", r == pid && status == 42);
}

// A pipe shared across fork must survive each side closing the end it does not
// use (exercises pipe + fd refcounting). Child writes, parent reads.
static void test_pipe(void) {
    int fds[2];
    if (sys_pipe(fds) < 0) {
        report("pipe", 0);
        return;
    }
    int pid = sys_fork();
    if (pid == 0) {
        sys_close(fds[0]);
        sys_write(fds[1], "ping", 4);
        sys_close(fds[1]);
        sys_exit(0);
    }
    sys_close(fds[1]);
    char buf[5] = {0, 0, 0, 0, 0};
    int n = sys_read(fds[0], buf, 4);
    sys_close(fds[0]);
    sys_wait(0);
    int ok = (n == 4) &&
             buf[0] == 'p' && buf[1] == 'i' && buf[2] == 'n' && buf[3] == 'g';
    report("pipe", ok);
}

// A large global lands in BSS, which the ELF loader must zero across several
// pages starting at a non-page-aligned offset. Regression guard for a loader
// bug that double-applied the in-page offset and corrupted page tables. Verify
// the region is zero-initialized and fully writable.
static char bss_region[9000];

static void test_bss(void) {
    int zeroed = 1;
    for (int i = 0; i < (int)sizeof(bss_region); i++) {
        if (bss_region[i] != 0) {
            zeroed = 0;
            break;
        }
    }
    for (int i = 0; i < (int)sizeof(bss_region); i++) {
        bss_region[i] = (char)((i * 7 + 1) & 0x7f);
    }
    int wrote = 1;
    for (int i = 0; i < (int)sizeof(bss_region); i++) {
        if (bss_region[i] != (char)((i * 7 + 1) & 0x7f)) {
            wrote = 0;
            break;
        }
    }
    report("bss_zeroed_writable", zeroed && wrote);
}

// Grow the heap with sbrk across a page boundary and verify the whole new
// region is usable. Exercises the on-demand page allocation in sys_sbrk.
static void test_sbrk(void) {
    char* base = (char*)sys_sbrk(0);
    char* p = (char*)sys_sbrk(8192);
    int ok = (p == base) && (p != (char*)0xFFFFFFFFu);
    if (ok) {
        for (int i = 0; i < 8192; i++) p[i] = (char)((i + 3) & 0x7f);
        for (int i = 0; i < 8192; i++) {
            if (p[i] != (char)((i + 3) & 0x7f)) {
                ok = 0;
                break;
            }
        }
    }
    // New break should sit 8192 bytes past the old one.
    ok = ok && ((char*)sys_sbrk(0) == base + 8192);
    report("sbrk_grow", ok);
}

// Push more bytes than the pipe buffer holds (PIPE_SIZE=4096) so the writer
// must block on a full pipe and be woken as the reader drains. Verifies the
// bidirectional blocking handshake never deadlocks and preserves byte order.
// Uses small stack buffers (no large BSS) and regenerates the pattern.
#define BIG 8000

static void test_pipe_large(void) {
    int fds[2];
    if (sys_pipe(fds) < 0) {
        report("pipe_large", 0);
        return;
    }
    int pid = sys_fork();
    if (pid == 0) {
        sys_close(fds[0]);
        char chunk[200];
        int written = 0;
        while (written < BIG) {
            int m = BIG - written;
            if (m > (int)sizeof(chunk)) m = sizeof(chunk);
            for (int i = 0; i < m; i++) chunk[i] = (char)((written + i) & 0x7f);
            int w = sys_write(fds[1], chunk, m);
            if (w <= 0) break;
            written += w;
        }
        sys_close(fds[1]);
        sys_exit(0);
    }
    sys_close(fds[1]);
    sys_sleep(30);  // let the child fill the pipe and block on a full buffer
    int total = 0, ok = 1;
    char rbuf[256];
    while (total < BIG) {
        int n = sys_read(fds[0], rbuf, sizeof(rbuf));
        if (n <= 0) break;
        for (int i = 0; i < n; i++) {
            if (rbuf[i] != (char)((total + i) & 0x7f)) {
                ok = 0;
                break;
            }
        }
        total += n;
    }
    sys_close(fds[0]);
    sys_wait(0);
    report("pipe_large", ok && total == BIG);
}

// Kill a looping child with SIGKILL; wait must reap the terminated child
// rather than hanging forever.
static void test_signal(void) {
    int pid = sys_fork();
    if (pid == 0) {
        for (;;) sys_sleep(100);
    }
    sys_sleep(50);  // let the child start looping
    sys_kill(pid, SIGKILL);
    int status = 0;
    int r = sys_wait(&status);
    report("signal_kill", r == pid);
}

// Write a file, reopen it with O_APPEND, append more, and read it back to
// confirm the second write was appended (not truncated). Exercises O_APPEND.
static void test_append(void) {
    int ok = 1;
    int fd = sys_open("/atest", O_CREAT, 0);
    if (fd < 0) { report("append", 0); return; }
    sys_write(fd, "AAA", 3);
    sys_close(fd);

    fd = sys_open("/atest", O_CREAT | O_APPEND, 0);
    if (fd < 0) { report("append", 0); return; }
    sys_write(fd, "BB", 2);
    sys_close(fd);

    fd = sys_open("/atest", 0, 0);
    char buf[8];
    int n = (fd >= 0) ? sys_read(fd, buf, 8) : -1;
    if (fd >= 0) sys_close(fd);
    ok = (n == 5) && buf[0]=='A' && buf[1]=='A' && buf[2]=='A' &&
         buf[3]=='B' && buf[4]=='B';
    report("append", ok);
}

// Exercise the working-directory machinery: initial cwd, chdir (absolute and
// ".."), getcwd, and relative-path resolution in open(). /bin/hello exists in
// the ramfs, so opening "hello" from /bin must succeed.
static void test_cd(void) {
    char buf[128];
    int ok = 1;

    sys_getcwd(buf, sizeof(buf));
    ok = ok && buf[0] == '/' && buf[1] == '\0';

    ok = ok && (sys_chdir("/bin") == 0);
    sys_getcwd(buf, sizeof(buf));
    ok = ok && buf[0]=='/' && buf[1]=='b' && buf[2]=='i' && buf[3]=='n' && buf[4]=='\0';

    int fd = sys_open("hello", 0, 0);   // relative -> /bin/hello
    ok = ok && (fd >= 0);
    if (fd >= 0) sys_close(fd);

    ok = ok && (sys_chdir("..") == 0);  // back to root
    sys_getcwd(buf, sizeof(buf));
    ok = ok && buf[0] == '/' && buf[1] == '\0';

    ok = ok && (sys_chdir("/nope") < 0);  // nonexistent dir fails

    report("cd_cwd", ok);
}

// Install a custom signal handler, raise the signal to ourselves, and confirm
// the handler ran in user mode and execution resumed afterwards. Exercises the
// full user-mode trampoline + sigreturn round trip.
static volatile int sig_caught = 0;
static void catch_handler(int s) {
    sig_caught = (s == SIGTERM) ? 99 : -1;
}

static void test_signal_handler(void) {
    sig_caught = 0;
    sys_signal(SIGTERM, catch_handler);
    sys_kill(sys_getpid(), SIGTERM);
    // By the time kill() returns to us, the handler should have run and
    // sigreturn should have resumed this code path.
    report("signal_handler", sig_caught == 99);
}

// A child that dereferences a NULL pointer must be killed by the page-fault
// handler (not panic the kernel); wait must reap it with status 128+SIGSEGV.
static void test_segfault(void) {
    int pid = sys_fork();
    if (pid == 0) {
        *(volatile int*)0 = 1;  // write to unmapped page -> page fault
        sys_exit(0);            // unreachable
    }
    int status = 0;
    int r = sys_wait(&status);
    report("segfault_kill", r == pid && status == 139);
}

void selftest_main(void) {
    WRITE_STR(1, "SELFTEST: BEGIN\n");
    test_bss();
    test_sbrk();
    test_append();
    test_cd();
    test_fork_wait();
    test_pipe();
    test_pipe_large();
    test_signal();
    test_signal_handler();
    test_segfault();

    char buf[12];
    WRITE_STR(1, "SELFTEST: COMPLETE ");
    int_to_str(tests_passed, buf);
    WRITE_STR(1, buf);
    WRITE_STR(1, "/");
    int_to_str(tests_run, buf);
    WRITE_STR(1, buf);
    WRITE_STR(1, "\n");
    sys_exit(0);
}

__asm__(
    ".globl _start\n"
    "_start:\n"
    "    call selftest_main\n"
    "    movl $1, %eax\n"
    "    xorl %ebx, %ebx\n"
    "    int $0x80\n"
);
