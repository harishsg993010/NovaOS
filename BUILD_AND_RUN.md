# Building and Running NovaeOS

This guide provides step-by-step instructions for building and running NovaeOS with Phase 2 (Memory Management) complete.

## Current Status

✓ **Phase 0**: Toolchain and build system
✓ **Phase 1**: Bootloader and minimal kernel
✓ **Phase 2**: Memory management (PMM, VMM, Heap)

## Prerequisites

You need a Linux environment to build. On Windows, use WSL2:

### One-Time Setup (Windows)

```powershell
# In PowerShell as Administrator:
wsl --install -d Ubuntu
# Restart computer
```

```bash
# In WSL2 Ubuntu:
sudo apt update
sudo apt install -y build-essential nasm xorriso grub-pc-bin \
    grub-common mtools git gdb qemu-system-x86
```

**Verify Installation**:
```bash
gcc --version      # Should show GCC 9.x or later
nasm --version     # Should show NASM 2.x
grub-mkrescue --version
qemu-system-x86_64 --version
```

## Building NovaeOS

### Quick Build

```bash
# Navigate to project directory
cd /mnt/c/Users/haris/Desktop/personal/OS

# Clean previous build (optional)
make clean

# Build kernel
make all

# Create bootable ISO
make iso

# Run in QEMU
make run
```

### What Gets Built

```
build/
├── kernel/
│   ├── main.o              # Kernel entry point
│   ├── arch/x86_64/
│   │   └── boot.o          # Bootstrap code
│   ├── drivers/
│   │   └── vga.o           # VGA driver
│   └── mm/
│       ├── pmm.o           # Physical memory manager
│       ├── vmm.o           # Virtual memory manager
│       └── heap.o          # Kernel heap
├── lib/
│   └── string.o            # String utilities
├── kernel.elf              # Final kernel executable
└── novae.iso               # Bootable ISO image
```

## Running NovaeOS

### Basic Run

```bash
make run
```

This will:
1. Start QEMU with the ISO
2. Boot GRUB
3. GRUB loads the kernel
4. Kernel initializes and runs tests

### Run Options

```bash
# Run with custom memory size
qemu-system-x86_64 -cdrom build/novae.iso -m 1G

# Run with serial output (see kernel logs)
qemu-system-x86_64 -cdrom build/novae.iso -m 512M -serial stdio

# Run with debug server (for GDB)
make debug
# Then in another terminal:
gdb build/kernel.elf -ex 'target remote localhost:1234'
```

### What You Should See

When you run `make run`, you should see:

```
    _   _                      ___  ____
   | \ | | _____   ____ _  ___|__ \/ ___|
   |  \| |/ _ \ \ / / _` |/ _ \ / /\___ \
   | |\  | (_) \ V / (_| |  __// /_ ___) |
   |_| \_|\___/ \_/ \__,_|\___/____|____/

   NovaeOS - Custom Operating System
   Version 0.1.0 (Development Build)
   Built for x86_64 architecture

Boot Information:
  Bootloader:   GRUB2 (Multiboot2)
  CPU Mode:     Long Mode (64-bit)
  Paging:       Enabled
  Interrupts:   Disabled

Kernel Memory Layout:
  Kernel Start: 0x100000
  Kernel End:   0x...
  Kernel Size:  ... KB

  .text:   0x... - 0x... (... bytes)
  .rodata: 0x... - 0x... (... bytes)
  .data:   0x... - 0x... (... bytes)
  .bss:    0x... - 0x... (... bytes)

Initializing Kernel Subsystems:
  [ OK ] VGA Text Mode
  PMM: Managing 512 MB (131072 pages)
  PMM: Kernel occupies ... KB (... pages)
  PMM: ... pages used, ... pages free
  [ OK ] Physical Memory Manager (PMM)
  VMM: Current PML4 at 0x...
  VMM: Kernel mapped to higher half (0xFFFF800000000000+)
  VMM: Paging enabled with 4-level page tables
  [ OK ] Virtual Memory Manager (VMM)
  Heap: Initialized at 0x..., size 16384 KB
  [ OK ] Kernel Heap Allocator

Testing Memory Management:
  PMM: Allocating 3 pages...
    Allocated: 0x..., 0x..., 0x...
  PMM: Freeing middle page...
  PMM: Free pages: ... / 131072
  Heap: Allocating memory...
    str1: Memory allocation works!
    str2: Heap allocator is functional!
    numbers[5] = 25
  Heap: Freeing memory...
  Heap: 0 KB used, 16384 KB free, 0 allocations
  VMM: Testing virtual memory mapping...
    Mapped 0x400000 -> 0x... (verified)

All tests passed! Kernel initialized successfully!

System is ready. Halting CPU...
```

## Debugging

### Using GDB

**Terminal 1** (Run QEMU with GDB server):
```bash
make debug
# or manually:
qemu-system-x86_64 -cdrom build/novae.iso -m 512M -s -S -serial stdio
```

**Terminal 2** (Connect GDB):
```bash
gdb build/kernel.elf
(gdb) target remote localhost:1234
(gdb) break kernel_main
(gdb) continue
(gdb) step
(gdb) print variable_name
(gdb) backtrace
```

### Useful GDB Commands

```gdb
# Set breakpoints
break kernel_main
break pmm_init
break *0x100000

# Examine memory
x/10x 0xb8000          # Show VGA text buffer
x/4gx $rsp             # Show stack

# Step through code
step                    # Step into
next                    # Step over
finish                  # Step out
continue                # Continue execution

# Print variables
print heap_size
print *current_pml4

# Show registers
info registers
info registers rax rbx rcx

# Show backtrace
backtrace
backtrace full
```

## Common Build Errors

### Error: "make: command not found"

**Solution**: You're not in WSL2. Open "Ubuntu" from Windows Start menu.

### Error: "nasm: command not found"

**Solution**:
```bash
sudo apt install nasm
```

### Error: "grub-mkrescue: command not found"

**Solution**:
```bash
sudo apt install grub-pc-bin grub-common xorriso
```

### Error: "multiple definition of `memcpy'"

**Solution**: Make sure you're using `-nostdlib` in CFLAGS (already in Makefile).

### Error: Kernel crashes on boot

**Symptoms**: Triple fault, QEMU resets

**Debug Steps**:
1. Enable serial output: `qemu-system-x86_64 -cdrom build/novae.iso -serial stdio`
2. Check if boot.S is properly assembled
3. Verify linker script produces correct layout
4. Use GDB to see where it crashes: `make debug`

## Testing Individual Components

### Test PMM Only

Add to `kernel/main.c` before other tests:
```c
vga_puts("Testing PMM...\n");
for (int i = 0; i < 10; i++) {
    uint64_t page = pmm_alloc_page();
    vga_printf("  Allocated page %d: 0x%x\n", i, page);
}
vga_printf("Free pages: %u\n", pmm_get_free_pages());
```

### Test Heap Only

```c
vga_puts("Testing Heap...\n");
for (int i = 0; i < 5; i++) {
    void *ptr = kmalloc(1024);
    vga_printf("  Allocated %d: 0x%x\n", i, (uint64_t)ptr);
    kfree(ptr);
}
heap_print_stats();
```

### Test VMM Only

```c
vga_puts("Testing VMM...\n");
for (uint64_t virt = 0x400000; virt < 0x500000; virt += PAGE_SIZE) {
    uint64_t phys = pmm_alloc_page();
    vmm_map_page(virt, phys, PAGE_FLAGS_KERNEL);
    vga_printf("  Mapped 0x%x -> 0x%x\n", virt, phys);
}
```

## Performance Testing

Add to kernel main:
```c
// Allocate 1000 pages and measure time
vga_puts("Allocating 1000 pages...\n");
uint64_t start = read_tsc();  // You'll need to implement this
for (int i = 0; i < 1000; i++) {
    pmm_alloc_page();
}
uint64_t end = read_tsc();
vga_printf("Time: %u cycles\n", (uint32_t)(end - start));
```

## Clean Build

```bash
make clean      # Remove all build artifacts
make all        # Rebuild everything
make iso        # Create ISO
make run        # Test
```

## Next Steps

After successfully building and running Phase 2:

1. **Verify all tests pass** - Check output matches expected results
2. **Try modifying code** - Change heap size, test different allocations
3. **Read Phase 3** - Start learning about interrupts and scheduling
4. **Experiment** - Add your own test cases

## Quick Reference

```bash
# Full build and run sequence:
make clean && make all && make iso && make run

# Debug sequence:
# Terminal 1:
make debug

# Terminal 2:
gdb build/kernel.elf -ex 'target remote localhost:1234' -ex 'break kernel_main' -ex 'continue'

# Check if tools are installed:
make info

# See available targets:
make help
```

## Help and Resources

- **Build issues**: Check `WINDOWS_SETUP.md`
- **Architecture questions**: See `ARCHITECTURE.md`
- **Phase details**: See `ROADMAP.md` and `docs/PHASE2_MEMORY.md`
- **General info**: See `README.md`

---

**Happy Kernel Hacking!** 🚀
