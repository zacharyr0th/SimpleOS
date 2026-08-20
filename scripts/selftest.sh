#!/usr/bin/env bash
# Headless behavioral test: build a SELFTEST kernel (runs userspace selftest as
# init instead of the shell), boot it in QEMU via the Multiboot1 header, capture
# COM1 serial, and assert every self-test check passed. Exercises the fork/wait,
# pipe, and signal paths that cannot be driven through the PS/2-only shell.
# Usage: scripts/selftest.sh [seconds]
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

SECS="${1:-8}"
QEMU="${QEMU:-qemu-system-i386}"
LOG="$(mktemp -t sos_selftest.XXXXXX)"

if [[ "${SIMPLEOS_SKIP_BUILD:-0}" != "1" ]]; then
    # Regenerate the embedded selftest binary, then force a -DSELFTEST rebuild
    # (make does not track CFLAGS changes, so a clean is required when toggling).
    make -C userspace selftest_binary.h >/dev/null
    make clean >/dev/null
    make kernel.bin SELFTEST=1 >/dev/null
fi

if [[ ! -f kernel.bin ]]; then
    echo "FAIL: self-test kernel.bin is missing"
    exit 1
fi

"$QEMU" -kernel kernel.bin -m 512M -display none -serial "file:$LOG" \
    -no-reboot -no-shutdown >/dev/null 2>&1 &
QPID=$!
sleep "$SECS"
kill "$QPID" 2>/dev/null || true
wait "$QPID" 2>/dev/null || true

echo "===== serial output ====="
cat "$LOG"
echo "===== end serial ====="

rc=0
if grep -qE "SELFTEST: .* FAIL" "$LOG"; then
    echo "FAIL: one or more self-test checks failed"
    rc=1
fi
if ! grep -q "SELFTEST: COMPLETE" "$LOG"; then
    echo "FAIL: self-test did not run to completion (hang or crash)"
    rc=1
fi
if grep -qiE "PANIC|Unhandled (page fault|exception)|GENERAL PROTECTION" "$LOG"; then
    echo "FAIL: kernel panicked or faulted during the self-test"
    rc=1
fi
[ "$rc" -eq 0 ] && echo "PASS: all self-test checks passed"
rm -f "$LOG"
exit "$rc"
