# NovaeOS - A Custom Operating System

NovaeOS is a custom x86_64 operating system built from scratch for educational purposes. It includes a bootable kernel, memory management, process scheduling, filesystem, networking, terminal/shell, and a simple GUI.

## Features

- **x86_64 Architecture**: 64-bit kernel with long mode support
- **Memory Management**: Physical and virtual memory management with paging
- **Process Scheduling**: Round-robin scheduler with preemption
- **Filesystem**: VFS layer with custom SimpleFS implementation
- **Networking**: TCP/IP stack with e1000 NIC driver
- **Terminal & Shell**: Interactive command-line interface
- **GUI**: Basic window manager with framebuffer graphics
- **Syscalls**: POSIX-like system call interface

## Development Environment (Windows)

This project is designed to be built on **Windows** using either **WSL2** or **MSYS2/MinGW**.

### Option A: WSL2 (Ubuntu) - Recommended

1. **Install WSL2**:
   ```powershell
   # In Windows PowerShell (as Administrator)
   wsl --install -d Ubuntu
   ```

2. **Install build tools** (inside WSL2):
   ```bash
   sudo apt update
   sudo apt install -y build-essential nasm xorriso grub-pc-bin \
       grub-common mtools git gdb
   ```

3. **Navigate to project**:
   ```bash
   cd /mnt/c/Users/haris/Desktop/personal/OS
   ```

### Option B: MSYS2/MinGW

1. **Install MSYS2**: Download from https://www.msys2.org/

2. **Install build tools** (in MSYS2 terminal):
   ```bash
   pacman -Syu
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-nasm \
       mingw-w64-x86_64-gdb make git
   ```

3. **For GRUB**: Download grub-mkrescue for Windows or use WSL2

### QEMU (Required)

QEMU should already be installed on your system. Verify with:
```bash
qemu-system-x86_64 --version
```

If not installed:
- **WSL2**: `sudo apt install qemu-system-x86`
- **Windows**: Download from https://qemu.weilnetz.de/

## Building NovaeOS

### Quick Start

```bash
# Build everything
make all

# Create bootable ISO
make iso

# Run in QEMU
make run
```

### Individual Build Targets

```bash
make kernel      # Build kernel only
make clean       # Clean build artifacts
make debug       # Run in QEMU with GDB server
```

## Running NovaeOS

### Basic Run

```bash
make run
```

Or manually:
```bash
qemu-system-x86_64 -cdrom build/novae.iso -m 512M
```

### With Networking

```bash
qemu-system-x86_64 -cdrom build/novae.iso -m 512M \
    -netdev user,id=net0 \
    -device e1000,netdev=net0
```

### With Disk Image

```bash
# Create disk image (first time only)
qemu-img create -f raw disk.img 100M

# Run with disk
qemu-system-x86_64 -cdrom build/novae.iso -m 512M \
    -drive file=disk.img,format=raw,index=0,media=disk
```

### Debug Mode

```bash
make debug
# In another terminal:
gdb build/kernel.elf
(gdb) target remote localhost:1234
(gdb) break kernel_main
(gdb) continue
```

## Project Structure

```
OS/
├── boot/               Bootloader configuration
├── kernel/             Kernel source code
│   ├── arch/x86_64/   Architecture-specific code
│   ├── mm/            Memory management
│   ├── sched/         Scheduler and processes
│   ├── fs/            Filesystem
│   ├── net/           Network stack
│   ├── drivers/       Device drivers
│   └── include/       Kernel headers
├── lib/               Shared libraries
├── user/              Userland programs
│   ├── shell/         Command shell
│   ├── init/          Init process
│   ├── utils/         Core utilities
│   └── gui/           GUI applications
├── scripts/           Build and test scripts
├── docs/              Documentation
├── build/             Build output (generated)
└── iso/               ISO image staging (generated)
```

## Current Status

**Phase 1**: Bootloader & Minimal Kernel ✓
- Multiboot2 bootloader (GRUB)
- Long mode initialization
- VGA text mode output

**Phase 2**: Memory Management ✓
- Physical memory manager (PMM) - Bitmap allocator
- Virtual memory manager (VMM) - 4-level paging
- Kernel heap allocator - First-fit with coalescing
- Dynamic memory allocation (kmalloc/kfree)
- Comprehensive testing suite

**Phase 3**: Interrupts & Scheduling ✓
- IDT (Interrupt Descriptor Table) with 256 entries
- CPU exception handlers (all 32 exceptions)
- PIC driver for hardware interrupt management
- Timer driver (PIT) with 100Hz ticks
- Process management with PCB structures
- Round-robin scheduler with preemption
- Full context switching
- Multiple concurrent tasks

**Phase 4**: Syscalls & User Mode ✓
- System call interface via int 0x80
- GDT with user/kernel segments (Ring 0/Ring 3)
- User mode process creation and execution
- 9 core syscalls implemented (exit, write, getpid, sleep, etc.)
- User space library with C wrappers
- Address space isolation per process
- Working user mode test programs

**Phase 5**: Filesystem ✓
- Virtual Filesystem (VFS) abstraction layer
- Block device interface
- ATA disk driver (PIO mode)
- SimpleFS custom filesystem implementation
- File operation syscalls (open, close, read)
- Disk detection and formatting
- Mount point management
- Full integration with kernel

**Phase 6**: Networking (Next)
- Network card driver (e1000)
- Network stack (Ethernet, IP, TCP/UDP)
- Socket interface
- Basic networking utilities

## Documentation

- **[ARCHITECTURE.md](ARCHITECTURE.md)**: System architecture and design
- **[ROADMAP.md](ROADMAP.md)**: Implementation roadmap and milestones
- **[WINDOWS_SETUP.md](WINDOWS_SETUP.md)**: Windows development environment setup
- **[GETTING_STARTED.md](GETTING_STARTED.md)**: Quick start guide
- **[BUILD_AND_RUN.md](BUILD_AND_RUN.md)**: Building and running instructions
- **[docs/PHASE2_MEMORY.md](docs/PHASE2_MEMORY.md)**: Phase 2 memory management details
- **[docs/PHASE3_INTERRUPTS.md](docs/PHASE3_INTERRUPTS.md)**: Phase 3 interrupts & scheduling details
- **[docs/PHASE4_SYSCALLS.md](docs/PHASE4_SYSCALLS.md)**: Phase 4 system calls & user mode details
- **[docs/PHASE5_FILESYSTEM.md](docs/PHASE5_FILESYSTEM.md)**: Phase 5 filesystem implementation details

## Development Workflow

1. **Make changes** to kernel or userland code
2. **Build**: `make all`
3. **Test**: `make run`
4. **Debug**: `make debug` + GDB in another terminal
5. **Iterate**!

## Troubleshooting

### Build Errors

**"Command not found: nasm"**
- Install NASM: `sudo apt install nasm` (WSL2) or via MSYS2

**"grub-mkrescue: command not found"**
- Install GRUB tools: `sudo apt install grub-pc-bin grub-common xorriso`

### Runtime Errors

**QEMU crashes or triple faults**
- Enable debug logging: `make debug`
- Check serial output: Add `-serial stdio` to QEMU command
- Verify GDT/IDT setup

**Page faults**
- Check memory mappings
- Verify page table entries
- Ensure stack is properly aligned

## Contributing

This is an educational project. Feel free to:
- Report bugs
- Suggest improvements
- Add features
- Improve documentation

## License

MIT License - see LICENSE file

## Resources

- [OSDev Wiki](https://wiki.osdev.org/)
- [Intel 64 and IA-32 Architectures Software Developer's Manual](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
- [GRUB Manual](https://www.gnu.org/software/grub/manual/)
- [AMD64 Architecture Programmer's Manual](https://www.amd.com/en/support/tech-docs)

---

**Happy OS Development!**
