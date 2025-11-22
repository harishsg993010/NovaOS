# NovaeOS Architecture Documentation

## Overview

NovaeOS is a custom x86_64 operating system built from scratch with the following major components:

```
┌─────────────────────────────────────────────────────────────┐
│                     USER APPLICATIONS                        │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐      │
│  │   Shell  │ │ GUI Apps │ │ Network  │ │   File   │      │
│  │          │ │          │ │  Utils   │ │  Utils   │      │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘      │
└─────────────────────────────────────────────────────────────┘
                           │
                   Syscall Interface
                           │
┌─────────────────────────────────────────────────────────────┐
│                      KERNEL SPACE                            │
│  ┌────────────────────────────────────────────────────────┐ │
│  │              Process & Thread Manager                   │ │
│  │  (Scheduler, Context Switch, IPC)                       │ │
│  └────────────────────────────────────────────────────────┘ │
│                                                              │
│  ┌──────────────┐  ┌──────────────┐  ┌─────────────────┐  │
│  │   Memory     │  │  Filesystem  │  │    Network      │  │
│  │  Management  │  │     VFS      │  │     Stack       │  │
│  │  (Paging,    │  │  (ext2/FAT)  │  │  (TCP/IP/UDP)   │  │
│  │   Heap)      │  │              │  │                 │  │
│  └──────────────┘  └──────────────┘  └─────────────────┘  │
│                                                              │
│  ┌────────────────────────────────────────────────────────┐ │
│  │                  Device Drivers                         │ │
│  │  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────────┐    │ │
│  │  │ VGA/ │ │ KB/  │ │ Disk │ │ Net  │ │  Timer   │    │ │
│  │  │  FB  │ │Mouse │ │ ATA  │ │ E1000│ │  PIT/RTC │    │ │
│  │  └──────┘ └──────┘ └──────┘ └──────┘ └──────────┘    │ │
│  └────────────────────────────────────────────────────────┘ │
│                                                              │
│  ┌────────────────────────────────────────────────────────┐ │
│  │          Hardware Abstraction Layer (HAL)               │ │
│  │    (Interrupts, GDT, IDT, PIC/APIC, PCI)               │ │
│  └────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
                           │
┌─────────────────────────────────────────────────────────────┐
│                       HARDWARE                               │
│         CPU, Memory, Disk, Network, Graphics                │
└─────────────────────────────────────────────────────────────┘
```

## Boot Flow

```
UEFI/BIOS Firmware
       │
       v
┌─────────────────┐
│   Bootloader    │  (GRUB2 or custom)
│   - Loads kernel│
│   - Sets up GDT │
│   - Enters long │
│     mode (64bit)│
└─────────────────┘
       │
       v
┌─────────────────┐
│  Kernel Entry   │  kernel/arch/x86_64/boot.S
│   - Initialize  │
│     stack       │
│   - Call main() │
└─────────────────┘
       │
       v
┌─────────────────┐
│  kernel_main()  │  kernel/main.c
│   - Init VGA    │
│   - Init GDT/IDT│
│   - Init memory │
│   - Init drivers│
│   - Start sched │
│   - Init shell  │
└─────────────────┘
```

## Memory Layout

### Physical Memory (example for 4GB system)

```
0x0000000000000000 - 0x00000000000003FF    IVT (legacy)
0x0000000000000400 - 0x00000000000004FF    BDA (legacy)
0x0000000000000500 - 0x0000000000007BFF    Free conventional memory
0x0000000000007C00 - 0x0000000000007DFF    Bootloader (512 bytes)
0x0000000000007E00 - 0x000000000009FFFF    Free conventional memory
0x00000000000A0000 - 0x00000000000FFFFF    VGA, ROM, etc.
0x0000000000100000 - 0x00000000FFFFFFFF    Extended memory (kernel + user)

Kernel loaded at: 0x100000 (1MB mark)
```

### Virtual Memory Layout (64-bit)

```
0x0000000000000000 - 0x00007FFFFFFFFFFF    User space (128TB)
  0x0000000000400000 - 0x0000000000600000    User code (.text)
  0x0000000000600000 - 0x0000000000800000    User data (.data, .bss)
  0x0000700000000000 - 0x00007FFFFFFFFFFF    User heap (grows up)
  0x00007FFFFFFFFFF0 - 0x00007FFFFFFFFFFF    User stack (grows down)

0xFFFF800000000000 - 0xFFFFFFFFFFFFFFFF    Kernel space (128TB)
  0xFFFF800000000000 - 0xFFFF800008000000    Direct map of physical RAM
  0xFFFF800100000000 - 0xFFFF800100100000    Kernel code (.text)
  0xFFFF800100100000 - 0xFFFF800100200000    Kernel data (.data, .rodata)
  0xFFFF800100200000 - 0xFFFF800100300000    Kernel BSS
  0xFFFF800200000000 - 0xFFFF800300000000    Kernel heap
  0xFFFFFFFF80000000 - 0xFFFFFFFFFFFFFFFF    Kernel stack
```

## Process & Task Model

### Process Control Block (PCB)

```c
typedef struct process {
    uint64_t pid;                    // Process ID
    uint64_t parent_pid;             // Parent process ID
    char name[256];                  // Process name

    // Memory management
    uint64_t *page_dir;              // Page directory pointer
    void *heap_start;                // Heap start address
    void *heap_end;                  // Current heap end

    // Scheduling
    enum proc_state state;           // READY, RUNNING, BLOCKED, ZOMBIE
    uint64_t priority;               // Scheduling priority
    uint64_t quantum;                // Time quantum remaining
    uint64_t cpu_time;               // Total CPU time used

    // Context
    struct cpu_context context;      // Saved registers

    // File descriptors
    struct file *fd_table[MAX_FDS];  // Open file descriptors

    // Threads
    struct thread *threads;          // List of threads
    uint64_t thread_count;

    // Linked list
    struct process *next;
    struct process *prev;
} process_t;
```

### Scheduling Strategy

- **Policy**: Priority-based round-robin with preemption
- **Quantum**: 10ms per task
- **Priority Levels**: 0 (highest) to 31 (lowest)
- **Scheduler Invocation**: Timer interrupt (100Hz)

## Memory Management

### Physical Memory Allocator

- **Algorithm**: Bitmap allocator (simple) or Buddy System (advanced)
- **Page Size**: 4KB
- **Tracking**: Bitmap where each bit represents one page
- **Functions**:
  - `pmm_alloc_page()` - Allocate single physical page
  - `pmm_free_page()` - Free physical page
  - `pmm_alloc_pages(n)` - Allocate n contiguous pages

### Virtual Memory (Paging)

- **Page Tables**: 4-level paging (PML4 → PDPT → PD → PT)
- **Page Size**: 4KB standard, 2MB large pages optional
- **TLB Management**: Explicit invalidation after page table changes
- **Functions**:
  - `vmm_map_page(virt, phys, flags)` - Map virtual to physical
  - `vmm_unmap_page(virt)` - Unmap virtual address
  - `vmm_create_address_space()` - Create new page directory

### Kernel Heap

- **Algorithm**: First-fit with block headers
- **Size**: Starts at 16MB, grows dynamically
- **Functions**: `kmalloc()`, `kfree()`, `krealloc()`

## Filesystem Design

### Virtual Filesystem (VFS) Layer

```c
struct vfs_node {
    char name[256];
    uint32_t inode;
    uint32_t type;        // FILE, DIRECTORY, DEVICE, etc.
    uint32_t permissions;
    uint32_t uid, gid;
    uint64_t size;

    // Operations
    int (*read)(struct vfs_node*, uint64_t offset, uint64_t size, void *buf);
    int (*write)(struct vfs_node*, uint64_t offset, uint64_t size, void *buf);
    struct vfs_node* (*readdir)(struct vfs_node*, uint32_t index);
    struct vfs_node* (*finddir)(struct vfs_node*, char *name);
    int (*open)(struct vfs_node*);
    void (*close)(struct vfs_node*);
};
```

### Concrete Filesystem: SimpleFS (Custom)

- **Block Size**: 4KB
- **Structure**:
  - Superblock (block 0): Magic, block count, inode count
  - Inode bitmap (blocks 1-N)
  - Data bitmap (blocks N+1 - M)
  - Inode table (blocks M+1 - K)
  - Data blocks (blocks K+1 - end)

### Alternative: ext2 Support (Phase 5B)

- Read-only initially, read-write later
- Standard ext2 structures

## Networking Stack

### Layer Architecture

```
┌─────────────────────────────────────┐
│     Application Layer               │
│   (Sockets, HTTP, DNS, etc.)        │
└─────────────────────────────────────┘
              │
┌─────────────────────────────────────┐
│     Transport Layer                 │
│        TCP  │  UDP                  │
└─────────────────────────────────────┘
              │
┌─────────────────────────────────────┐
│     Network Layer                   │
│         IPv4, ICMP, ARP             │
└─────────────────────────────────────┘
              │
┌─────────────────────────────────────┐
│     Link Layer                      │
│    Ethernet (e1000 driver)          │
└─────────────────────────────────────┘
```

### Packet Flow

1. **RX**: NIC → Driver → Ethernet → IP → TCP/UDP → Socket → App
2. **TX**: App → Socket → TCP/UDP → IP → Ethernet → Driver → NIC

### Network Device Driver

- **Target**: Intel e1000 (well-documented, QEMU support)
- **Buffer Management**: Ring buffers for RX and TX
- **Interrupts**: Handle packet arrival and transmission completion

## GUI Architecture

### Graphics Abstraction Layer

```c
struct framebuffer {
    uint32_t *addr;        // Linear framebuffer address
    uint32_t width;
    uint32_t height;
    uint32_t pitch;        // Bytes per scanline
    uint8_t bpp;           // Bits per pixel (32)
};

// Drawing primitives
void fb_putpixel(uint32_t x, uint32_t y, uint32_t color);
void fb_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void fb_draw_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color);
void fb_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg, uint32_t bg);
void fb_blit(uint32_t *src, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
```

### Window Manager

```c
struct window {
    uint32_t id;
    char title[64];
    int32_t x, y;
    uint32_t width, height;
    uint32_t *framebuffer;     // Window's own buffer
    bool visible;
    bool focused;
    uint32_t flags;            // RESIZABLE, CLOSABLE, etc.

    // Event handlers
    void (*on_paint)(struct window*);
    void (*on_mouse)(struct window*, int x, int y, int buttons);
    void (*on_key)(struct window*, int key);

    struct window *next;
};
```

### Compositor

- Manages window Z-order
- Handles window overlapping and clipping
- Composites all windows to final framebuffer
- Basic window decorations (title bar, borders, close button)

## Interrupt & Exception Handling

### IDT (Interrupt Descriptor Table)

- 256 entries
- Entries 0-31: CPU exceptions
- Entry 32: Timer (PIT)
- Entry 33: Keyboard
- Entry 128: Syscall (software interrupt)
- Entries 40-47: NIC interrupts

### Interrupt Service Routine Flow

```
Hardware Interrupt
       │
       v
CPU pushes SS, RSP, RFLAGS, CS, RIP
       │
       v
IDT Entry → ISR stub (assembly)
       │
       v
Save all registers (push rax, rbx, ..., r15)
       │
       v
Call C handler: interrupt_handler(int_num)
       │
       v
Restore all registers
       │
       v
IRETQ (return from interrupt)
```

## Syscall Interface

### Syscall Mechanism

- **Method**: `int 0x80` or `syscall` instruction
- **Calling Convention**:
  - Syscall number in `rax`
  - Args in `rdi`, `rsi`, `rdx`, `r10`, `r8`, `r9`
  - Return value in `rax`

### Core Syscalls

| Number | Name      | Description                    |
|--------|-----------|--------------------------------|
| 0      | read      | Read from file descriptor      |
| 1      | write     | Write to file descriptor       |
| 2      | open      | Open file                      |
| 3      | close     | Close file descriptor          |
| 4      | stat      | Get file status                |
| 5      | mmap      | Map memory                     |
| 6      | munmap    | Unmap memory                   |
| 7      | fork      | Create child process           |
| 8      | exec      | Execute program                |
| 9      | exit      | Terminate process              |
| 10     | wait      | Wait for child process         |
| 11     | getpid    | Get process ID                 |
| 12     | socket    | Create socket                  |
| 13     | bind      | Bind socket to address         |
| 14     | listen    | Listen for connections         |
| 15     | accept    | Accept connection              |
| 16     | connect   | Connect to remote socket       |
| 17     | send      | Send data on socket            |
| 18     | recv      | Receive data from socket       |

## Device Driver Model

### Driver Structure

```c
struct device_driver {
    char name[32];
    uint32_t type;  // BLOCK, CHAR, NETWORK

    int (*init)(void);
    int (*probe)(struct device*);
    int (*read)(void *buf, uint64_t offset, uint64_t size);
    int (*write)(void *buf, uint64_t offset, uint64_t size);
    int (*ioctl)(uint32_t cmd, void *arg);
    void (*irq_handler)(void);
};
```

### Driver Registry

- Maintains list of all registered drivers
- Device enumeration via PCI bus scan
- Automatic driver loading for detected devices

## Testing Strategy

### Unit Testing

- Kernel functions tested via dedicated test harness
- Mock hardware interfaces for driver testing

### Integration Testing

- Boot tests: Does it boot?
- Syscall tests: Can user programs interact correctly?
- Network tests: Can we ping? Send/receive packets?
- FS tests: Create, read, write, delete files

### Debugging Tools

- **Serial Port Logging**: All kernel messages to COM1
- **GDB Support**: QEMU `-s -S` flags for remote debugging
- **Kernel Assertions**: `kassert()` macro
- **Stack Traces**: On panic, dump call stack

## Performance Considerations

- **Zero-copy networking** where possible
- **Page cache** for filesystem
- **Lazy allocation** for heap and stack
- **DMA** for disk and network I/O
- **SMP support** (future): Multi-core scheduling

## Security Considerations

- **User/Kernel separation**: Ring 3 vs Ring 0
- **Page protections**: NX bit, read-only kernel pages
- **Syscall validation**: Check all user pointers
- **ASLR** (future): Address space layout randomization
- **Stack canaries** (future): Buffer overflow detection

---

This architecture provides a solid foundation for a functional, educational operating system. Each component is designed to be implementable incrementally, with clear interfaces between subsystems.
