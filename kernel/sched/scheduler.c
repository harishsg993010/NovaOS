/**
 * Process Scheduler Implementation
 *
 * Implements preemptive multitasking with round-robin scheduling.
 */

#include <kernel/scheduler.h>
#include <kernel/process.h>
#include <kernel/timer.h>
#include <kernel/isr.h>
#include <kernel/idt.h>
#include <kernel/vga.h>
#include <kernel/string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// External functions from process.c
extern void _process_set_current(process_t *proc);
extern process_t **_process_get_table(void);
extern uint32_t _process_get_max(void);

// External context switch function (implemented in assembly)
extern void switch_context(cpu_context_t *old, cpu_context_t *new);

// Scheduler state
static sched_algorithm_t algorithm = SCHED_ROUND_ROBIN;
static bool scheduler_running = false;
static uint32_t total_switches = 0;

// Ready queue (simple circular queue for round-robin)
static process_t *ready_queue_head = NULL;
static process_t *ready_queue_tail = NULL;
static uint32_t ready_count = 0;

/**
 * Add process to ready queue
 */
void scheduler_add_process(process_t *process) {
    if (!process) return;

    process->next = NULL;
    process->prev = NULL;

    if (ready_queue_head == NULL) {
        // First process in queue
        ready_queue_head = process;
        ready_queue_tail = process;
    } else {
        // Add to tail
        ready_queue_tail->next = process;
        process->prev = ready_queue_tail;
        ready_queue_tail = process;
    }

    ready_count++;
    process->state = PROCESS_STATE_READY;
}

/**
 * Remove process from ready queue
 */
void scheduler_remove_process(process_t *process) {
    if (!process) return;

    if (process->prev) {
        process->prev->next = process->next;
    } else {
        ready_queue_head = process->next;
    }

    if (process->next) {
        process->next->prev = process->prev;
    } else {
        ready_queue_tail = process->prev;
    }

    process->next = NULL;
    process->prev = NULL;
    ready_count--;
}

/**
 * Get next process to run (round-robin)
 */
static process_t *scheduler_pick_next(void) {
    // Wake up sleeping processes
    process_wakeup_sleeping();

    // Round-robin: pick head of ready queue
    if (ready_queue_head == NULL) {
        return NULL;
    }

    process_t *next = ready_queue_head;

    // Move to tail (circular queue)
    scheduler_remove_process(next);
    scheduler_add_process(next);

    return next;
}

/**
 * Schedule next process
 *
 * Called by timer interrupt.
 */
void scheduler_schedule(registers_t *regs) {
    if (!scheduler_running) return;

    process_t *current = process_get_current();
    process_t *next = scheduler_pick_next();

    // No process to run
    if (!next) {
        return;
    }

    // Same process, just continue
    if (current == next) {
        return;
    }

    // Save current process state if any
    if (current) {
        // Save CPU context from interrupt frame
        current->context.rax = regs->rax;
        current->context.rbx = regs->rbx;
        current->context.rcx = regs->rcx;
        current->context.rdx = regs->rdx;
        current->context.rsi = regs->rsi;
        current->context.rdi = regs->rdi;
        current->context.rbp = regs->rbp;
        current->context.rsp = regs->rsp;
        current->context.r8 = regs->r8;
        current->context.r9 = regs->r9;
        current->context.r10 = regs->r10;
        current->context.r11 = regs->r11;
        current->context.r12 = regs->r12;
        current->context.r13 = regs->r13;
        current->context.r14 = regs->r14;
        current->context.r15 = regs->r15;
        current->context.rip = regs->rip;
        current->context.rflags = regs->rflags;
        current->context.cs = regs->cs;
        current->context.ss = regs->ss;

        // Update state
        if (current->state == PROCESS_STATE_RUNNING) {
            current->state = PROCESS_STATE_READY;
        }

        // Reset time slice
        current->time_used = 0;
    }

    // Switch to next process
    next->state = PROCESS_STATE_RUNNING;
    next->total_time++;
    _process_set_current(next);
    total_switches++;

    // Restore next process context to interrupt frame
    regs->rax = next->context.rax;
    regs->rbx = next->context.rbx;
    regs->rcx = next->context.rcx;
    regs->rdx = next->context.rdx;
    regs->rsi = next->context.rsi;
    regs->rdi = next->context.rdi;
    regs->rbp = next->context.rbp;
    regs->rsp = next->context.rsp;
    regs->r8 = next->context.r8;
    regs->r9 = next->context.r9;
    regs->r10 = next->context.r10;
    regs->r11 = next->context.r11;
    regs->r12 = next->context.r12;
    regs->r13 = next->context.r13;
    regs->r14 = next->context.r14;
    regs->r15 = next->context.r15;
    regs->rip = next->context.rip;
    regs->rflags = next->context.rflags;
    regs->cs = next->context.cs;
    regs->ss = next->context.ss;

    // Switch page directory if needed
    if (next->page_directory) {
        uint64_t current_cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(current_cr3));
        uint64_t next_cr3 = (uint64_t)next->page_directory;

        if (current_cr3 != next_cr3) {
            __asm__ volatile("mov %0, %%cr3" :: "r"(next_cr3) : "memory");
        }
    }
}

/**
 * Timer callback for scheduling
 */
static void scheduler_timer_callback(void) {
    // This will be called from timer interrupt handler
    // The actual scheduling happens in the IRQ handler
}

/**
 * Initialize scheduler
 */
void scheduler_init(sched_algorithm_t algo) {
    algorithm = algo;
    scheduler_running = false;
    ready_queue_head = NULL;
    ready_queue_tail = NULL;
    ready_count = 0;
    total_switches = 0;

    // Register scheduler with timer interrupt
    isr_register_handler(32, scheduler_schedule);  // IRQ0 = interrupt 32

    const char *algo_name = "Unknown";
    switch (algorithm) {
        case SCHED_ROUND_ROBIN: algo_name = "Round-Robin"; break;
        case SCHED_PRIORITY: algo_name = "Priority"; break;
        case SCHED_PRIORITY_RR: algo_name = "Priority Round-Robin"; break;
    }

    vga_printf("  Scheduler: Initialized (%s)\n", algo_name);
}

/**
 * Start scheduling
 */
void scheduler_start(void) {
    scheduler_running = true;
    vga_printf("  Scheduler: Started\n");
}

/**
 * Stop scheduling
 */
void scheduler_stop(void) {
    scheduler_running = false;
    vga_printf("  Scheduler: Stopped\n");
}

/**
 * Yield CPU to another process
 */
void scheduler_yield(void) {
    // Trigger timer interrupt to force rescheduling
    __asm__ volatile("int $32");
}

/**
 * Block current process
 */
void scheduler_block(void) {
    process_t *current = process_get_current();
    if (current) {
        current->state = PROCESS_STATE_BLOCKED;
        scheduler_remove_process(current);
        scheduler_yield();
    }
}

/**
 * Unblock a process
 */
void scheduler_unblock(process_t *process) {
    if (process && process->state == PROCESS_STATE_BLOCKED) {
        process->state = PROCESS_STATE_READY;
        scheduler_add_process(process);
    }
}

/**
 * Get number of ready processes
 */
uint32_t scheduler_get_ready_count(void) {
    return ready_count;
}

/**
 * Get total number of processes
 */
uint32_t scheduler_get_total_count(void) {
    uint32_t count = 0;
    process_t **table = _process_get_table();
    uint32_t max = _process_get_max();

    for (uint32_t i = 0; i < max; i++) {
        if (table[i] != NULL) {
            count++;
        }
    }

    return count;
}

/**
 * Print scheduler statistics
 */
void scheduler_print_stats(void) {
    vga_printf("\nScheduler Statistics:\n");
    vga_printf("  Algorithm:      %s\n",
               algorithm == SCHED_ROUND_ROBIN ? "Round-Robin" : "Other");
    vga_printf("  Running:        %s\n", scheduler_running ? "Yes" : "No");
    vga_printf("  Ready processes: %u\n", ready_count);
    vga_printf("  Total processes: %u\n", scheduler_get_total_count());
    vga_printf("  Context switches: %u\n", total_switches);
    vga_printf("  Uptime:         %u ms\n", (uint32_t)timer_get_uptime_ms());
    vga_puts("\n");
}

/**
 * Check if scheduler is running
 */
bool scheduler_is_running(void) {
    return scheduler_running;
}
