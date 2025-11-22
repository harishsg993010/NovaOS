#!/bin/bash
# Debug boot issues by running QEMU with detailed logging

echo "Starting QEMU with debug output..."
echo "Watch for any error messages or triple faults"
echo ""

qemu-system-x86_64 \
    -cdrom build/novae.iso \
    -m 512M \
    -serial stdio \
    -d cpu_reset,guest_errors \
    -D qemu_debug.log \
    -no-reboot \
    -no-shutdown
