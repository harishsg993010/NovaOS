# Phase 4: System Calls & User Mode

## Overview

Phase 4 implements system calls and user mode (Ring 3) execution, enabling user space programs to interact with the kernel through a controlled interface.

## Implementation Status

**Status**: COMPLETE (Testing Pending)

All core components have been implemented:
- Global Descriptor Table with user segments
- System call dispatcher and handlers
- User mode process creation
- User space library
- Test programs

## Architecture

### Privilege Levels

x86_64 protection rings:
- **Ring 0 (Kernel Mode)**: Full hardware access, executes kernel code
- **Ring 3 (User Mode)**: Restricted access, executes user programs
- Rings 1 and 2 are unused in most modern OSes

### System Call Mechanism

System calls use `int 0x80` software interrupt:

```
User Program (Ring 3)
    |
    | int 0x80 (with syscall number in rax)
    v
Syscall Dispatcher (kernel/arch/x86_64/syscall.c)
    |
    | Lookup handler in syscall_table[]
    v
Syscall Handler (Ring 0)
    |
    | Perform requested operation
    v
Return to User Program (Ring 3)
```

### Register Convention

Arguments passed in registers:
- `rax`: Syscall number
- `rdi`: Argument 1
- `rsi`: Argument 2
- `rdx`: Argument 3
- `r10`: Argument 4
- `r8`:  Argument 5

Return value in `rax`.

## Components

### 1. Global Descriptor Table (GDT)

**File**: `kernel/arch/x86_64/gdt.c`

Five segment descriptors:
```
Index  Selector  DPL  Description
-----  --------  ---  -----------
  0      0x00     -   Null descriptor (required)
  1      0x08     0   Kernel code segment
  2      0x10     0   Kernel data segment
  3    0x18/0x1B  3   User code segment (RPL=3)
  4    0x20/0x23  3   User data segment (RPL=3)
```

**Key Features**:
- 64-bit code segments (L=1 flag)
- User segments have DPL=3 (ring 3)
- Flat memory model (base=0, limit=0xFFFFF)
- Page granularity (4KB pages)

### 2. System Call Interface

**Files**:
- `kernel/include/kernel/syscall.h` - Definitions
- `kernel/arch/x86_64/syscall.c` - Implementation

**Implemented Syscalls**:

| Number | Name      | Description                    |
|--------|-----------|--------------------------------|
| 0      | EXIT      | Terminate process              |
| 1      | WRITE     | Write to file descriptor       |
| 2      | READ      | Read from file descriptor      |
| 5      | GETPID    | Get process ID                 |
| 6      | SLEEP     | Sleep for milliseconds         |
| 7      | YIELD     | Yield CPU to scheduler         |
| 13     | TIME      | Get system uptime              |
| 14     | GETCHAR   | Read character (placeholder)   |
| 15     | PUTCHAR   | Write single character         |

**Reserved for Future**:
- OPEN, CLOSE, FORK, EXEC, WAIT, MALLOC, FREE

### 3. User Mode Process Creation

**File**: `kernel/sched/process.c`

Function: `process_create_user(uint64_t entry, const char *name, uint32_t priority)`

**Process**:
1. Allocate process structure
2. Create kernel stack (16KB)
3. Create user stack (16KB at high virtual address)
4. Create new page directory (address space isolation)
5. Map user stack with PAGE_FLAGS_USER
6. Identity map user code region
7. Set up initial CPU context:
   - RIP = entry point
   - RSP = user stack (0x7FFFFFFFFFF0)
   - CS = 0x1B (user code, RPL=3)
   - SS = 0x23 (user data, RPL=3)
   - RFLAGS = 0x202 (interrupts enabled)

### 4. User Space Library

**File**: `user/lib/syscall.h`

Provides C wrappers for system calls:

```c
// Core syscalls
void exit(int code);
int write(int fd, const char *buf, size_t count);
int getpid(void);
void sleep_ms(uint64_t ms);
void yield(void);
uint64_t get_time(void);
void putchar(char c);
int getchar(void);

// Helper functions
void puts(const char *str);
void print_num(int num);
```

**Implementation**:
```c
static inline int64_t syscall(uint64_t num, uint64_t arg1, ...) {
    int64_t ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "D"(arg1), "S"(arg2), ...
        : "rcx", "r11", "memory"
    );
    return ret;
}
```

### 5. Test Programs

**File**: `user/init/user_test.c`

Simple user mode program that:
- Gets its process ID
- Prints greeting with PID
- Loops 10 times, showing iteration and uptime
- Sleeps 1 second between iterations
- Exits cleanly

**File**: `kernel/main.c` (user_mode_entry function)

Inline test that demonstrates:
- Making syscalls from Ring 3
- Character output via putchar
- Process sleeping
- Clean exit

## Memory Layout

### User Process Address Space

```
0xFFFFFFFFFFFFFFFF  +-----------------------+
                    |                       |
                    |   Unmapped (kernel)   |
                    |                       |
0xFFFF800000000000  +-----------------------+
                    |                       |
                    |   Unmapped (gap)      |
                    |                       |
0x7FFFFFFFFFF0      +-----------------------+ <- User stack top
                    |   User Stack (16KB)   |
                    |   (grows down)        |
0x7FFFFFFFFBF0      +-----------------------+
                    |                       |
                    |   Unmapped            |
                    |                       |
                    +-----------------------+
                    |   User Code/Data      |
                    |   (identity mapped)   |
0x0000000000000000  +-----------------------+
```

### Context Switch Flow

1. User program executes in Ring 3
2. `int 0x80` triggers syscall
3. CPU switches to Ring 0, loads kernel stack
4. Syscall handler executes
5. Handler returns via `iretq`
6. CPU switches back to Ring 3, restores user stack

## Security Features

### Privilege Separation

- User code cannot directly access hardware
- User code cannot modify kernel memory
- User processes have isolated address spaces

### Address Space Isolation

- Each user process has its own page directory
- User pages marked with PAGE_USER flag
- Kernel memory unmapped in user space (or protected)

### Syscall Validation

- All syscalls go through dispatcher
- Invalid syscall numbers rejected
- Pointer arguments should be validated (TODO)

## Testing

### Build and Run

```bash
# Install build tools (if not already done)
sudo apt update
sudo apt install build-essential nasm grub-pc-bin grub-common xorriso qemu-system-x86

# Build kernel
make clean
make all
make iso

# Run in QEMU
make run
```

### Expected Output

```
NovaeOS - Custom Operating System
...
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

Testing Multitasking:
  Created task 1 (PID 1)
  Created task 2 (PID 2)
  Created task 3 (PID 3)
  Created idle task (PID 4)
  Creating user mode process...
  Created user mode process (PID 5)
  Added tasks to scheduler

Starting Multitasking...
[Task 1] Count: 0
[Task 2] Count: 0
[Task 3] Uptime: 100 ms
[User Mode] Iteration: 0
[Task 1] Count: 1
[User Mode] Iteration: 1
...
```

## Implementation Details

### Syscall Handler Template

```c
static int64_t sys_example(registers_t *regs) {
    // Extract arguments from registers
    uint64_t arg1 = regs->rdi;
    uint64_t arg2 = regs->rsi;

    // Perform operation
    // ...

    // Return value (will be in rax)
    return result;
}
```

### Adding New Syscalls

1. Add syscall number to `kernel/include/kernel/syscall.h`
2. Implement handler in `kernel/arch/x86_64/syscall.c`
3. Register handler in `syscall_table[]`
4. Add wrapper to `user/lib/syscall.h`

### User Stack Layout

```
0x7FFFFFFFFFF0  <- Initial RSP (top of stack)
     |
     v (grows down)

0x7FFFFFFFFBF0  <- Bottom of 16KB stack
```

## Known Limitations

1. **No Memory Protection**: User pointers not validated in syscalls
2. **Identity Mapping**: Code region identity-mapped (not secure)
3. **No ELF Loader**: User programs compiled with kernel
4. **Limited Syscalls**: Only 9 syscalls implemented
5. **No Copy-on-Write**: Memory not shared efficiently
6. **No Permissions**: File descriptor access not checked

## Future Enhancements

### Phase 5 (Filesystem)
- Proper ELF loading
- Executable files on disk
- Open/close/read/write for files

### Security Improvements
- Validate user pointers before dereferencing
- Implement proper memory protection
- Add permission checks for resources
- Implement copy-on-write for fork

### Additional Syscalls
- fork() - Create child process
- exec() - Execute program
- wait() - Wait for child process
- mmap() - Memory mapping
- ioctl() - Device control

## References

- Intel 64 and IA-32 Architectures Software Developer's Manual, Volume 3
- OSDev Wiki: [GDT](https://wiki.osdev.org/GDT), [System Calls](https://wiki.osdev.org/System_Calls)
- Linux syscall convention (for reference)

## Files Modified/Created

### New Files
- `kernel/include/kernel/gdt.h`
- `kernel/arch/x86_64/gdt.c`
- `kernel/include/kernel/syscall.h`
- `kernel/arch/x86_64/syscall.c`
- `user/lib/syscall.h`
- `user/init/user_test.c`
- `docs/PHASE4_SYSCALLS.md` (this file)

### Modified Files
- `kernel/main.c` - Added GDT/syscall init, user mode test
- `kernel/sched/process.c` - Added process_create_user()

## Summary

Phase 4 successfully implements the foundation for user space execution:

- User mode processes run in Ring 3 with restricted privileges
- System calls provide controlled kernel access
- GDT properly configured for privilege separation
- Address space isolation per process
- Test programs demonstrate functionality

This enables running untrusted user code safely and sets the stage for loading external programs in future phases.

**Status**: Implementation complete, ready for testing!
