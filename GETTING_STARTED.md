# Getting Started with NovaeOS

This guide will help you get NovaeOS up and running quickly.

## Current Status

**Phase 0 ✓ Complete**: Toolchain and build system
**Phase 1 ✓ Complete**: Bootloader and minimal kernel

The kernel is ready to build and run! It will:
- Boot using GRUB2
- Initialize in 64-bit long mode
- Display a colorful banner
- Show memory layout information
- List initialized subsystems

## Prerequisites

Before you can build NovaeOS, you need to set up your development environment. Since you're on Windows, follow this setup guide:

### Quick Setup (Recommended)

**Use WSL2 (Windows Subsystem for Linux)**

```powershell
# 1. In Windows PowerShell (as Administrator):
wsl --install -d Ubuntu

# 2. Restart your computer

# 3. Open Ubuntu from Start menu

# 4. Install build tools:
sudo apt update
sudo apt install -y build-essential nasm xorriso grub-pc-bin \
    grub-common mtools git gdb qemu-system-x86
```

For detailed setup instructions, see **[WINDOWS_SETUP.md](WINDOWS_SETUP.md)**.

## Building NovaeOS

Once your environment is set up:

```bash
# Navigate to project directory (in WSL2):
cd /mnt/c/Users/haris/Desktop/personal/OS

# Build the kernel:
make all

# Create bootable ISO:
make iso

# Run in QEMU:
make run
```

## What You'll See

When you run `make run`, you should see:

1. **GRUB Menu**: Select "NovaeOS" (it will auto-select after 5 seconds)
2. **Kernel Boot**: The screen will clear
3. **Banner**: Colorful NovaeOS ASCII art banner
4. **Boot Info**: Information about the boot process
5. **Memory Layout**: Kernel memory sections and sizes
6. **Subsystem Init**: List of initialized subsystems
7. **Success Message**: "Kernel initialized successfully!"

## Available Make Targets

```bash
make all        # Build kernel
make iso        # Create bootable ISO
make run        # Run in QEMU
make debug      # Run with GDB debugging
make run-net    # Run with networking
make run-disk   # Run with disk image
make clean      # Clean build artifacts
make help       # Show help message
make info       # Show build configuration
```

## Running with Different Options

### Basic Run
```bash
make run
```

### With More Memory
```bash
qemu-system-x86_64 -cdrom build/novae.iso -m 1G
```

### With Serial Output
```bash
qemu-system-x86_64 -cdrom build/novae.iso -m 512M -serial stdio
```

### Debug Mode
```bash
# Terminal 1:
make debug

# Terminal 2:
gdb build/kernel.elf
(gdb) target remote localhost:1234
(gdb) break kernel_main
(gdb) continue
```

## Project Structure

```
OS/
├── boot/               # GRUB configuration
├── kernel/             # Kernel source code
│   ├── arch/x86_64/   # x86_64 boot code
│   ├── drivers/       # VGA driver
│   ├── include/       # Headers
│   └── main.c         # Kernel entry point
├── scripts/           # Helper scripts
│   ├── run.sh        # Run script with options
│   └── debug.sh      # Debug script
├── docs/              # Documentation
├── Makefile           # Build system
├── linker.ld          # Linker script
└── README.md          # Main readme

Generated directories:
├── build/             # Compiled objects and kernel.elf
└── iso/               # ISO staging area
```

## Files Created (Phase 0 & 1)

### Documentation
- `README.md` - Main project readme
- `ARCHITECTURE.md` - Complete system architecture
- `ROADMAP.md` - Implementation roadmap (9 phases)
- `WINDOWS_SETUP.md` - Windows-specific setup guide
- `GETTING_STARTED.md` - This file
- `LICENSE` - MIT License

### Build System
- `Makefile` - Complete build system with multiple targets
- `linker.ld` - Kernel linker script
- `.gitignore` - Git ignore patterns

### Boot & Kernel
- `boot/grub.cfg` - GRUB configuration
- `kernel/arch/x86_64/boot.S` - Assembly bootstrap (Multiboot2, long mode)
- `kernel/main.c` - Kernel main entry point
- `kernel/drivers/vga.c` - VGA text mode driver
- `kernel/include/kernel/vga.h` - VGA header

### Scripts
- `scripts/run.sh` - QEMU run script with options
- `scripts/debug.sh` - GDB debugging script

## Next Steps

### Immediate Next Step: Build and Run

```bash
cd /mnt/c/Users/haris/Desktop/personal/OS
make all
make iso
make run
```

### After Successful Run

Continue to **Phase 2: Memory Management**

This will implement:
- Physical memory manager (PMM)
- Virtual memory manager (VMM) with paging
- Kernel heap allocator (kmalloc/kfree)

See `ROADMAP.md` for detailed implementation plan.

## Troubleshooting

### "make: command not found"
You're not in WSL2 or MSYS2. Open Ubuntu (WSL2) from the Start menu.

### "grub-mkrescue: command not found"
Install GRUB tools: `sudo apt install grub-pc-bin grub-common xorriso`

### "qemu-system-x86_64: command not found"
Install QEMU: `sudo apt install qemu-system-x86`

### Build errors
1. Check toolchain: `make info`
2. Verify tools are installed: `make check-tools` (if target exists)
3. Clean and rebuild: `make clean && make all`

### QEMU crashes or black screen
1. Check serial output: Add `-serial stdio` to QEMU command
2. Enable debug mode: `make debug`
3. Verify ISO was created: `ls -lh build/novae.iso`

## Learning Resources

- **OSDev Wiki**: https://wiki.osdev.org/
- **Intel Manual**: https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
- **GRUB Manual**: https://www.gnu.org/software/grub/manual/
- **x86_64 ABI**: https://refspecs.linuxbase.org/elf/x86_64-abi-0.99.pdf

## Need Help?

1. Check `WINDOWS_SETUP.md` for environment setup
2. Check `ROADMAP.md` for implementation details
3. Check `ARCHITECTURE.md` for system design
4. Review code comments in source files

## Current Capabilities

After Phase 1, NovaeOS can:
- ✓ Boot from GRUB
- ✓ Run in 64-bit long mode
- ✓ Display text output (VGA text mode)
- ✓ Show formatted output (printf-style)
- ✓ Report memory layout

## Coming in Phase 2

- Physical memory allocator
- Virtual memory with paging
- Kernel heap (dynamic memory)
- Memory statistics

Stay tuned!

---

**Happy OS Development! 🚀**
