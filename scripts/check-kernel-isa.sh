#!/usr/bin/env bash

set -euo pipefail

kernel_file="${1:-kernel.bin}"
objdump_bin="${OBJDUMP:-${CROSS_PREFIX:-i686-elf-}objdump}"

if [[ ! -f "$kernel_file" ]]; then
    echo "Missing $kernel_file. Build the kernel first."
    exit 1
fi

if ! command -v "$objdump_bin" >/dev/null 2>&1; then
    echo "Missing objdump tool: $objdump_bin"
    exit 1
fi

if "$objdump_bin" -d "$kernel_file" | grep -Eq '%(xmm|ymm|zmm|mm[0-7]|st\()'; then
    echo "Kernel contains FPU, MMX, or SIMD instructions before those CPU features are enabled."
    exit 1
fi

echo "Kernel instruction set is safe for early boot."
