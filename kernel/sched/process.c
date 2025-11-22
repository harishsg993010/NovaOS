/**
 * Process Management Implementation
 */

#include <kernel/process.h>
#include <kernel/pmm.h>
#include <kernel/vmm.h>
#include <kernel/heap.h>
#include <kernel/string.h>
#include <kernel/vga.h>
#include <stdint.h>
#include <stddef.h>

// Process table
static process_t *process_table[MAX_PROCESSES] = {0};
static uint32_t next_pid = 1;
static uint32_t process_count = 0;

// Current running process
static process_t *current_process = NULL;

/**
 * Initialize process management
 */
void process_init(void) {
    memset(process_table, 0, sizeof(process_table));
    next_pid = 1;
    process_count = 0;
    current_process = NULL;

    vga_printf("  Process: Initialized (max %u processes)\n", MAX_PROCESSES);
}

/**
 * Allocate a new PID
 */
static uint32_t alloc_pid(void) {
    return next_pid++;
}

/**
 * Add process to table
 */
static int add_process(process_t *proc) {
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i] == NULL) {
            process_table[i] = proc;
            process_count++;
            return 0;
        }
    }
    return -1;  // Table full
}

/**
 * Remove process from table
 */
static void remove_process(process_t *proc) {
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i] == proc) {
            process_table[i] = NULL;
            process_count--;
            return;
        }
    }
}

/**
 * Create a new kernel task
 */
process_t *process_create_kernel_task(void (*entry)(void), const char *name, uint32_t priority) {
    // Allocate process structure
    process_t *proc = (process_t *)kzalloc(sizeof(process_t));
    if (!proc) return NULL;

    // Set up process info
    proc->pid = alloc_pid();
    proc->parent_pid = current_process ? current_process->pid : 0;
    strncpy(proc->name, name, sizeof(proc->name) - 1);
    proc->state = PROCESS_STATE_READY;
    proc->priority = priority;
    proc->time_slice = 10;  // 10 ticks default
    proc->time_used = 0;
    proc->total_time = 0;

    // Allocate kernel stack (16KB)
    uint64_t stack_phys = pmm_alloc_pages(4);
    if (!stack_phys) {
        kfree(proc);
        return NULL;
    }

    uint64_t stack_virt = vmm_phys_to_virt(stack_phys);
    proc->kernel_stack = stack_virt + (4 * PAGE_SIZE);  // Stack grows down

    // Set up initial context
    memset(&proc->context, 0, sizeof(cpu_context_t));
    proc->context.rip = (uint64_t)entry;
    proc->context.rsp = proc->kernel_stack;
    proc->context.rflags = 0x202;  // IF = 1 (interrupts enabled)
    proc->context.cs = 0x08;       // Kernel code segment
    proc->context.ss = 0x10;       // Kernel data segment

    // Use current page directory (kernel tasks share kernel space)
    proc->page_directory = (uint64_t *)vmm_get_current_page_directory();

    // Add to process table
    if (add_process(proc) != 0) {
        pmm_free_pages(stack_phys, 4);
        kfree(proc);
        return NULL;
    }

    return proc;
}

/**
 * Create a new user mode process
 */
process_t *process_create_user(uint64_t entry, const char *name, uint32_t priority) {
    // Allocate process structure
    process_t *proc = (process_t *)kzalloc(sizeof(process_t));
    if (!proc) return NULL;

    // Set up process info
    proc->pid = alloc_pid();
    proc->parent_pid = current_process ? current_process->pid : 0;
    strncpy(proc->name, name, sizeof(proc->name) - 1);
    proc->state = PROCESS_STATE_READY;
    proc->priority = priority;
    proc->time_slice = 10;
    proc->time_used = 0;
    proc->total_time = 0;

    // Allocate kernel stack (16KB)
    uint64_t kstack_phys = pmm_alloc_pages(4);
    if (!kstack_phys) {
        kfree(proc);
        return NULL;
    }

    uint64_t kstack_virt = vmm_phys_to_virt(kstack_phys);
    proc->kernel_stack = kstack_virt + (4 * PAGE_SIZE);

    // Allocate user stack (16KB)
    uint64_t ustack_phys = pmm_alloc_pages(4);
    if (!ustack_phys) {
        pmm_free_pages(kstack_phys, 4);
        kfree(proc);
        return NULL;
    }

    // User stack is at fixed virtual address: 0x7FFFFFFFFFF0 (just below 128TB)
    uint64_t ustack_virt = 0x7FFFFFFFFFF0ULL;
    proc->user_stack = ustack_virt;

    // Create new page directory for user process
    uint64_t pml4_phys = vmm_create_address_space();
    if (!pml4_phys) {
        pmm_free_pages(kstack_phys, 4);
        pmm_free_pages(ustack_phys, 4);
        kfree(proc);
        return NULL;
    }
    proc->page_directory = (uint64_t *)pml4_phys;

    // Map user stack into process address space
    // We need to temporarily switch to the new page directory to map user pages
    uint64_t old_cr3 = vmm_get_current_page_directory();
    vmm_switch_page_directory(pml4_phys);

    // Map user stack pages
    for (int i = 0; i < 4; i++) {
        uint64_t virt = ustack_virt - (4 * PAGE_SIZE) + (i * PAGE_SIZE);
        uint64_t phys = ustack_phys + (i * PAGE_SIZE);
        vmm_map_page(virt, phys, PAGE_FLAGS_USER);
    }

    // Allocate and map user code pages at fixed user address 0x400000 (4MB)
    uint64_t user_code_virt = 0x400000;
    uint64_t user_code_phys = pmm_alloc_pages(4);  // 16KB for code
    if (!user_code_phys) {
        vmm_switch_page_directory(old_cr3);
        vmm_destroy_address_space(pml4_phys);
        pmm_free_pages(kstack_phys, 4);
        pmm_free_pages(ustack_phys, 4);
        kfree(proc);
        return NULL;
    }

    // Map user code pages
    for (int i = 0; i < 4; i++) {
        uint64_t virt = user_code_virt + (i * PAGE_SIZE);
        uint64_t phys = user_code_phys + (i * PAGE_SIZE);
        vmm_map_page(virt, phys, PAGE_FLAGS_USER);
    }

    // Switch back to original page directory to copy code
    vmm_switch_page_directory(old_cr3);

    // Copy user code from kernel space to user code pages
    void *user_code_kernel_ptr = (void *)vmm_phys_to_virt(user_code_phys);
    memcpy(user_code_kernel_ptr, (void *)entry, 16 * 1024);  // Copy 16KB

    // Set up initial context for user mode
    memset(&proc->context, 0, sizeof(cpu_context_t));
    proc->context.rip = user_code_virt;  // Execute from user code space, not kernel!
    proc->context.rsp = ustack_virt;
    proc->context.rflags = 0x202;  // IF = 1 (interrupts enabled)
    proc->context.cs = 0x1B;       // User code segment (GDT index 3, RPL=3)
    proc->context.ss = 0x23;       // User data segment (GDT index 4, RPL=3)

    // Add to process table
    if (add_process(proc) != 0) {
        vmm_destroy_address_space(pml4_phys);
        pmm_free_pages(kstack_phys, 4);
        pmm_free_pages(ustack_phys, 4);
        kfree(proc);
        return NULL;
    }

    return proc;
}

/**
 * Exit current process
 */
void process_exit(int exit_code) {
    if (!current_process) return;

    current_process->state = PROCESS_STATE_ZOMBIE;
    current_process->exit_code = exit_code;

    // TODO: Free resources
    // TODO: Wake up parent if waiting

    // Yield to scheduler
    __asm__ volatile("int $0x20");  // Force timer interrupt

    // Should never reach here
    while (1) __asm__ volatile("hlt");
}

/**
 * Sleep current process
 */
void process_sleep(uint64_t ticks) {
    if (!current_process) return;

    extern uint64_t timer_get_ticks(void);
    current_process->sleep_until = timer_get_ticks() + ticks;
    current_process->state = PROCESS_STATE_SLEEPING;

    // Yield to scheduler
    __asm__ volatile("int $0x20");
}

/**
 * Wake up sleeping processes
 */
void process_wakeup_sleeping(void) {
    extern uint64_t timer_get_ticks(void);
    uint64_t now = timer_get_ticks();

    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        process_t *proc = process_table[i];
        if (proc && proc->state == PROCESS_STATE_SLEEPING) {
            if (now >= proc->sleep_until) {
                proc->state = PROCESS_STATE_READY;
            }
        }
    }
}

/**
 * Get current process
 */
process_t *process_get_current(void) {
    return current_process;
}

/**
 * Get process by PID
 */
process_t *process_get_by_pid(uint32_t pid) {
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i] && process_table[i]->pid == pid) {
            return process_table[i];
        }
    }
    return NULL;
}

/**
 * Kill a process
 */
int process_kill(uint32_t pid) {
    process_t *proc = process_get_by_pid(pid);
    if (!proc) return -1;

    proc->state = PROCESS_STATE_DEAD;
    return 0;
}

/**
 * Print process list
 */
void process_list(void) {
    vga_printf("\nProcess List:\n");
    vga_printf("PID   NAME                 STATE       PRIORITY  TIME\n");
    vga_printf("----  -------------------  ----------  --------  ----\n");

    const char *state_names[] = {
        "READY", "RUNNING", "BLOCKED", "SLEEPING", "ZOMBIE", "DEAD"
    };

    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        process_t *proc = process_table[i];
        if (proc) {
            vga_printf("%-4u  %-19s  %-10s  %8u  %4u\n",
                       proc->pid,
                       proc->name,
                       state_names[proc->state],
                       proc->priority,
                       (uint32_t)proc->total_time);
        }
    }
    vga_printf("\n");
}

// Internal function used by scheduler
void _process_set_current(process_t *proc) {
    current_process = proc;
}

process_t **_process_get_table(void) {
    return process_table;
}

uint32_t _process_get_max(void) {
    return MAX_PROCESSES;
}
