# Product Marketing Context

*Last updated: 2026-08-20 · Auto-drafted from repository evidence*

## Product Overview

**One-liner:** SimpleOS is a small Unix-like 32-bit x86 operating system that students and systems programmers can boot, inspect, and modify.

**What it does:** The C and assembly codebase boots from an ISO or directly in QEMU, enters user mode, schedules processes, manages virtual memory, exposes 23 syscalls, and runs an in-memory filesystem and shell. A browser demo runs the same built image through v86.

**Product category:** Operating-systems education and systems-programming reference project

**Product type:** MIT-licensed open-source learning project

**Business model:** Free open-source project. No commercial product, support plan, or production-use claim is represented.

## Target Audience

**Primary users:** Systems-programming students, self-directed learners, educators, and developers exploring x86 kernels.

**Primary use case:** Boot a working small OS, trace each subsystem from entry point to userspace, then change and test it locally.

**Jobs to be done:**

- Learn how boot, interrupts, processes, paging, syscalls, filesystems, and userspace connect.
- Experiment with kernel code in a bounded codebase.
- Demonstrate a real shell, pipes, signals, jobs, and executable loading.
- Run a browser demonstration without installing a local emulator toolchain.

## Problems and Alternatives

**Core problem:** Operating-system concepts are easier to understand in a complete, traceable implementation than in isolated snippets or a production kernel with a very large codebase.

**Alternatives:** xv6, educational kernels, OS development tutorials, emulators, and production kernels may better fit formal curricula or deeper study. SimpleOS emphasizes a compact implementation and immediate browser trial.

## Differentiation

- A direct browser demo and real ISO expose the same kernel and userspace.
- `kernel_main()` provides a readable map of subsystem initialization.
- The shell exercises processes, pipes, redirection, signals, job control, and ELF loading.
- Serial-driven smoke and self-test kernels make core behavior repeatable in CI.
- An instruction-set guard prevents compiler-generated SSE use before the kernel enables it.

## Objections and Fit

| Question | Answer |
| --- | --- |
| Is this a production operating system? | No. It is a proof of concept and learning project. |
| Does the browser demo emulate every x86 environment? | No. It uses a pinned v86 runtime and specific assets. |
| Is every subsystem complete or hardened? | No. The repository documents bounded implementations and fixed limits. |

**Anti-persona:** Anyone seeking a secure general-purpose OS, modern hardware support, production workloads, or POSIX completeness.

## Customer Language

No learner interviews or testimonials are recorded. Use: boot it, read the code, trace the subsystem, from scratch, learning project, real ISO, browser demo, and self-test. Avoid: production OS, secure, POSIX-compliant, complete Unix, and hardware-ready.

## Brand Voice

**Tone:** Curious, concrete, technical, and welcoming to learners

**Style:** Let readers run something first, then guide them from observable behavior to the relevant source files.

## Proof Points

- 32-bit x86 kernel in C and assembly with 23 syscalls.
- Processes, preemptive scheduling, virtual memory, copy-on-write, signals, pipes, ramfs, ELF loading, and a user-mode shell.
- Browser demo plus QEMU and ISO boot paths.
- Headless smoke test, instruction-set check, static verifier, and a 10-case self-test kernel.

These are implementation and test facts, not claims of production safety, completeness, or teaching outcomes.

## Goals

**Primary goal:** Help learners reach a working OS quickly and connect visible behavior to understandable source code.

**Conversion action:** Boot the browser demo, run `help` and one pipeline, then start reading at `kernel_main()`.

## Messaging Guardrails

- Lead with learning and inspection, not production use.
- Keep code-size and feature counts tied to the current repository state.
- Do not equate passing self-tests with security or standards conformance.
- Distinguish SimpleOS code from third-party emulator, BIOS, and build assets.
