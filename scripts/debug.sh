#!/bin/bash
# Debug NovaeOS with GDB

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[0;33m'
NC='\033[0m'

echo -e "${BLUE}NovaeOS Debug Session${NC}"
echo ""

# Check if kernel.elf exists
if [ ! -f "build/kernel.elf" ]; then
    echo -e "${RED}Error: build/kernel.elf not found${NC}"
    echo "Please run 'make all' first"
    exit 1
fi

# Check if GDB is installed
if ! command -v gdb &> /dev/null; then
    echo -e "${RED}Error: GDB is not installed${NC}"
    echo "Install with: sudo apt install gdb"
    exit 1
fi

echo -e "${YELLOW}Instructions:${NC}"
echo "1. QEMU will start with GDB server on port 1234"
echo "2. QEMU will wait for GDB to connect"
echo "3. In GDB, use these commands:"
echo "   - break kernel_main    (set breakpoint)"
echo "   - continue             (start execution)"
echo "   - step / next          (step through code)"
echo "   - print variable       (inspect variables)"
echo "   - backtrace            (show call stack)"
echo ""

# Start QEMU in background with GDB server
echo -e "${BLUE}Starting QEMU with GDB server...${NC}"
qemu-system-x86_64 -cdrom build/novae.iso -m 512M -serial stdio -s -S &
QEMU_PID=$!

# Give QEMU time to start
sleep 1

# Start GDB
echo -e "${GREEN}Starting GDB...${NC}"
echo ""

# Create GDB init script
cat > /tmp/gdb_init_novae << 'EOF'
# Connect to QEMU
target remote localhost:1234

# Load symbols
symbol-file build/kernel.elf

# Set breakpoint at kernel_main
break kernel_main

# Display some info
echo \n
echo ========================================\n
echo GDB connected to QEMU\n
echo Breakpoint set at kernel_main\n
echo Type 'continue' to start execution\n
echo ========================================\n
echo \n
EOF

# Run GDB with init script
gdb -q -x /tmp/gdb_init_novae

# Clean up
rm /tmp/gdb_init_novae
kill $QEMU_PID 2>/dev/null || true

echo -e "${GREEN}Debug session ended${NC}"
