# SimpleOS

<!-- project-record: simpleos -->

**Complete open-source learning project · MIT License**

A Unix-like 32-bit x86 operating system built from scratch in C and assembly
for systems programmers and students. It boots from a real ISO in QEMU or in a
browser through v86.

## Boot SimpleOS

Build the ISO and boot it in QEMU with `make run`, or run the [browser demo](#browser-demo)
locally. See [Building](#building) for the toolchain.

SimpleOS starts at the shell prompt. Run `help`, `ps`, `ls`, or a pipeline such as
`echo hello | wc` to exercise the same kernel and userspace built by this repository.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## Why use SimpleOS

Read a complete, small kernel end to end. SimpleOS implements processes, virtual
memory, syscalls, pipes, signals, a filesystem, and a shell in roughly 8,000 lines
of code. It boots as a real ISO image via GRUB, and the optional [web app](web/)
runs it in the browser with the [v86](https://github.com/copy/v86) x86 emulator.

SimpleOS is not a production operating system. It exists to show how an operating
system works by building one from the ground up.

## What's inside

| Subsystem | Key files | What it does |
|-----------|-----------|--------------|
| **Boot** | [boot.s](src/arch/i386/boot.s), [grub.cfg](boot/grub/grub.cfg), [linker.ld](linker.ld) | Multiboot2 entry, GDT, stack setup, jumps to `kernel_main` |
| **Kernel core** | [kernel.c](src/kernel/kernel.c) | Initializes every subsystem, creates test processes, starts scheduler |
| **Processes** | [process.c](src/kernel/process.c), [scheduler.c](src/kernel/scheduler.c) | Round-robin preemptive scheduling (10-tick quantum), kernel threads and user processes, up to 64 |
| **Context switching** | [context_switch.s](src/arch/i386/context_switch.s) | Saves/restores register state, interrupt-return trampoline for user mode |
| **Memory** | [pmm.c](src/mm/pmm.c), [vmm.c](src/mm/vmm.c), [kmalloc.c](src/mm/kmalloc.c) | Bitmap physical allocator, two-level paging with copy-on-write, free-list heap with block coalescing |
| **Syscalls** | [syscall.c](src/kernel/syscall.c) | 23 syscalls via `int 0x80` — fork, execve, wait, pipe, dup2, kill, signal, chdir, and more |
| **IPC** | [pipe.c](src/ipc/pipe.c), [signal.c](src/ipc/signal.c) | 4 KB blocking circular pipes, Unix-style signals (SIGINT, SIGKILL, SIGSTOP, etc.) |
| **Filesystem** | [fs.c](src/fs/fs.c) | In-memory ramfs — 64 nodes, 512KB of 512-byte blocks, pre-populated with `/bin/*` |
| **Drivers** | [terminal.c](src/drivers/terminal.c), [keyboard.c](src/drivers/keyboard.c), [timer.c](src/drivers/timer.c), [vt.c](src/drivers/vt.c) | VGA text mode, PS/2 keyboard with Ctrl+C/Z, PIT timer, virtual terminal switching (Alt+F1–F4) |
| **ELF loader** | [elf.c](src/lib/elf.c) | Loads 32-bit ELF binaries into user address space |
| **User mode** | [usermode.c](src/arch/i386/usermode.c), [tss.c](src/arch/i386/tss.c) | Ring 0 → Ring 3 via IRET, TSS for kernel stack on interrupts |
| **Userspace** | [shell.c](userspace/shell.c), [grep.c](userspace/grep.c), [wc.c](userspace/wc.c), [hello.c](userspace/hello.c) | Freestanding programs built as ELF, converted to C byte arrays, embedded in the kernel image |
| **Web demo** | [Emulator.tsx](web/components/Emulator.tsx), [CRTFrame.tsx](web/components/CRTFrame.tsx) | v86 emulator with asset preflight, CRT-styled terminal with animated ASCII background |

## Boot flow

```
GRUB (Multiboot2)
  → _start             src/arch/i386/boot.s      load GDT, set up stack
  → kernel_main        src/kernel/kernel.c        init all subsystems
  → scheduler_enable   src/kernel/scheduler.c     start round-robin loop
  → idle process                                  PID 0 runs when nothing else is ready
```

The kernel is linked at 1MB ([linker.ld](linker.ld)). The heap lives at 2–3.5MB.
User processes get virtual addresses starting at 128MB, with stacks below 3GB.

## Memory layout

```
0x00100000  (1MB)    Kernel code (.multiboot, .text, .data, .bss)
0x00200000  (2MB)    Kernel heap start
0x00380000  (3.5MB)  Kernel heap end
0x00400000  (4MB)    Physical memory allocator starts here
0x04000000  (64MB)   End of kernel identity-mapped window
0x08000000  (128MB)  User virtual address space begins
0xBFFFF000  (~3GB)   User stack top (grows down)
0xC0000000  (3GB)    Kernel boundary
```

## Syscalls

All syscalls use `int 0x80` with arguments in `eax` (number), `ebx`, `ecx`, `edx`.
Userspace wrappers live in [ulib.h](userspace/ulib.h).

| # | Name | Description |
|---|------|-------------|
| 1 | `exit` | Terminate process |
| 2 | `write` | Write to fd (terminal, file, or pipe) |
| 3 | `read` | Read from fd |
| 4 | `getpid` | Get current PID |
| 5 | `sleep` | Sleep N milliseconds |
| 6 | `sbrk` | Grow user heap |
| 7 | `fork` | Fork process (COW page tables) |
| 8 | `wait` | Wait for child, reap zombie |
| 9 | `execve` | Load and run ELF binary |
| 10 | `ps` | List all processes |
| 11 | `open` | Open file |
| 12 | `close` | Close fd |
| 13 | `stat` | File metadata |
| 14 | `mkdir` | Create directory |
| 15 | `readdir` | Read directory entries |
| 16 | `kill` | Send signal to process |
| 17 | `pipe` | Create pipe pair |
| 18 | `dup2` | Duplicate fd |
| 19 | `signal` | Install a user-mode signal handler |
| 20 | `sigreturn` | Return from a signal handler (used by the trampoline) |
| 21 | `setfg` | Set the terminal foreground job (Ctrl+C/Z target) |
| 22 | `chdir` | Change working directory |
| 23 | `getcwd` | Get working directory |

## Shell

The [shell](userspace/shell.c) supports:

- **Built-ins**: `help`, `ps`, `echo`, `cd`, `pwd`, `ls`, `cat`, `clear`, `history`, `jobs`, `fg`, `bg`, `kill`, `exit`
- **External programs**: `/bin/hello`, `/bin/grep`, `/bin/wc` (via fork + execve)
- **Pipes**: `echo hello | wc` — multi-stage too (`cmd1 | cmd2 | cmd3`)
- **Redirection**: `cmd > file`, `cmd >> file` (append), `cmd < file`
- **Background jobs**: `cmd &`, then `jobs`, `fg`, `bg`
- **Job control**: Ctrl+C (SIGINT), Ctrl+Z (SIGTSTP) routed to the foreground job
- **History**: up/down arrows (10 entries)

## Repository layout

```
src/
  arch/i386/          boot.s, asm_functions.s, context_switch.s, tss.c, usermode.c
  boot/               exceptions.c (ISR/IDT setup)
  kernel/             kernel.c, process.c, scheduler.c, syscall.c, panic.c
  mm/                 kmalloc.c, pmm.c, vmm.c
  drivers/            terminal.c, keyboard.c, timer.c, ports.c, vt.c, serial.c
  fs/                 fs.c
  ipc/                pipe.c, signal.c
  lib/                elf.c, string.c
include/              headers mirroring src/ layout
userspace/            shell.c, grep.c, wc.c, hello.c, selftest.c, ulib.h, Makefile
boot/grub/            grub.cfg
web/                  Next.js app (Emulator.tsx, CRTFrame.tsx, page.tsx)
scripts/              dev-shell.sh, setup-toolchain.sh, smoke-test.sh, selftest.sh
.devcontainer/        Dockerfile + devcontainer.json
```

## Building

The kernel targets 32-bit freestanding x86. The [Makefile](Makefile) cross-compiles
with `i686-elf-gcc` and produces `simpleos.iso` via `grub-mkrescue`.

Userspace programs are compiled separately ([userspace/Makefile](userspace/Makefile)),
then converted to C byte arrays with `xxd -i` and `#include`d into the kernel's
[filesystem init](src/fs/fs.c).

### Dev container (recommended)

Open the repo in VS Code, Cursor, or Zed — the [devcontainer config](.devcontainer/devcontainer.json)
has everything pre-installed.

```bash
make
cd web && npm install && npm run dev
```

### Local toolchain

See [scripts/setup-toolchain.sh](scripts/setup-toolchain.sh) for install
instructions. You need:

- `i686-elf-gcc` / `i686-elf-as`
- `grub-mkrescue` and `xorriso`
- `qemu-system-i386` (for local testing)

```bash
make          # build simpleos.iso
make run      # boot in QEMU
```

To build and boot just the kernel (no ISO/GRUB needed), use the Multiboot1 path:

```bash
make kernel.bin
qemu-system-i386 -kernel kernel.bin
```

### Docker build

```bash
./build.sh    # builds simpleos.iso → web/public/os/
```

The [Dockerfile](Dockerfile) extends a cross-compiler image with GRUB and xorriso.
You can also get an interactive shell with [scripts/dev-shell.sh](scripts/dev-shell.sh).

## Browser demo

The [web app](web/) boots the ISO in a [v86](https://github.com/copy/v86)
emulator with a CRT-styled UI. It expects three assets:

- `web/public/os/simpleos.iso` — built by `make` or `./build.sh`
- `web/public/bios/seabios.bin` — SeaBIOS ROM
- `web/public/bios/vgabios.bin` — VGA BIOS ROM

It also expects `web/public/v86/libv86.js` and `web/public/v86/v86.wasm` from
an official [v86 release](https://github.com/copy/v86/releases). These
third-party build artifacts and BIOS files are intentionally not checked in.

If anything is missing, the [preflight check](web/components/Emulator.tsx) shows
exactly what's absent instead of booting into a broken state.

```bash
cd web
bun install
bun run dev         # starts on port 3500
bun run build:os    # rebuild kernel and copy ISO in one step
```

For a production build, prepare the pinned browser runtime after building the
ISO, then build from the `web/` directory:

```bash
./build.sh
bun run prepare:web-assets
cd web
vercel build --prod
vercel deploy --prebuilt --prod
```

## Reading the code

**Start here:** [src/kernel/kernel.c](src/kernel/kernel.c) — `kernel_main()`
initializes every subsystem in order and is the best map of how the pieces
connect.

From there:

1. **Boot**: [boot.s](src/arch/i386/boot.s) → how the CPU gets from GRUB to `kernel_main`
2. **Interrupts**: [asm_functions.s](src/arch/i386/asm_functions.s) → ISR stubs, IDT/GDT loading, paging enable
3. **Processes**: [process.c](src/kernel/process.c) → PCB structure, creation, destruction
4. **Scheduling**: [scheduler.c](src/kernel/scheduler.c) → preemptive round-robin, context switch calls
5. **Memory**: [vmm.c](src/mm/vmm.c) → page tables, COW fork, address space management
6. **Syscalls**: [syscall.c](src/kernel/syscall.c) → `int 0x80` dispatch, all 23 implementations
7. **Userspace**: [ulib.h](userspace/ulib.h) → syscall wrappers that user programs call
8. **Shell**: [shell.c](userspace/shell.c) → pipes, redirection, job control in ~700 lines

## Verification

The kernel can be exercised headlessly — COM1 serial mirrors all terminal output,
and the kernel boots directly via its Multiboot1 header with `qemu -kernel` (no
ISO/GRUB needed). Two scripts drive this:

- [scripts/smoke-test.sh](scripts/smoke-test.sh) — build, boot, and assert the
  kernel reaches full init without panicking.
- [scripts/selftest.sh](scripts/selftest.sh) — build a self-test kernel that runs
  [userspace/selftest.c](userspace/selftest.c) as init and verifies fork/wait,
  pipes (including blocking transfers larger than the buffer), signals (default
  actions and user-mode handlers), `sbrk`, BSS zeroing, `O_APPEND`, working-directory
  resolution, and the terminal's ANSI escape parser.

A separate [verify_os.py](verify_os.py) checks code integrity without compiling — include
dependencies, function signatures, syscall coverage, and common issues.

## Contributing

Read [CONTRIBUTING.md](CONTRIBUTING.md) for the code style, generated-artifact boundary,
and required web and kernel checks.

## Raintree open-source system

SimpleOS is an independent systems-learning project. Raintree also maintains
[DocPull](https://github.com/raintree-technology/docpull) for context acquisition,
[HIG Doctor](https://github.com/raintree-technology/hig-doctor) for interface audits,
[PolicyStrata](https://github.com/raintree-technology/policystrata) for policy regression
testing, [Trellis](https://github.com/raintree-technology/trellis) for shared code policy,
and [Raintree Standards](https://github.com/raintree-technology/raintree.standards) for
governed requirements. See the
[Raintree open-source portfolio](https://raintree.technology/portfolio#open-source).

Raintree also builds [BetterOff](https://betteroff.finance), which keeps a household's
spending, investments, assets, debts, and net worth in one current record.

## Project policies

[Contributing](CONTRIBUTING.md) · [Code of Conduct](CODE_OF_CONDUCT.md) · [Security](SECURITY.md) ·
[Changelog](CHANGELOG.md) ·
[Source repository](https://github.com/raintree-technology/SimpleOS) · [MIT License](LICENSE)

Third-party tools and runtime assets used to build or run SimpleOS keep their own
licenses.
