# SimpleOS agent instructions

SimpleOS is a teaching operating system for 32-bit x86 with a browser demo.

- Preserve the freestanding C and assembly boundary; do not introduce host-runtime
  assumptions into kernel or userspace code.
- Keep the real ISO, direct Multiboot kernel, and browser demo compatible.
- Add or update headless checks when kernel, syscall, filesystem, process, or signal
  behavior changes.
- Keep downloaded browser runtime and BIOS assets pinned and checksum-verified.
- Do not commit generated objects, binaries, ISO images, browser runtime assets, or
  local environment files.
- Use Bun 1.3.11 and Node 24 for the web app.
- Run `bun run validate` for web or static changes. Run `bun run validate:full` when
  the cross-toolchain and QEMU are available for kernel or release-facing changes.
