/**
 * System Call Implementation
 *
 * Handles system calls from user mode programs.
 * Syscalls are invoked via "int 0x80" instruction.
 */

#include <kernel/syscall.h>
#include <kernel/isr.h>
#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <kernel/timer.h>
#include <kernel/vga.h>
#include <kernel/string.h>
#include <kernel/vfs.h>
#include <stdint.h>
#include <stddef.h>

// Syscall handler table
static syscall_handler_t syscall_table[SYSCALL_COUNT] = {0};

/**
 * Register a syscall handler
 */
void syscall_register(uint32_t num, syscall_handler_t handler) {
    if (num < SYSCALL_COUNT) {
        syscall_table[num] = handler;
    }
}

/**
 * System call dispatcher
 *
 * Called from interrupt 0x80.
 * Syscall number in rax, arguments in rdi, rsi, rdx, r10, r8, r9
 */
void syscall_dispatcher(registers_t *regs) {
    uint64_t syscall_num = regs->rax;

    // Check if syscall number is valid
    if (syscall_num >= SYSCALL_COUNT) {
        regs->rax = (uint64_t)-1;  // Return -1 for invalid syscall
        return;
    }

    // Check if handler is registered
    if (syscall_table[syscall_num] == NULL) {
        regs->rax = (uint64_t)-1;  // Return -1 for unimplemented syscall
        return;
    }

    // Call the syscall handler
    int64_t result = syscall_table[syscall_num](regs);

    // Store result in rax
    regs->rax = (uint64_t)result;
}

/**
 * sys_exit - Terminate current process
 *
 * Arguments:
 *   rdi = exit_code
 */
int64_t sys_exit(int exit_code) {
    process_exit(exit_code);
    return 0;  // Never actually returns
}

static int64_t sys_exit_handler(registers_t *regs) {
    return sys_exit((int)regs->rdi);
}

/**
 * sys_write - Write to file descriptor
 *
 * Arguments:
 *   rdi = fd (file descriptor)
 *   rsi = buf (buffer pointer)
 *   rdx = count (number of bytes)
 *
 * Returns: Number of bytes written, or -1 on error
 */
int64_t sys_write(int fd, const char *buf, size_t count) {
    // For now, only support stdout (fd 1) and stderr (fd 2)
    if (fd != 1 && fd != 2) {
        return -1;  // Invalid file descriptor
    }

    // TODO: Validate user pointer
    // For now, assume it's valid

    // Write to VGA console
    for (size_t i = 0; i < count; i++) {
        vga_putchar(buf[i]);
    }

    return (int64_t)count;
}

static int64_t sys_write_handler(registers_t *regs) {
    return sys_write((int)regs->rdi, (const char *)regs->rsi, (size_t)regs->rdx);
}

/**
 * sys_open - Open a file
 *
 * Arguments:
 *   rdi = path
 *   rsi = flags
 *
 * Returns: File descriptor, or -1 on error
 */
int64_t sys_open(const char *path, uint32_t flags) {
    // TODO: Validate user pointer
    if (!path) {
        return -1;
    }

    int fd = vfs_open(path, flags);
    return (int64_t)fd;
}

static int64_t sys_open_handler(registers_t *regs) {
    return sys_open((const char *)regs->rdi, (uint32_t)regs->rsi);
}

/**
 * sys_close - Close a file descriptor
 *
 * Arguments:
 *   rdi = fd
 *
 * Returns: 0 on success, -1 on error
 */
int64_t sys_close(int fd) {
    return (int64_t)vfs_close(fd);
}

static int64_t sys_close_handler(registers_t *regs) {
    return sys_close((int)regs->rdi);
}

/**
 * sys_read - Read from file descriptor
 *
 * Arguments:
 *   rdi = fd
 *   rsi = buf
 *   rdx = count
 *
 * Returns: Number of bytes read, or -1 on error
 */
int64_t sys_read(int fd, char *buf, size_t count) {
    // TODO: Validate user pointer
    if (!buf) {
        return -1;
    }

    // Use VFS read
    return (int64_t)vfs_read(fd, buf, count);
}

static int64_t sys_read_handler(registers_t *regs) {
    return sys_read((int)regs->rdi, (char *)regs->rsi, (size_t)regs->rdx);
}

/**
 * sys_getpid - Get process ID
 *
 * Returns: Current process ID
 */
int64_t sys_getpid(void) {
    process_t *current = process_get_current();
    if (current) {
        return (int64_t)current->pid;
    }
    return 0;
}

static int64_t sys_getpid_handler(registers_t *regs) {
    (void)regs;
    return sys_getpid();
}

/**
 * sys_sleep - Sleep for specified milliseconds
 *
 * Arguments:
 *   rdi = milliseconds
 */
int64_t sys_sleep(uint64_t ms) {
    if (ms == 0) return 0;

    // Convert ms to ticks (100Hz = 10ms per tick)
    uint64_t ticks = (ms + 9) / 10;  // Round up
    process_sleep(ticks);

    return 0;
}

static int64_t sys_sleep_handler(registers_t *regs) {
    return sys_sleep((uint64_t)regs->rdi);
}

/**
 * sys_yield - Voluntarily yield CPU
 */
int64_t sys_yield(void) {
    scheduler_yield();
    return 0;
}

static int64_t sys_yield_handler(registers_t *regs) {
    (void)regs;
    return sys_yield();
}

/**
 * sys_time - Get system uptime in milliseconds
 *
 * Returns: Uptime in milliseconds
 */
int64_t sys_time(void) {
    return (int64_t)timer_get_uptime_ms();
}

static int64_t sys_time_handler(registers_t *regs) {
    (void)regs;
    return sys_time();
}

/**
 * sys_putchar - Write single character to console
 *
 * Arguments:
 *   rdi = character
 */
int64_t sys_putchar(char c) {
    vga_putchar(c);
    return 0;
}

static int64_t sys_putchar_handler(registers_t *regs) {
    return sys_putchar((char)regs->rdi);
}

/**
 * sys_getchar - Read single character from keyboard
 *
 * Returns: Character code, or -1 if no input
 */
int64_t sys_getchar(void) {
    // TODO: Implement keyboard input buffer
    return -1;
}

static int64_t sys_getchar_handler(registers_t *regs) {
    (void)regs;
    return sys_getchar();
}

/**
 * Initialize system call subsystem
 */
void syscall_init(void) {
    // Clear syscall table
    for (uint32_t i = 0; i < SYSCALL_COUNT; i++) {
        syscall_table[i] = NULL;
    }

    // Register syscall handlers
    syscall_register(SYS_EXIT, sys_exit_handler);
    syscall_register(SYS_WRITE, sys_write_handler);
    syscall_register(SYS_READ, sys_read_handler);
    syscall_register(SYS_OPEN, sys_open_handler);
    syscall_register(SYS_CLOSE, sys_close_handler);
    syscall_register(SYS_GETPID, sys_getpid_handler);
    syscall_register(SYS_SLEEP, sys_sleep_handler);
    syscall_register(SYS_YIELD, sys_yield_handler);
    syscall_register(SYS_TIME, sys_time_handler);
    syscall_register(SYS_PUTCHAR, sys_putchar_handler);
    syscall_register(SYS_GETCHAR, sys_getchar_handler);

    // Register syscall dispatcher with interrupt 0x80
    isr_register_handler(0x80, syscall_dispatcher);

    vga_printf("  Syscall: Registered %u system calls\n", SYSCALL_COUNT);
}
