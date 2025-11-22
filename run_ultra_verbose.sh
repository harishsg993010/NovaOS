#!/bin/bash
# Run QEMU with MAXIMUM verbose output for deep debugging

echo "=== Running QEMU with ULTRA VERBOSE output ==="
echo "This will log CPU state, interrupts, exceptions, and more"
echo ""

qemu-system-x86_64 \
    -cdrom build/novae.iso \
    -m 512M \
    -serial stdio \
    -d int,cpu_reset,guest_errors,exec,in_asm \
    -D qemu_full_debug.log \
    -no-reboot \
    -no-shutdown

echo ""
echo "=== QEMU exited. Check qemu_full_debug.log for details ==="
echo "Warning: This log file will be VERY large!"
