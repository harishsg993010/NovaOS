# Build and Test Guide for Phase 5

## Prerequisites

Make sure you have installed the build tools. If not, follow **INSTALL_TOOLS.md** first.

## Step 1: Verify Build Tools

```bash
# Check that tools are installed
gcc --version
nasm -v
make --version
qemu-system-x86_64 --version
```

Expected output:
- gcc (GCC) 13.x or later
- NASM version 2.x or later
- GNU Make 4.x or later
- QEMU emulator version 8.x or later

## Step 2: Navigate to Project

```bash
cd /c/Users/haris/Desktop/personal/OS
```

## Step 3: Clean Previous Builds

```bash
make clean
```

Expected output:
```
Cleaning build artifacts...
```

## Step 4: Build the Kernel

```bash
make all
```

Expected output:
```
Assembling kernel/arch/x86_64/boot.S...
Assembling kernel/arch/x86_64/interrupts.S...
Compiling kernel/main.c...
Compiling kernel/arch/x86_64/gdt.c...
Compiling kernel/arch/x86_64/idt.c...
... (many more files) ...
Compiling kernel/fs/vfs.c...
Compiling kernel/drivers/ata.c...
Compiling kernel/fs/simplefs.c...
Linking kernel...
Kernel linked: build/kernel.elf
Build complete!
```

### Common Build Errors

**Error: "No such file or directory"**
- Make sure you're in the right directory: `pwd`
- Should show: `/c/Users/haris/Desktop/personal/OS`

**Error: "undefined reference to..."**
- Some function is not implemented
- Check that all .c files are being compiled

**Error: "permission denied"**
- Close any program that might have files open
- Try: `make clean` then `make all`

## Step 5: Create Disk Image

```bash
qemu-img create -f raw disk.img 10M
```

Expected output:
```
Formatting 'disk.img', fmt=raw size=10485760
```

This creates a 10MB virtual disk image.

## Step 6: Run Without ISO (Direct Kernel Boot)

Since GRUB is not easily available on MSYS2, we'll boot the kernel directly:

```bash
qemu-system-x86_64 -kernel build/kernel.elf -m 512M \
    -drive file=disk.img,format=raw,index=0,media=disk \
    -serial stdio
```

### Expected Output

You should see the QEMU window open with:

```
    _   _                      ___  ____
   | \ | | _____   ____ _  ___|__ \/ ___|
   |  \| |/ _ \ \ / / _` |/ _ \ / /\___ \
   | |\  | (_) \ V / (_| |  __// /_ ___) |
   |_| \_|\___/ \_/ \__,_|\___|____|____/

   NovaeOS - Custom Operating System
   Version 0.1.0 (Development Build)
   Built for x86_64 architecture

Boot Information:
  Bootloader:   GRUB2 (Multiboot2)
  CPU Mode:     Long Mode (64-bit)
  Paging:       Enabled
  Interrupts:   Disabled

Kernel Memory Layout:
  Kernel Start: 0x...
  Kernel End:   0x...
  Kernel Size:  ... KB

  .text:   0x... - 0x... (... bytes)
  .rodata: 0x... - 0x... (... bytes)
  .data:   0x... - 0x... (... bytes)
  .bss:    0x... - 0x... (... bytes)

Initializing Kernel Subsystems:
  [ OK ] VGA Text Mode
  [ OK ] Physical Memory Manager (PMM)
  [ OK ] Virtual Memory Manager (VMM)
  [ OK ] Kernel Heap Allocator
  [ OK ] Global Descriptor Table (GDT)
  [ OK ] Interrupt Descriptor Table (IDT)
  [ OK ] Programmable Interrupt Controller (PIC)
  [ OK ] Timer (PIT)
  [ OK ] Process Management
  [ OK ] Scheduler
  [ OK ] System Call Interface
  [ OK ] Block Device Layer
  [ OK ] ATA Disk Driver
  ATA: Primary Master - QEMU HARDDISK (10 MB)
  Block: Registered device 'hda' (20480 blocks, 10485760 bytes)
  [ OK ] Virtual Filesystem (VFS)
  VFS: Initialized

Testing Memory Management:
  PMM: Allocating 3 pages...
    Allocated: 0x..., 0x..., 0x...
  PMM: Freeing middle page...
  PMM: Free pages: ... / ...
  Heap: Allocating memory...
    str1: Memory allocation works!
    str2: Heap allocator is functional!
    numbers[5] = 25
  Heap: Freeing memory...
  Heap: ... KB used, ... KB free, ... allocations
  VMM: Testing virtual memory mapping...
    Mapped 0x400000 -> 0x... (verified)

Testing Filesystem:
  Found disk: hda (10 MB)
  Formatting disk with SimpleFS...
  SimpleFS: Formatting device 'hda'...
  SimpleFS: Format complete (256 inodes, 20480 blocks)
  Creating SimpleFS instance...
  SimpleFS: Mounted successfully
  VFS: Registered filesystem 'simplefs'
  Mounting filesystem at '/'...
  VFS: Mounted 'simplefs' at '/'
  Filesystem mounted successfully!
  Note: File operations available via syscalls.

All subsystems initialized successfully!

Testing Multitasking:
  Created task 1 (PID 1)
  Created task 2 (PID 2)
  Created task 3 (PID 3)
  Created idle task (PID 4)
  Creating user mode process...
  Created user mode process (PID 5)
  Added tasks to scheduler

Starting Multitasking...
(You should see tasks alternating below)

[Task 1] Count: 0
[Task 2] Count: 0
[Task 3] Uptime: 100 ms
[User Mode] Iteration: 0
[Task 1] Count: 1
[Task 2] Count: 1
[User Mode] Iteration: 1
...
```

### Key Things to Look For

✅ **All subsystems show [ OK ]**
- If any show [FAIL], there's a problem

✅ **ATA driver detects disk**
- Should see: "ATA: Primary Master - QEMU HARDDISK (10 MB)"
- Should see: "Block: Registered device 'hda'"

✅ **Filesystem initialization works**
- Should see: "Formatting disk with SimpleFS..."
- Should see: "SimpleFS: Format complete"
- Should see: "Filesystem mounted successfully!"

✅ **Tasks run and alternate**
- Should see messages from Task 1, 2, 3, and User Mode
- Messages should alternate, showing multitasking works

## Step 7: Test with QEMU Monitor

For more advanced testing, you can access the QEMU monitor:

```bash
qemu-system-x86_64 -kernel build/kernel.elf -m 512M \
    -drive file=disk.img,format=raw,index=0,media=disk \
    -serial stdio -monitor stdio
```

In the monitor, you can:
- `info registers` - View CPU registers
- `info mem` - View memory mappings
- `x /10x 0x100000` - Examine memory
- `q` - Quit QEMU

## Step 8: Test Without Disk (Optional)

To verify the kernel works without filesystem:

```bash
qemu-system-x86_64 -kernel build/kernel.elf -m 512M -serial stdio
```

Expected output:
- Everything works except filesystem
- Should see: "No disk found (hda). Skipping filesystem tests."

## Debugging

### QEMU Window Shows Nothing

**Problem**: Black screen or no output

**Solutions**:
1. Check serial output in terminal
2. Try: `-nographic` flag
3. Verify kernel built successfully

### Kernel Crashes (Triple Fault)

**Problem**: QEMU resets repeatedly

**Solutions**:
1. Enable debugging: `-d int,cpu_reset -no-reboot`
2. Check for:
   - Stack overflow
   - Invalid memory access
   - Bad page tables

### No Disk Detected

**Problem**: "No disk found (hda)"

**Solutions**:
1. Verify disk image exists: `ls -lh disk.img`
2. Check QEMU command includes `-drive` parameter
3. Try different disk format: `-drive file=disk.img,format=raw,if=ide`

### Filesystem Format Fails

**Problem**: "Failed to format disk"

**Solutions**:
1. Check disk permissions
2. Verify disk size: `ls -lh disk.img`
3. Try recreating disk: `rm disk.img && qemu-img create -f raw disk.img 10M`

## Advanced Testing

### Test with GDB

Terminal 1 - Start QEMU with GDB server:
```bash
qemu-system-x86_64 -kernel build/kernel.elf -m 512M \
    -drive file=disk.img,format=raw,index=0,media=disk \
    -s -S
```

Terminal 2 - Connect with GDB:
```bash
gdb build/kernel.elf
(gdb) target remote localhost:1234
(gdb) break kernel_main
(gdb) continue
```

### Test with Different Disk Sizes

```bash
# Small disk (1MB)
qemu-img create -f raw disk_small.img 1M
qemu-system-x86_64 -kernel build/kernel.elf -m 512M \
    -drive file=disk_small.img,format=raw,index=0,media=disk

# Large disk (100MB)
qemu-img create -f raw disk_large.img 100M
qemu-system-x86_64 -kernel build/kernel.elf -m 512M \
    -drive file=disk_large.img,format=raw,index=0,media=disk
```

### Test with Multiple Disks

```bash
qemu-img create -f raw disk1.img 10M
qemu-img create -f raw disk2.img 10M

qemu-system-x86_64 -kernel build/kernel.elf -m 512M \
    -drive file=disk1.img,format=raw,index=0,media=disk \
    -drive file=disk2.img,format=raw,index=1,media=disk
```

Should detect both as hda and hdb.

## Success Criteria

Phase 5 is successfully working if you see:

- ✅ Kernel builds without errors
- ✅ All subsystems initialize successfully
- ✅ ATA driver detects disk
- ✅ Filesystem formats and mounts
- ✅ Tasks run concurrently
- ✅ User mode process executes
- ✅ No crashes or panics

## Next Steps

After successful testing:

1. **Implement Write Operations**
   - Add write functionality to SimpleFS
   - Test file creation

2. **Add File Creation**
   - Implement file creation syscalls
   - Create test files from user mode

3. **Move to Phase 6**
   - Implement networking stack
   - Add network card driver

## Troubleshooting Resources

- QEMU Documentation: https://www.qemu.org/docs/master/
- OSDev Wiki: https://wiki.osdev.org/
- Project README: README.md
- Phase 5 Docs: docs/PHASE5_FILESYSTEM.md

## Quick Reference

```bash
# Build
make clean && make all

# Create disk
qemu-img create -f raw disk.img 10M

# Run
qemu-system-x86_64 -kernel build/kernel.elf -m 512M \
    -drive file=disk.img,format=raw,index=0,media=disk \
    -serial stdio

# Debug
qemu-system-x86_64 -kernel build/kernel.elf -m 512M \
    -drive file=disk.img,format=raw,index=0,media=disk \
    -serial stdio -d int,cpu_reset -no-reboot

# Clean
make clean
```

Good luck testing! 🚀
