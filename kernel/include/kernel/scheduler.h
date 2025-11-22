/**
 * Process Scheduler
 *
 * Implements preemptive multitasking with priority-based round-robin scheduling.
 */

#ifndef KERNEL_SCHEDULER_H
#define KERNEL_SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>
#include <kernel/process.h>
#include <kernel/isr.h>

// Scheduling algorithms
typedef enum sched_algorithm {
    SCHED_ROUND_ROBIN,          // Round robin
    SCHED_PRIORITY,             // Priority-based
    SCHED_PRIORITY_RR           // Priority-based round robin
} sched_algorithm_t;

/**
 * Initialize scheduler
 *
 * @param algorithm Scheduling algorithm to use
 */
void scheduler_init(sched_algorithm_t algorithm);

/**
 * Start scheduling
 *
 * Enables timer interrupts and begins multitasking.
 */
void scheduler_start(void);

/**
 * Stop scheduling
 *
 * Disables timer interrupts and stops multitasking.
 */
void scheduler_stop(void);

/**
 * Add process to ready queue
 *
 * @param process Process to add
 */
void scheduler_add_process(process_t *process);

/**
 * Remove process from ready queue
 *
 * @param process Process to remove
 */
void scheduler_remove_process(process_t *process);

/**
 * Schedule next process
 *
 * Called by timer interrupt to select next process to run.
 * Performs context switch if needed.
 *
 * @param regs Current CPU register state
 */
void scheduler_schedule(registers_t *regs);

/**
 * Yield CPU to another process
 *
 * Voluntarily give up CPU time slice.
 */
void scheduler_yield(void);

/**
 * Block current process
 *
 * Mark current process as blocked (waiting for I/O, etc.)
 */
void scheduler_block(void);

/**
 * Unblock a process
 *
 * @param process Process to unblock
 */
void scheduler_unblock(process_t *process);

/**
 * Get number of ready processes
 *
 * @return Number of processes in ready queue
 */
uint32_t scheduler_get_ready_count(void);

/**
 * Get total number of processes
 *
 * @return Total number of processes (all states)
 */
uint32_t scheduler_get_total_count(void);

/**
 * Print scheduler statistics
 */
void scheduler_print_stats(void);

/**
 * Check if scheduler is running
 *
 * @return true if scheduler is active
 */
bool scheduler_is_running(void);

#endif // KERNEL_SCHEDULER_H
