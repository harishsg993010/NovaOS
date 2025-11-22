/**
 * System Call Interface
 *
 * Defines the system call numbers and syscall handler.
 * User programs invoke syscalls via "int 0x80" instruction.
 */

#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#include <stdint.h>
#include <kernel/isr.h>

// System call numbers
#define SYS_EXIT        0   // Exit process
#define SYS_WRITE       1   // Write to file descriptor
#define SYS_READ        2   // Read from file descriptor
#define SYS_OPEN        3   // Open file
#define SYS_CLOSE       4   // Close file descriptor
#define SYS_GETPID      5   // Get process ID
#define SYS_SLEEP       6   // Sleep for milliseconds
#define SYS_YIELD       7   // Yield CPU to another process
#define SYS_FORK        8   // Create child process (future)
#define SYS_EXEC        9   // Execute program (future)
#define SYS_WAIT        10  // Wait for child process (future)
#define SYS_MALLOC      11  // Allocate memory (user heap)
#define SYS_FREE        12  // Free memory (user heap)
#define SYS_TIME        13  // Get current time
#define SYS_GETCHAR     14  // Get character from keyboard
#define SYS_PUTCHAR     15  // Put character to console

#define SYSCALL_COUNT   16  // Total number of syscalls

/**
 * System call handler function type
 *
 * @param regs CPU register state at syscall time
 * @return Return value (placed in rax)
 */
typedef int64_t (*syscall_handler_t)(registers_t *regs);

/**
 * Initialize system call subsystem
 */
void syscall_init(void);

/**
 * Register a system call handler
 *
 * @param num Syscall number
 * @param handler Handler function
 */
void syscall_register(uint32_t num, syscall_handler_t handler);

/**
 * Main syscall dispatcher (called from ISR 0x80)
 *
 * @param regs CPU register state
 */
void syscall_dispatcher(registers_t *regs);

// Individual syscall handlers
int64_t sys_exit(int exit_code);
int64_t sys_write(int fd, const char *buf, size_t count);
int64_t sys_read(int fd, char *buf, size_t count);
int64_t sys_getpid(void);
int64_t sys_sleep(uint64_t ms);
int64_t sys_yield(void);
int64_t sys_time(void);
int64_t sys_putchar(char c);
int64_t sys_getchar(void);

#endif // KERNEL_SYSCALL_H
