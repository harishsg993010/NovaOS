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
 * Helper: Manually map a page in a specific page directory
 * (without switching CR3)
 */
static void manual_map_page(uint64_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags) {
    // Extract indices
    uint32_t pml4_idx = (virt >> 39) & 0x1FF;
    uint32_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint32_t pd_idx = (virt >> 21) & 0x1FF;
    uint32_t pt_idx = (virt >> 12) & 0x1FF;

    // Get or create PDPT
    if (!(pml4[pml4_idx] & PAGE_PRESENT)) {
        uint64_t pdpt_phys = pmm_alloc_page();
        memset((void *)vmm_phys_to_virt(pdpt_phys), 0, PAGE_SIZE);
        pml4[pml4_idx] = pdpt_phys | PAGE_FLAGS_USER;
    }
    uint64_t *pdpt = (uint64_t *)vmm_phys_to_virt(pml4[pml4_idx] & ~0xFFF);

    // Get or create PD
    if (!(pdpt[pdpt_idx] & PAGE_PRESENT)) {
        uint64_t pd_phys = pmm_alloc_page();
        memset((void *)vmm_phys_to_virt(pd_phys), 0, PAGE_SIZE);
        pdpt[pdpt_idx] = pd_phys | PAGE_FLAGS_USER;
    }
    uint64_t *pd = (uint64_t *)vmm_phys_to_virt(pdpt[pdpt_idx] & ~0xFFF);

    // Get or create PT
    if (!(pd[pd_idx] & PAGE_PRESENT)) {
        uint64_t pt_phys = pmm_alloc_page();
        memset((void *)vmm_phys_to_virt(pt_phys), 0, PAGE_SIZE);
        pd[pd_idx] = pt_phys | PAGE_FLAGS_USER;
    }
    uint64_t *pt = (uint64_t *)vmm_phys_to_virt(pd[pd_idx] & ~0xFFF);

    // Map the page
    pt[pt_idx] = phys | flags | PAGE_PRESENT;
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

    // User stack: Use PML4[1] range (512GB+) to avoid conflict with kernel PML4[0]
    // PML4[0] is reserved for kernel identity mapping (0-4MB)
    uint64_t ustack_virt_base = 0x8000000000ULL;  // 512GB (start of PML4[1])
    uint64_t ustack_virt = ustack_virt_base + (4 * PAGE_SIZE);  // Stack top
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

    // Allocate and map user pages WITHOUT switching page directories
    // (Switching is dangerous as kernel code might be in PML4[0])

    // Allocate user code pages in PML4[1] range to avoid PML4[0] conflicts
    uint64_t user_code_virt = 0x8000010000ULL;  // 512GB + 64KB
    uint64_t user_code_phys = pmm_alloc_pages(4);  // 16KB for code
    if (!user_code_phys) {
        vmm_destroy_address_space(pml4_phys);
        pmm_free_pages(kstack_phys, 4);
        pmm_free_pages(ustack_phys, 4);
        kfree(proc);
        return NULL;
    }

    // Manually map pages in the user page directory (without switching CR3)
    uint64_t *user_pml4 = (uint64_t *)vmm_phys_to_virt(pml4_phys);

    // Map user stack pages (at 0x7FFFFFFFFFF0 - 16KB)
    for (int i = 0; i < 4; i++) {
        uint64_t virt = ustack_virt_base + (i * PAGE_SIZE);
        uint64_t phys = ustack_phys + (i * PAGE_SIZE);
        // Manually create page tables and map
        manual_map_page(user_pml4, virt, phys, PAGE_FLAGS_USER);
    }

    // Map user code pages (at 0x400000)
    for (int i = 0; i < 4; i++) {
        uint64_t virt = user_code_virt + (i * PAGE_SIZE);
        uint64_t phys = user_code_phys + (i * PAGE_SIZE);
        manual_map_page(user_pml4, virt, phys, PAGE_FLAGS_USER);
    }

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
    proc->context.ds = 0x23;       // User data segment
    proc->context.es = 0x23;       // User data segment
    proc->context.fs = 0x23;       // User data segment
    proc->context.gs = 0x23;       // User data segment

    vga_printf("[PROC] User process RIP=0x%x RSP=0x%x CS=0x%x SS=0x%x\n",
               user_code_virt, ustack_virt, proc->context.cs, proc->context.ss);

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
