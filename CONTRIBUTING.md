# Contributing to SimpleOS

SimpleOS is a learning project. Keep changes small enough that readers can follow the
boot, kernel, userspace, and browser boundaries.

## Before proposing a change

1. Explain the behavior or teaching outcome the change improves.
2. Follow the existing K&R-adjacent C style and comment only non-obvious decisions.
3. Add or update a headless check for behavior changes.
4. Do not commit generated binaries, ISO images, or downloaded browser assets.

Run the repository checks:

```bash
bun install --frozen-lockfile
bun run validate
```

For kernel changes, also run the QEMU checks with the cross-toolchain installed:

```bash
bun run test:kernel
```

For larger architecture changes, open an issue before implementation.
