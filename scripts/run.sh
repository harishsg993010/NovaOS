#!/bin/bash
# Run NovaeOS in QEMU

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}Starting NovaeOS in QEMU...${NC}"

# Check if ISO exists
if [ ! -f "build/novae.iso" ]; then
    echo -e "${RED}Error: build/novae.iso not found${NC}"
    echo "Please run 'make iso' first"
    exit 1
fi

# Default QEMU options
QEMU_CMD="qemu-system-x86_64"
QEMU_OPTS="-cdrom build/novae.iso -m 512M -serial stdio"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--debug)
            echo -e "${GREEN}Debug mode enabled${NC}"
            echo -e "${GREEN}Connect with: gdb build/kernel.elf -ex 'target remote localhost:1234'${NC}"
            QEMU_OPTS="$QEMU_OPTS -s -S"
            shift
            ;;
        -n|--network)
            echo -e "${GREEN}Network enabled${NC}"
            QEMU_OPTS="$QEMU_OPTS -netdev user,id=net0 -device e1000,netdev=net0"
            shift
            ;;
        -g|--graphics)
            echo -e "${GREEN}Graphics mode enabled${NC}"
            QEMU_OPTS="$QEMU_OPTS -vga std -display sdl"
            shift
            ;;
        --disk)
            if [ ! -f "disk.img" ]; then
                echo -e "${BLUE}Creating disk image...${NC}"
                qemu-img create -f raw disk.img 100M
            fi
            echo -e "${GREEN}Disk image attached${NC}"
            QEMU_OPTS="$QEMU_OPTS -drive file=disk.img,format=raw,index=0,media=disk"
            shift
            ;;
        -m|--memory)
            MEM="$2"
            echo -e "${GREEN}Memory: ${MEM}${NC}"
            QEMU_OPTS=$(echo "$QEMU_OPTS" | sed "s/-m [0-9]*M/-m ${MEM}/")
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  -d, --debug      Enable GDB debugging (port 1234)"
            echo "  -n, --network    Enable networking"
            echo "  -g, --graphics   Enable graphics mode"
            echo "  --disk           Attach disk image"
            echo "  -m, --memory MB  Set memory size (default: 512M)"
            echo "  -h, --help       Show this help"
            echo ""
            echo "Examples:"
            echo "  $0                  # Basic run"
            echo "  $0 -d               # Debug mode"
            echo "  $0 -n --disk        # With network and disk"
            echo "  $0 -m 1G            # With 1GB RAM"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            echo "Use -h or --help for usage information"
            exit 1
            ;;
    esac
done

# Run QEMU
echo -e "${BLUE}Command: $QEMU_CMD $QEMU_OPTS${NC}"
echo ""
$QEMU_CMD $QEMU_OPTS
