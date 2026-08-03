#!/usr/bin/env bash
# Headless boot smoke test: build kernel.bin, boot it in QEMU via the Multiboot1
# header (no GRUB/ISO needed), capture COM1 serial, and assert the kernel reaches
# full init without panicking. Usage: scripts/smoke-test.sh [seconds]
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

SECS="${1:-6}"
QEMU="${QEMU:-qemu-system-i386}"
LOG="$(mktemp -t sos_serial.XXXXXX)"

make kernel.bin >/dev/null

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
if ! grep -q "All init complete" "$LOG"; then
    echo "FAIL: kernel did not finish init"
    rc=1
fi
if grep -qiE "PANIC|Unhandled (page fault|exception)|GENERAL PROTECTION" "$LOG"; then
    echo "FAIL: kernel panicked or faulted during boot"
    rc=1
fi
[ "$rc" -eq 0 ] && echo "PASS: clean boot to full init"
rm -f "$LOG"
exit "$rc"
