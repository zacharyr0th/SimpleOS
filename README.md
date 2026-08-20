# SimpleOS

A Unix-like 32-bit x86 operating system built from scratch in C and assembly
that boots from a real ISO and runs in your browser through v86.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![GitHub stars](https://img.shields.io/github/stars/raintree-technology/SimpleOS?style=social)](https://github.com/raintree-technology/SimpleOS/stargazers)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](#contributing)

## Star History

<a href="https://star-history.com/#raintree-technology/SimpleOS&Date">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/svg?repos=raintree-technology/SimpleOS&type=Date&theme=dark" />
    <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/svg?repos=raintree-technology/SimpleOS&type=Date" />
    <img alt="Star history chart for raintree-technology/SimpleOS" src="https://api.star-history.com/svg?repos=raintree-technology/SimpleOS&type=Date" />
  </picture>
</a>

SimpleOS is a learning project — a from-scratch kernel that implements
processes, virtual memory, syscalls, pipes, signals, a filesystem, and a shell,
all in roughly 8,000 lines of code. It boots as a real ISO image via GRUB, and
an optional [web app](web/) can run the OS in-browser using the
[v86](https://github.com/copy/v86) x86 emulator.

This isn't a production operating system. It's a proof of concept and a learning
tool — built to understand how operating systems actually work by writing one
from the ground up.

## Table of Contents

- [What's Inside](#whats-inside)
- [Boot Flow](#boot-flow)
- [Memory Layout](#memory-layout)
- [Syscalls](#syscalls)
- [Shell](#shell)
- [Repository Layout](#repository-layout)
- [Building](#building)
- [Browser Demo](#browser-demo)
- [Reading the Code](#reading-the-code)
- [Verification](#verification)
- [Contributing](#contributing)
- [License](#license)

## What's Inside

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

## Boot Flow

```
GRUB (Multiboot2)
  → _start             src/arch/i386/boot.s      load GDT, set up stack
  → kernel_main        src/kernel/kernel.c        init all subsystems
  → scheduler_enable   src/kernel/scheduler.c     start round-robin loop
  → idle process                                  PID 0 runs when nothing else is ready
```

The kernel is linked at 1MB ([linker.ld](linker.ld)). The heap lives at 2–3.5MB.
User processes get virtual addresses starting at 128MB, with stacks below 3GB.

## Memory Layout

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

## Repository Layout

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

### Dev Container (recommended)

Open the repo in VS Code, Cursor, or Zed — the [devcontainer config](.devcontainer/devcontainer.json)
has everything pre-installed.

```bash
make
cd web && npm install && npm run dev
```

### Local Toolchain

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

### Docker Build

```bash
./build.sh    # builds simpleos.iso → web/public/os/
```

The [Dockerfile](Dockerfile) extends a cross-compiler image with GRUB and xorriso.
You can also get an interactive shell with [scripts/dev-shell.sh](scripts/dev-shell.sh).

## Browser Demo

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
npm install
npm run dev         # starts on port 3500
npm run build:os    # rebuild kernel and copy ISO in one step
```

## Reading the Code

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

Contributions are welcome!

1. Fork the repo and create a branch: `git checkout -b feat/your-feature`
2. Make your changes and ensure the build passes: `make kernel.bin`
3. Run the checks: `scripts/smoke-test.sh` and `scripts/selftest.sh` (and `python3 verify_os.py`)
4. Open a pull request with a clear description of what changed and why

Please follow the existing code style (K&R-adjacent C, descriptive names, comments
on non-obvious logic). For larger changes, open an issue first to discuss the approach.

## License

SimpleOS source is [MIT](LICENSE) licensed. Third-party tools and runtime
assets used to build or run the project keep their own licenses.
