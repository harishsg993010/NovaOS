#!/bin/bash
# Run QEMU with verbose debugging output

echo "=== Running QEMU with verbose output ==="
echo ""

qemu-system-x86_64 \
    -cdrom build/novae.iso \
    -m 512M \
    -serial stdio \
    -d int,cpu_reset,guest_errors \
    -D qemu_debug.log \
    -no-reboot \
    -no-shutdown

echo ""
echo "=== QEMU exited. Check qemu_debug.log for details ==="
