/**
 * NovaeOS Kernel Main Entry Point
 *
 * This is where the kernel starts execution after the bootloader
 * sets up the initial environment.
 */

#include <kernel/vga.h>
#include <kernel/memory.h>
#include <kernel/pmm.h>
#include <kernel/vmm.h>
#include <kernel/heap.h>
#include <kernel/string.h>
#include <kernel/idt.h>
#include <kernel/isr.h>
#include <kernel/pic.h>
#include <kernel/timer.h>
#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <kernel/gdt.h>
#include <kernel/syscall.h>
#include <kernel/block.h>
#include <kernel/ata.h>
#include <kernel/vfs.h>
#include <kernel/simplefs.h>
#include <stdint.h>
#include <stddef.h>

// Assume 512MB of RAM for now (can be detected from multiboot later)
#define TOTAL_MEMORY (512 * 1024 * 1024)

// Multiboot2 information structure
struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

struct multiboot_info {
    uint32_t total_size;
    uint32_t reserved;
    struct multiboot_tag tags[];
};

// External symbols from linker script
extern uint8_t _kernel_start[];
extern uint8_t _kernel_end[];
extern uint8_t _text_start[];
extern uint8_t _text_end[];
extern uint8_t _rodata_start[];
extern uint8_t _rodata_end[];
extern uint8_t _data_start[];
extern uint8_t _data_end[];
extern uint8_t _bss_start[];
extern uint8_t _bss_end[];

/**
 * Display kernel banner
 */
static void display_banner(void) {
    vga_setcolor(VGA_COLOR_LIGHT_CYAN | (VGA_COLOR_BLACK << 4));
    vga_puts("\n");
    vga_puts("    _   _                      ___  ____  \n");
    vga_puts("   | \\ | | _____   ____ _  ___|__ \\/ ___|  \n");
    vga_puts("   |  \\| |/ _ \\ \\ / / _` |/ _ \\ / /\\___ \\  \n");
    vga_puts("   | |\\  | (_) \\ V / (_| |  __// /_ ___) | \n");
    vga_puts("   |_| \\_|\\___/ \\_/ \\__,_|\\___/____|____/  \n");
    vga_puts("\n");

    vga_setcolor(VGA_COLOR_WHITE | (VGA_COLOR_BLACK << 4));
    vga_puts("   NovaeOS - Custom Operating System\n");
    vga_puts("   Version 0.1.0 (Development Build)\n");
    vga_puts("   Built for x86_64 architecture\n");
    vga_puts("\n");
}

/**
 * Display memory map information
 */
static void display_memory_info(void) {
    vga_setcolor(VGA_COLOR_LIGHT_GREEN | (VGA_COLOR_BLACK << 4));
    vga_puts("Kernel Memory Layout:\n");
    vga_setcolor(VGA_COLOR_WHITE | (VGA_COLOR_BLACK << 4));

    vga_printf("  Kernel Start: 0x%x\n", (uint64_t)_kernel_start);
    vga_printf("  Kernel End:   0x%x\n", (uint64_t)_kernel_end);
    vga_printf("  Kernel Size:  %u KB\n",
               ((uint64_t)_kernel_end - (uint64_t)_kernel_start) / 1024);
    vga_puts("\n");

    vga_printf("  .text:   0x%x - 0x%x (%u bytes)\n",
               (uint64_t)_text_start, (uint64_t)_text_end,
               (uint64_t)_text_end - (uint64_t)_text_start);
    vga_printf("  .rodata: 0x%x - 0x%x (%u bytes)\n",
               (uint64_t)_rodata_start, (uint64_t)_rodata_end,
               (uint64_t)_rodata_end - (uint64_t)_rodata_start);
    vga_printf("  .data:   0x%x - 0x%x (%u bytes)\n",
               (uint64_t)_data_start, (uint64_t)_data_end,
               (uint64_t)_data_end - (uint64_t)_data_start);
    vga_printf("  .bss:    0x%x - 0x%x (%u bytes)\n",
               (uint64_t)_bss_start, (uint64_t)_bss_end,
               (uint64_t)_bss_end - (uint64_t)_bss_start);
    vga_puts("\n");
}

/**
 * Display boot information
 */
static void display_boot_info(void) {
    vga_setcolor(VGA_COLOR_YELLOW | (VGA_COLOR_BLACK << 4));
    vga_puts("Boot Information:\n");
    vga_setcolor(VGA_COLOR_WHITE | (VGA_COLOR_BLACK << 4));

    vga_puts("  Bootloader:   GRUB2 (Multiboot2)\n");
    vga_puts("  CPU Mode:     Long Mode (64-bit)\n");
    vga_puts("  Paging:       Enabled\n");
    vga_puts("  Interrupts:   Disabled\n");
    vga_puts("\n");
}

/**
 * Display initialization status
 */
static void display_init_status(const char *component, int status) {
    vga_printf("  [");
    if (status == 0) {
        vga_setcolor(VGA_COLOR_LIGHT_GREEN | (VGA_COLOR_BLACK << 4));
        vga_puts(" OK ");
    } else {
        vga_setcolor(VGA_COLOR_LIGHT_RED | (VGA_COLOR_BLACK << 4));
        vga_puts("FAIL");
    }
    vga_setcolor(VGA_COLOR_WHITE | (VGA_COLOR_BLACK << 4));
    vga_printf("] %s\n", component);
}

/**
 * Initialize kernel subsystems
 */
static void init_subsystems(void) {
    vga_setcolor(VGA_COLOR_LIGHT_BLUE | (VGA_COLOR_BLACK << 4));
    vga_puts("Initializing Kernel Subsystems:\n");
    vga_setcolor(VGA_COLOR_WHITE | (VGA_COLOR_BLACK << 4));

    // VGA is already initialized
    display_init_status("VGA Text Mode", 0);

    // Initialize memory management
    // PMM: Physical Memory Manager
    uint64_t kernel_end = (uint64_t)_kernel_end;
    pmm_init(TOTAL_MEMORY, kernel_end);
    display_init_status("Physical Memory Manager (PMM)", 0);

    // VMM: Virtual Memory Manager
    vmm_init();
    display_init_status("Virtual Memory Manager (VMM)", 0);

    // Heap: Kernel heap allocator
    uint64_t heap_start = 0xFFFF800200000000ULL;  // Start of heap in higher half
    size_t heap_size = 16 * 1024 * 1024;  // 16MB initial size
    heap_init(heap_start, heap_size);
    display_init_status("Kernel Heap Allocator", 0);

    // GDT: Global Descriptor Table
    gdt_init();
    display_init_status("Global Descriptor Table (GDT)", 0);

    // IDT: Interrupt Descriptor Table
    idt_init();
    display_init_status("Interrupt Descriptor Table (IDT)", 0);

    // PIC: Programmable Interrupt Controller
    pic_init(0x20, 0x28);  // Remap PIC to 0x20-0x2F
    display_init_status("Programmable Interrupt Controller (PIC)", 0);

    // Timer: Programmable Interval Timer
    timer_init(100);  // 100Hz = 10ms ticks
    display_init_status("Timer (PIT)", 0);

    // Process Management
    process_init();
    display_init_status("Process Management", 0);

    // Scheduler
    scheduler_init(SCHED_ROUND_ROBIN);
    display_init_status("Scheduler", 0);

    // Syscall: System call interface
    syscall_init();
    display_init_status("System Call Interface", 0);

    // Block Devices
    block_init();
    display_init_status("Block Device Layer", 0);

    // ATA: Disk driver
    ata_init();
    display_init_status("ATA Disk Driver", 0);

    // VFS: Virtual Filesystem
    vfs_init();
    display_init_status("Virtual Filesystem (VFS)", 0);

    vga_puts("\n");
}

/**
 * Test memory management subsystems
 */
static void test_memory_management(void) {
    vga_setcolor(VGA_COLOR_LIGHT_MAGENTA | (VGA_COLOR_BLACK << 4));
    vga_puts("Testing Memory Management:\n");
    vga_setcolor(VGA_COLOR_WHITE | (VGA_COLOR_BLACK << 4));

    // Test PMM
    vga_puts("  PMM: Allocating 3 pages...\n");
    uint64_t page1 = pmm_alloc_page();
    uint64_t page2 = pmm_alloc_page();
    uint64_t page3 = pmm_alloc_page();
    vga_printf("    Allocated: 0x%x, 0x%x, 0x%x\n", page1, page2, page3);

    vga_puts("  PMM: Freeing middle page...\n");
    pmm_free_page(page2);

    vga_printf("  PMM: Free pages: %u / %u\n",
               pmm_get_free_pages(), pmm_get_total_pages());

    // Test heap
    vga_puts("  Heap: Allocating memory...\n");
    char *str1 = (char *)kmalloc(64);
    char *str2 = (char *)kmalloc(128);
    int *numbers = (int *)kmalloc(10 * sizeof(int));

    if (str1 && str2 && numbers) {
        strcpy(str1, "Memory allocation works!");
        strcpy(str2, "Heap allocator is functional!");

        for (int i = 0; i < 10; i++) {
            numbers[i] = i * i;
        }

        vga_printf("    str1: %s\n", str1);
        vga_printf("    str2: %s\n", str2);
        vga_printf("    numbers[5] = %d\n", numbers[5]);

        vga_puts("  Heap: Freeing memory...\n");
        kfree(str1);
        kfree(str2);
        kfree(numbers);

        vga_printf("  Heap: %u KB used, %u KB free, %u allocations\n",
                   (uint32_t)(heap_get_used_size() / 1024),
                   (uint32_t)(heap_get_free_size() / 1024),
                   heap_get_allocation_count());
    } else {
        vga_puts("    ERROR: Allocation failed!\n");
    }

    // Test VMM
    vga_puts("  VMM: Testing virtual memory mapping...\n");
    uint64_t test_virt = 0x400000;  // 4MB mark
    uint64_t test_phys = pmm_alloc_page();

    if (test_phys) {
        vmm_map_page(test_virt, test_phys, PAGE_FLAGS_KERNEL);
        uint64_t retrieved_phys = vmm_get_physical(test_virt);

        if (retrieved_phys == test_phys) {
            vga_printf("    Mapped 0x%x -> 0x%x (verified)\n", test_virt, test_phys);
        } else {
            vga_printf("    ERROR: Mapping verification failed!\n");
        }

        vmm_unmap_page(test_virt);
        pmm_free_page(test_phys);
    }

    vga_puts("\n");
}

/**
 * Test filesystem
 */
static void test_filesystem(void) {
    vga_setcolor(VGA_COLOR_LIGHT_MAGENTA | (VGA_COLOR_BLACK << 4));
    vga_puts("Testing Filesystem:\n");
    vga_setcolor(VGA_COLOR_WHITE | (VGA_COLOR_BLACK << 4));

    // Check if we have a disk
    block_device_t *disk = block_get_device("hda");
    if (!disk) {
        vga_puts("  No disk found (hda). Skipping filesystem tests.\n");
        vga_puts("  Note: Add -drive with QEMU to test filesystem.\n\n");
        return;
    }

    vga_printf("  Found disk: %s (%llu MB)\n", disk->name, disk->size / (1024 * 1024));

    // Format disk with SimpleFS
    vga_puts("  Formatting disk with SimpleFS...\n");
    if (simplefs_format(disk) != 0) {
        vga_puts("  ERROR: Failed to format disk!\n\n");
        return;
    }

    // Create filesystem
    vga_puts("  Creating SimpleFS instance...\n");
    filesystem_t *fs = simplefs_create(disk);
    if (!fs) {
        vga_puts("  ERROR: Failed to create filesystem!\n\n");
        return;
    }

    // Register filesystem
    vfs_register_filesystem(fs);

    // Mount filesystem
    vga_puts("  Mounting filesystem at '/'...\n");
    if (vfs_mount("/", fs) != 0) {
        vga_puts("  ERROR: Failed to mount filesystem!\n\n");
        return;
    }

    vga_puts("  Filesystem mounted successfully!\n");
    vga_puts("  Note: File operations available via syscalls.\n\n");
}

/**
 * Test task 1 - Prints periodically
 */
static void test_task1(void) {
    uint32_t count = 0;
    while (1) {
        vga_setcolor(VGA_COLOR_LIGHT_GREEN | (VGA_COLOR_BLACK << 4));
        vga_printf("[Task 1] Count: %u\n", count++);
        vga_setcolor(VGA_COLOR_WHITE | (VGA_COLOR_BLACK << 4));

        // Sleep for 1 second
        process_sleep(100);
    }
}

/**
 * Test task 2 - Prints periodically at different rate
 */
static void test_task2(void) {
    uint32_t count = 0;
    while (1) {
        vga_setcolor(VGA_COLOR_LIGHT_CYAN | (VGA_COLOR_BLACK << 4));
        vga_printf("[Task 2] Count: %u\n", count++);
        vga_setcolor(VGA_COLOR_WHITE | (VGA_COLOR_BLACK << 4));

        // Sleep for 1.5 seconds
        process_sleep(150);
    }
}

/**
 * Test task 3 - Shows uptime
 */
static void test_task3(void) {
    while (1) {
        uint64_t uptime = timer_get_uptime_ms();
        vga_setcolor(VGA_COLOR_YELLOW | (VGA_COLOR_BLACK << 4));
        vga_printf("[Task 3] Uptime: %u ms\n", (uint32_t)uptime);
        vga_setcolor(VGA_COLOR_WHITE | (VGA_COLOR_BLACK << 4));

        // Sleep for 2 seconds
        process_sleep(200);
    }
}

/**
 * Idle task - Runs when no other tasks are ready
 */
static void idle_task(void) {
    while (1) {
        __asm__ volatile("hlt");  // Halt until next interrupt
    }
}

/**
 * User mode test entry point
 * This function will run in Ring 3 and use syscalls
 */
static void user_mode_entry(void) {
    // This code will run in Ring 3 (user mode)
    // We'll use inline assembly to make syscalls

    // Syscall: getpid (SYS_GETPID = 5)
    int64_t pid;
    __asm__ volatile(
        "mov $5, %%rax\n"      // SYS_GETPID
        "int $0x80\n"
        : "=a"(pid)
        :
        : "memory"
    );

    // Simple loop to demonstrate user mode execution
    for (int i = 0; i < 5; i++) {
        // Syscall: putchar (SYS_PUTCHAR = 15)
        const char *msg = "[User Mode] Iteration: ";
        for (const char *p = msg; *p; p++) {
            __asm__ volatile(
                "mov $15, %%rax\n"   // SYS_PUTCHAR
                "mov %0, %%rdi\n"    // Character
                "int $0x80\n"
                :
                : "r"((uint64_t)*p)
                : "rax", "rdi", "memory"
            );
        }

        // Print iteration number
        char digit = '0' + i;
        __asm__ volatile(
            "mov $15, %%rax\n"
            "mov %0, %%rdi\n"
            "int $0x80\n"
            :
            : "r"((uint64_t)digit)
            : "rax", "rdi", "memory"
        );

        // Print newline
        __asm__ volatile(
            "mov $15, %%rax\n"
            "mov %0, %%rdi\n"
            "int $0x80\n"
            :
            : "r"((uint64_t)'\n')
            : "rax", "rdi", "memory"
        );

        // Syscall: sleep_ms (SYS_SLEEP = 6) - sleep for 1 second
        __asm__ volatile(
            "mov $6, %%rax\n"      // SYS_SLEEP
            "mov $1000, %%rdi\n"   // 1000 ms
            "int $0x80\n"
            :
            :
            : "rax", "rdi", "memory"
        );
    }

    // Syscall: exit (SYS_EXIT = 0)
    __asm__ volatile(
        "mov $0, %%rax\n"      // SYS_EXIT
        "mov $0, %%rdi\n"      // Exit code 0
        "int $0x80\n"
        :
        :
        : "rax", "rdi", "memory"
    );

    // Should never reach here
    while (1) {
        __asm__ volatile("hlt");
    }
}

/**
 * Test multitasking
 */
static void test_multitasking(void) {
    vga_setcolor(VGA_COLOR_LIGHT_MAGENTA | (VGA_COLOR_BLACK << 4));
    vga_puts("Testing Multitasking:\n");
    vga_setcolor(VGA_COLOR_WHITE | (VGA_COLOR_BLACK << 4));

    // Create test tasks
    process_t *task1 = process_create_kernel_task(test_task1, "task1", 0);
    process_t *task2 = process_create_kernel_task(test_task2, "task2", 0);
    process_t *task3 = process_create_kernel_task(test_task3, "task3", 0);
    process_t *idle = process_create_kernel_task(idle_task, "idle", 31);

    if (!task1 || !task2 || !task3 || !idle) {
        vga_puts("  ERROR: Failed to create tasks!\n");
        return;
    }

    vga_printf("  Created task 1 (PID %u)\n", task1->pid);
    vga_printf("  Created task 2 (PID %u)\n", task2->pid);
    vga_printf("  Created task 3 (PID %u)\n", task3->pid);
    vga_printf("  Created idle task (PID %u)\n", idle->pid);

    // Create user mode process
    vga_setcolor(VGA_COLOR_LIGHT_MAGENTA | (VGA_COLOR_BLACK << 4));
    vga_puts("  Creating user mode process...\n");
    vga_setcolor(VGA_COLOR_WHITE | (VGA_COLOR_BLACK << 4));

    process_t *user_proc = process_create_user((uint64_t)user_mode_entry, "user_test", 0);
    if (!user_proc) {
        vga_puts("  ERROR: Failed to create user mode process!\n");
    } else {
        vga_printf("  Created user mode process (PID %u)\n", user_proc->pid);
        scheduler_add_process(user_proc);
    }

    // Add tasks to scheduler
    scheduler_add_process(task1);
    scheduler_add_process(task2);
    scheduler_add_process(task3);
    scheduler_add_process(idle);

    vga_puts("  Added tasks to scheduler\n\n");

    // Start scheduler
    vga_setcolor(VGA_COLOR_LIGHT_GREEN | (VGA_COLOR_BLACK << 4));
    vga_puts("Starting Multitasking...\n");
    vga_setcolor(VGA_COLOR_WHITE | (VGA_COLOR_BLACK << 4));
    vga_puts("(You should see tasks alternating below)\n\n");

    scheduler_start();

    // Enable interrupts - this starts multitasking!
    interrupts_enable();

    // Main kernel thread becomes idle after this
    while (1) {
        __asm__ volatile("hlt");
    }
}

/**
 * Kernel main entry point
 * Called from boot.S after initial setup
 */
void kernel_main(void) {
    // Initialize VGA for output
    vga_init();
    vga_clear();

    // Display banner
    display_banner();

    // Display boot information
    display_boot_info();

    // Display memory information
    display_memory_info();

    // Initialize kernel subsystems
    init_subsystems();

    // Test memory management
    test_memory_management();

    // Test filesystem
    test_filesystem();

    // Success message
    vga_setcolor(VGA_COLOR_LIGHT_GREEN | (VGA_COLOR_BLACK << 4));
    vga_puts("All subsystems initialized successfully!\n");
    vga_setcolor(VGA_COLOR_WHITE | (VGA_COLOR_BLACK << 4));
    vga_puts("\n");

    // Test multitasking (this never returns)
    test_multitasking();

    // Should never reach here
    while (1) {
        __asm__ volatile("cli; hlt");
    }
}

/**
 * Kernel panic - called when an unrecoverable error occurs
 */
void kernel_panic(const char *message) {
    vga_setcolor(VGA_COLOR_WHITE | (VGA_COLOR_RED << 4));
    vga_puts("\n");
    vga_puts("*** KERNEL PANIC ***\n");
    vga_puts(message);
    vga_puts("\n");
    vga_puts("System halted.\n");

    // Halt forever
    while (1) {
        __asm__ volatile("cli; hlt");
    }
}
