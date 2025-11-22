# NovaeOS Implementation Roadmap

This document outlines the complete implementation strategy for NovaeOS, broken into discrete phases. Each phase builds upon the previous, with clear milestones and testing criteria.

---

## Phase 0: Project Setup & Toolchain

**Duration**: 1-2 days

### Goals
- Set up development environment on Windows (WSL2/MSYS2)
- Install and configure cross-compiler toolchain
- Create project directory structure
- Write build scripts and Makefile
- Verify toolchain with "hello world" kernel

### Windows/WSL2 Setup Steps

#### Option A: Using WSL2 (Recommended)

```bash
# In Windows PowerShell (as Administrator)
wsl --install -d Ubuntu

# Inside WSL2 Ubuntu:
sudo apt update
sudo apt install -y build-essential nasm xorriso grub-pc-bin grub-common \
    mtools gcc-multilib g++-multilib git wget curl vim gdb

# Cross-compiler (if needed for bare metal)
sudo apt install -y gcc-x86-64-linux-gnu binutils-x86-64-linux-gnu
```

#### Option B: Using MSYS2

```bash
# Download and install MSYS2 from https://www.msys2.org/
# In MSYS2 MINGW64 terminal:
pacman -Syu
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-nasm \
    mingw-w64-x86_64-gdb make git
```

### Project Directory Structure

```
OS/
├── boot/                   # Bootloader files
│   ├── grub.cfg            # GRUB configuration
│   └── boot.S              # Bootstrap assembly
├── kernel/                 # Kernel source code
│   ├── main.c              # Kernel entry point
│   ├── arch/               # Architecture-specific code
│   │   └── x86_64/
│   │       ├── boot.S      # Early boot assembly
│   │       ├── gdt.c       # GDT management
│   │       ├── idt.c       # IDT management
│   │       ├── interrupts.S # ISR stubs
│   │       └── port_io.c   # I/O port access
│   ├── mm/                 # Memory management
│   │   ├── pmm.c           # Physical memory manager
│   │   ├── vmm.c           # Virtual memory manager
│   │   └── heap.c          # Kernel heap allocator
│   ├── sched/              # Process/thread scheduler
│   │   ├── process.c
│   │   ├── scheduler.c
│   │   └── context.S       # Context switching
│   ├── fs/                 # Filesystem
│   │   ├── vfs.c           # Virtual filesystem
│   │   └── simplefs.c      # Simple custom FS
│   ├── net/                # Network stack
│   │   ├── ethernet.c
│   │   ├── arp.c
│   │   ├── ip.c
│   │   ├── tcp.c
│   │   └── udp.c
│   ├── drivers/            # Device drivers
│   │   ├── vga.c           # VGA text mode
│   │   ├── framebuffer.c   # Linear framebuffer
│   │   ├── keyboard.c
│   │   ├── ata.c           # Disk driver
│   │   └── e1000.c         # Network card
│   └── include/            # Kernel headers
│       └── kernel/
├── lib/                    # Shared libraries
│   ├── string.c            # String functions
│   ├── stdio.c             # Standard I/O
│   └── stdlib.c            # Standard library
├── user/                   # Userland programs
│   ├── shell/              # Command shell
│   ├── init/               # Init process
│   ├── utils/              # Core utilities (ls, cat, etc.)
│   └── gui/                # GUI applications
│       ├── wm/             # Window manager
│       └── apps/           # GUI apps
├── scripts/                # Build and test scripts
│   ├── run.sh              # Run in QEMU
│   ├── debug.sh            # Debug with GDB
│   └── test.sh             # Run tests
├── docs/                   # Documentation
│   ├── ARCHITECTURE.md
│   ├── ROADMAP.md
│   └── SYSCALLS.md
├── build/                  # Build output (generated)
├── iso/                    # ISO image staging (generated)
├── Makefile                # Main build file
└── linker.ld               # Linker script
```

### Files to Create

- `Makefile` - Complete build system
- `linker.ld` - Linker script for kernel
- `boot/grub.cfg` - GRUB configuration
- `kernel/arch/x86_64/boot.S` - Assembly bootstrap
- `kernel/main.c` - Simple kernel main
- `scripts/run.sh` - QEMU launcher

### Milestone Test

```bash
make clean
make all
make iso
make run
```

**Expected Output**: QEMU starts, GRUB menu appears, kernel boots and displays "NovaeOS kernel initialized!" on screen.

### QEMU Command (from Windows/WSL2)

```bash
# From WSL2:
qemu-system-x86_64 -cdrom build/novae.iso -m 512M

# From Windows (if QEMU installed natively):
qemu-system-x86_64.exe -cdrom build\novae.iso -m 512M
```

---

## Phase 1: Bootloader & Minimal Kernel

**Duration**: 2-3 days

### Goals
- Implement GRUB2-based bootloader
- Write kernel bootstrap code (long mode transition)
- Set up initial GDT (Global Descriptor Table)
- Initialize VGA text mode output
- Print "Hello from NovaeOS!" to screen

### Files to Implement

1. **boot/grub.cfg**
   ```
   menuentry "NovaeOS" {
       multiboot2 /boot/kernel.elf
       boot
   }
   ```

2. **kernel/arch/x86_64/boot.S**
   - Multiboot2 header
   - Stack setup
   - Long mode checks
   - Page table initialization
   - Jump to kernel_main

3. **kernel/drivers/vga.c**
   - VGA text mode driver (80x25)
   - Functions: `vga_init()`, `vga_putchar()`, `vga_puts()`, `vga_clear()`

4. **kernel/main.c**
   ```c
   void kernel_main(void) {
       vga_init();
       vga_clear();
       vga_puts("Hello from NovaeOS!\n");
       vga_puts("Kernel loaded successfully.\n");
       while(1) { __asm__ volatile("hlt"); }
   }
   ```

5. **linker.ld**
   - Define memory sections (.text, .data, .bss, .rodata)
   - Set kernel load address (0x100000)

### Build Commands

```bash
# In WSL2:
cd /mnt/c/Users/haris/Desktop/personal/OS
make clean
make kernel
make iso
make run
```

### Milestone Test

- Boot in QEMU
- See GRUB menu
- Select "NovaeOS"
- Kernel prints messages to VGA text mode screen
- System halts (no crash)

### Expected Screen Output

```
Hello from NovaeOS!
Kernel loaded successfully.
```

---

## Phase 2: Memory Management

**Duration**: 3-4 days

### Goals
- Implement physical memory manager (PMM)
- Implement virtual memory manager (VMM) with paging
- Set up kernel heap allocator
- Test memory allocation/deallocation

### Files to Implement

1. **kernel/mm/pmm.c**
   - Bitmap-based physical page allocator
   - Functions: `pmm_init()`, `pmm_alloc_page()`, `pmm_free_page()`
   - Track available/used memory

2. **kernel/mm/vmm.c**
   - 4-level page table management
   - Functions: `vmm_init()`, `vmm_map_page()`, `vmm_unmap_page()`
   - Identity map kernel
   - Set up higher-half kernel

3. **kernel/mm/heap.c**
   - First-fit heap allocator
   - Functions: `kmalloc()`, `kfree()`, `krealloc()`
   - Block headers with size and free flags

4. **kernel/include/kernel/mm.h**
   - Memory management structures and prototypes

### Testing Code

```c
void test_memory(void) {
    // Test PMM
    void *page1 = pmm_alloc_page();
    void *page2 = pmm_alloc_page();
    vga_printf("Allocated pages: %p, %p\n", page1, page2);
    pmm_free_page(page1);

    // Test heap
    char *buf = kmalloc(256);
    strcpy(buf, "Memory allocation works!");
    vga_puts(buf);
    kfree(buf);
}
```

### Milestone Test

- Boot kernel
- Initialize PMM with available memory
- Initialize VMM and enable paging
- Allocate and free pages
- Use `kmalloc`/`kfree` successfully
- Print memory statistics

### QEMU Command

```bash
qemu-system-x86_64 -cdrom build/novae.iso -m 512M
```

---

## Phase 3: Interrupts, Exceptions & Scheduling

**Duration**: 4-5 days

### Goals
- Set up IDT (Interrupt Descriptor Table)
- Implement CPU exception handlers
- Configure PIC (Programmable Interrupt Controller)
- Set up timer interrupt (PIT)
- Implement basic round-robin scheduler
- Enable context switching between kernel tasks

### Files to Implement

1. **kernel/arch/x86_64/idt.c**
   - IDT setup and loading
   - Exception handlers (divide by zero, page fault, etc.)

2. **kernel/arch/x86_64/interrupts.S**
   - ISR stubs (save/restore registers)
   - Common interrupt handler

3. **kernel/arch/x86_64/pic.c**
   - PIC initialization and configuration
   - IRQ masking/unmasking

4. **kernel/drivers/timer.c**
   - PIT (Programmable Interval Timer) setup
   - Timer interrupt handler
   - System tick counter

5. **kernel/sched/process.c**
   - Process structure
   - Process creation and destruction

6. **kernel/sched/scheduler.c**
   - Round-robin scheduler
   - Task queue management
   - `schedule()` function called from timer

7. **kernel/sched/context.S**
   - `switch_context(old, new)` - saves/restores all registers

### Testing Code

```c
void task1(void) {
    while(1) {
        vga_puts("Task 1\n");
        for(volatile int i = 0; i < 10000000; i++);
    }
}

void task2(void) {
    while(1) {
        vga_puts("Task 2\n");
        for(volatile int i = 0; i < 10000000; i++);
    }
}

void test_scheduling(void) {
    create_kernel_task(task1, "task1");
    create_kernel_task(task2, "task2");
    enable_scheduling();
}
```

### Milestone Test

- Boot kernel
- Initialize interrupts and timer
- Create two kernel tasks
- See alternating output from both tasks
- Verify timer is firing (print tick count)

### QEMU Command

```bash
qemu-system-x86_64 -cdrom build/novae.iso -m 512M
```

---

## Phase 4: Syscalls & User Mode Processes

**Duration**: 5-6 days

### Goals
- Implement syscall interface (`int 0x80` or `syscall`)
- Create user mode (Ring 3) execution
- Implement basic syscalls: write, read, exit, fork, exec
- Load and execute ELF user programs
- Test simple user program

### Files to Implement

1. **kernel/arch/x86_64/syscall.c**
   - Syscall dispatcher
   - Syscall table

2. **kernel/sched/process.c** (extend)
   - `fork()` - duplicate process
   - `exec()` - load and execute ELF
   - User mode transition

3. **kernel/fs/vfs.c** (basic)
   - File descriptor table
   - Basic file operations

4. **user/init/init.c**
   - First user process
   - Simple test program

5. **user/lib/syscall.S**
   - User-space syscall wrappers

6. **user/lib/libc.c**
   - Minimal C library for user programs

### Example User Program

```c
// user/init/init.c
#include "syscalls.h"

void _start(void) {
    const char *msg = "Hello from user space!\n";
    syscall_write(1, msg, strlen(msg));
    syscall_exit(0);
}
```

### Milestone Test

- Boot kernel
- Kernel creates init process in user mode
- Init process calls `write` syscall
- Message appears on screen
- Process exits cleanly

### QEMU Command

```bash
qemu-system-x86_64 -cdrom build/novae.iso -m 512M
```

---

## Phase 5: Filesystem Support

**Duration**: 5-7 days

### Goals
- Implement VFS (Virtual Filesystem) layer
- Create simple custom filesystem (SimpleFS)
- Implement ATA/IDE disk driver
- Support basic file operations: open, read, write, close, mkdir, readdir
- Create ramdisk or disk image with files

### Files to Implement

1. **kernel/fs/vfs.c**
   - VFS node structure
   - Mount point management
   - Path resolution

2. **kernel/fs/simplefs.c**
   - SimpleFS implementation
   - Superblock, inodes, data blocks
   - Read/write operations

3. **kernel/drivers/ata.c**
   - ATA PIO mode driver
   - Read/write sectors

4. **kernel/fs/initrd.c**
   - Initial ramdisk support
   - Load from multiboot

### Testing Code

```c
void test_filesystem(void) {
    // Mount root filesystem
    vfs_mount("/", simplefs_driver);

    // Create file
    int fd = syscall_open("/test.txt", O_CREAT | O_WRONLY);
    syscall_write(fd, "Hello, filesystem!", 18);
    syscall_close(fd);

    // Read file
    fd = syscall_open("/test.txt", O_RDONLY);
    char buf[32];
    syscall_read(fd, buf, 32);
    vga_puts(buf);
    syscall_close(fd);
}
```

### Milestone Test

- Boot with disk image or ramdisk
- Mount root filesystem
- Create, write, read, and delete files
- List directory contents
- Verify data persistence (across reboots if using disk image)

### QEMU Command

```bash
# With disk image:
qemu-system-x86_64 -cdrom build/novae.iso -m 512M \
    -drive file=disk.img,format=raw,index=0,media=disk
```

---

## Phase 6: Networking Stack

**Duration**: 7-10 days

### Goals
- Implement e1000 network card driver
- Implement Ethernet layer
- Implement ARP, IP, ICMP
- Implement UDP and basic TCP
- Create socket interface
- Test ping and simple UDP communication

### Files to Implement

1. **kernel/drivers/e1000.c**
   - E1000 NIC initialization
   - RX/TX ring buffers
   - Packet send/receive

2. **kernel/net/ethernet.c**
   - Ethernet frame parsing
   - MAC address handling

3. **kernel/net/arp.c**
   - ARP cache
   - ARP request/reply

4. **kernel/net/ip.c**
   - IP packet processing
   - Routing (basic)

5. **kernel/net/icmp.c**
   - ICMP echo request/reply (ping)

6. **kernel/net/udp.c**
   - UDP datagram handling

7. **kernel/net/tcp.c**
   - TCP state machine (simplified)
   - Connection management

8. **kernel/net/socket.c**
   - Socket API
   - Syscalls: socket, bind, connect, send, recv

### Testing Code

```c
void test_network(void) {
    // Initialize network
    e1000_init();
    set_ip_address(IP(192, 168, 1, 100));

    // Test ping
    icmp_send_echo(IP(192, 168, 1, 1), 1234, "ping!", 5);

    // Test UDP
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8080),
        .sin_addr = IP(192, 168, 1, 1)
    };
    sendto(sock, "Hello UDP!", 10, 0, &addr, sizeof(addr));
}
```

### Milestone Test

- Boot with network enabled in QEMU
- Initialize NIC
- Send and receive Ethernet frames
- Respond to ARP requests
- Respond to ping (ICMP echo)
- Send/receive UDP packets
- Establish basic TCP connection

### QEMU Command

```bash
qemu-system-x86_64 -cdrom build/novae.iso -m 512M \
    -netdev user,id=net0,hostfwd=tcp::8080-:8080 \
    -device e1000,netdev=net0
```

---

## Phase 7: Terminal & Shell

**Duration**: 4-5 days

### Goals
- Implement keyboard driver
- Create terminal emulator (scrolling, cursor, input)
- Build interactive shell
- Implement core commands: ls, cat, echo, mkdir, rm, ps, etc.

### Files to Implement

1. **kernel/drivers/keyboard.c**
   - PS/2 keyboard driver
   - Scancode to ASCII conversion
   - Interrupt handler

2. **user/shell/terminal.c**
   - Terminal emulation
   - Line editing (backspace, arrow keys)
   - Command history

3. **user/shell/shell.c**
   - Command parsing
   - Built-in commands
   - Program execution

4. **user/utils/ls.c**
   - List directory contents

5. **user/utils/cat.c**
   - Display file contents

6. **user/utils/echo.c**
   - Print arguments

7. **user/utils/ps.c**
   - List processes

### Testing

- Boot to shell prompt
- Type commands
- Execute programs
- Navigate filesystem
- Manage processes

### Milestone Test

```bash
# In NovaeOS shell:
$ ls /
bin  etc  home  tmp

$ echo "Hello World"
Hello World

$ cat /etc/motd
Welcome to NovaeOS!

$ ps
PID   NAME      STATE
1     init      RUNNING
2     shell     RUNNING
```

### QEMU Command

```bash
qemu-system-x86_64 -cdrom build/novae.iso -m 512M
```

---

## Phase 8: Graphics & GUI

**Duration**: 7-10 days

### Goals
- Set up linear framebuffer (VESA or GOP)
- Implement drawing primitives
- Create simple window manager
- Build basic widgets (button, label, textbox)
- Create demo GUI applications

### Files to Implement

1. **kernel/drivers/framebuffer.c**
   - Framebuffer initialization (VESA BIOS or UEFI GOP)
   - Pixel plotting

2. **user/gui/graphics.c**
   - Drawing primitives (line, rect, circle, text)
   - Font rendering (bitmap font)

3. **user/gui/wm/window.c**
   - Window structure
   - Window creation/destruction
   - Window events

4. **user/gui/wm/compositor.c**
   - Window compositing
   - Z-order management
   - Damage rectangles

5. **user/gui/wm/decorations.c**
   - Title bars
   - Borders
   - Close/minimize buttons

6. **user/gui/widgets/button.c**
7. **user/gui/widgets/label.c**
8. **user/gui/widgets/textbox.c**

9. **user/gui/apps/terminal.c**
   - GUI terminal emulator

10. **user/gui/apps/clock.c**
    - Simple clock application

### Testing

- Boot to GUI
- See desktop with wallpaper
- Open terminal window
- Type commands in GUI terminal
- Open clock app
- Move, resize, close windows

### Milestone Test

- Framebuffer initializes with 1024x768x32
- Desktop background renders
- Window manager starts
- Can create and manipulate windows
- GUI terminal is functional

### QEMU Command

```bash
qemu-system-x86_64 -cdrom build/novae.iso -m 512M \
    -vga std -display sdl
```

---

## Phase 9: Core Utilities & Packaging

**Duration**: 5-7 days

### Goals
- Implement remaining utilities (cp, mv, rm, mkdir, rmdir, touch, etc.)
- Add network utilities (ping, netstat, ifconfig)
- Create package/installer system
- Write comprehensive documentation
- Polish and bug fixes

### Files to Implement

1. **user/utils/cp.c**
2. **user/utils/mv.c**
3. **user/utils/rm.c**
4. **user/utils/mkdir.c**
5. **user/utils/touch.c**
6. **user/net/ping.c**
7. **user/net/netstat.c**
8. **user/net/ifconfig.c**

### Documentation

- **README.md**: Quick start guide
- **BUILDING.md**: Detailed build instructions
- **SYSCALLS.md**: Complete syscall reference
- **DRIVER_DEV.md**: How to write drivers
- **USER_PROGRAMS.md**: How to write user programs

### Final Testing

- Complete boot-to-desktop flow
- All utilities work correctly
- Network stack functional
- GUI responsive
- No major bugs

### Release

- Create bootable ISO
- Create USB installer (future)
- Package source code
- Publish documentation

### QEMU Command (Final)

```bash
qemu-system-x86_64 -cdrom build/novae.iso \
    -m 1G \
    -drive file=disk.img,format=raw,index=0,media=disk \
    -netdev user,id=net0,hostfwd=tcp::8080-:8080 \
    -device e1000,netdev=net0 \
    -vga std \
    -display sdl \
    -enable-kvm  # If on Linux with KVM support
```

---

## Summary Timeline

| Phase | Duration | Cumulative |
|-------|----------|------------|
| Phase 0: Setup | 1-2 days | 2 days |
| Phase 1: Boot & Kernel | 2-3 days | 5 days |
| Phase 2: Memory | 3-4 days | 9 days |
| Phase 3: Interrupts & Sched | 4-5 days | 14 days |
| Phase 4: Syscalls & Userland | 5-6 days | 20 days |
| Phase 5: Filesystem | 5-7 days | 27 days |
| Phase 6: Networking | 7-10 days | 37 days |
| Phase 7: Terminal & Shell | 4-5 days | 42 days |
| Phase 8: GUI | 7-10 days | 52 days |
| Phase 9: Utilities & Polish | 5-7 days | 59 days |

**Total Estimated Time**: ~2 months of focused development

---

## Next Steps

Proceed to implement **Phase 0** - see the actual code files being created in the next sections!
