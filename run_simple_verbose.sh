#!/bin/bash
# Run QEMU with simple verbose output (easier to read)

echo "=== Running QEMU with simple verbose output ==="
echo ""

qemu-system-x86_64 \
    -cdrom build/novae.iso \
    -m 512M \
    -serial stdio \
    -d cpu_reset \
    -D qemu_simple.log \
    -no-reboot

echo ""
echo "=== Check output above and qemu_simple.log ==="
