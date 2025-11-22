#!/bin/bash
# Run NovaeOS with disk properly attached to IDE bus

# Make sure the ISO is built
if [ ! -f build/novae.iso ]; then
    echo "Building ISO first..."
    make iso
fi

# Run QEMU with disk attached to primary master IDE
qemu-system-x86_64 \
    -m 512M \
    -serial stdio \
    -cdrom build/novae.iso \
    -drive file=disk.img,format=raw,if=ide,index=0,media=disk
