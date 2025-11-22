/**
 * Process and Task Management
 *
 * Defines process structures and task management functions.
 */

#ifndef KERNEL_PROCESS_H
#define KERNEL_PROCESS_H

#include <stdint.h>
#include <stdbool.h>

// Maximum number of processes
#define MAX_PROCESSES 256

// Process states
typedef enum process_state {
    PROCESS_STATE_READY,        // Ready to run
    PROCESS_STATE_RUNNING,      // Currently running
    PROCESS_STATE_BLOCKED,      // Blocked (waiting for I/O, etc.)
    PROCESS_STATE_SLEEPING,     // Sleeping
    PROCESS_STATE_ZOMBIE,       // Terminated but not yet reaped
    PROCESS_STATE_DEAD          // Dead (can be freed)
} process_state_t;

// CPU context (saved during context switch)
typedef struct cpu_context {
    // General purpose registers
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;

    // Instruction pointer and flags
    uint64_t rip;
    uint64_t rflags;

    // Segment registers
    uint64_t cs, ss;
    uint64_t ds, es, fs, gs;
} __attribute__((packed)) cpu_context_t;

// Process Control Block (PCB)
typedef struct process {
    uint32_t pid;               // Process ID
    uint32_t parent_pid;        // Parent process ID
    char name[64];              // Process name

    // State
    process_state_t state;      // Current state
    int exit_code;              // Exit code (for zombies)

    // CPU context
    cpu_context_t context;      // Saved CPU state

    // Memory management
    uint64_t *page_directory;   // Page directory (CR3)
    uint64_t kernel_stack;      // Kernel stack pointer
    uint64_t user_stack;        // User stack pointer

    // Scheduling
    uint32_t priority;          // Priority level (0 = highest)
    uint32_t time_slice;        // Time slice in ticks
    uint32_t time_used;         // Time used in current slice
    uint64_t total_time;        // Total CPU time used

    // Sleep support
    uint64_t sleep_until;       // Wake up at this tick count

    // Linked list
    struct process *next;       // Next process in queue
    struct process *prev;       // Previous process in queue
} process_t;

/**
 * Initialize process management
 */
void process_init(void);

/**
 * Create a new kernel task
 *
 * @param entry Entry point function
 * @param name Task name
 * @param priority Priority (0 = highest)
 * @return Pointer to process structure, or NULL on failure
 */
process_t *process_create_kernel_task(void (*entry)(void), const char *name, uint32_t priority);

/**
 * Create a new user process
 *
 * @param entry Entry point address
 * @param name Process name
 * @param priority Priority (0 = highest)
 * @return Pointer to process structure, or NULL on failure
 */
process_t *process_create_user(uint64_t entry, const char *name, uint32_t priority);

/**
 * Exit current process
 *
 * @param exit_code Exit code
 */
void process_exit(int exit_code);

/**
 * Sleep current process
 *
 * @param ticks Number of ticks to sleep
 */
void process_sleep(uint64_t ticks);

/**
 * Wake up sleeping processes
 *
 * Called by timer interrupt to check for sleeping processes.
 */
void process_wakeup_sleeping(void);

/**
 * Get current process
 *
 * @return Pointer to current process, or NULL if none
 */
process_t *process_get_current(void);

/**
 * Get process by PID
 *
 * @param pid Process ID
 * @return Pointer to process, or NULL if not found
 */
process_t *process_get_by_pid(uint32_t pid);

/**
 * Kill a process
 *
 * @param pid Process ID
 * @return 0 on success, -1 on failure
 */
int process_kill(uint32_t pid);

/**
 * Print process list (for debugging)
 */
void process_list(void);

#endif // KERNEL_PROCESS_H
