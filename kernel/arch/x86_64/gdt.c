/**
 * Global Descriptor Table (GDT) Setup
 *
 * Sets up segment descriptors for kernel and user mode.
 */

#include <kernel/vga.h>
#include <kernel/string.h>
#include <stdint.h>

// GDT entry structure
typedef struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

// GDT pointer structure
typedef struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) gdt_ptr_t;

// GDT with 5 entries:
// 0: Null descriptor
// 1: Kernel code (0x08)
// 2: Kernel data (0x10)
// 3: User code   (0x18, or 0x1B with RPL=3)
// 4: User data   (0x20, or 0x23 with RPL=3)
static gdt_entry_t gdt[5];
static gdt_ptr_t gdt_ptr;

/**
 * Set a GDT entry
 */
static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_mid = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access = access;
}

/**
 * Initialize GDT
 */
void gdt_init(void) {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 5) - 1;
    gdt_ptr.base = (uint64_t)&gdt;

    // Null descriptor
    gdt_set_gate(0, 0, 0, 0, 0);

    // Kernel code segment (0x08)
    // Base = 0, Limit = 0xFFFFF, Access = 0x9A, Granularity = 0xAF
    // Access: Present=1, DPL=00 (kernel), Type=1010 (code, execute/read)
    // Gran: G=1 (4KB), D=0, L=1 (64-bit), AVL=0, Limit[19:16]=1111
    gdt_set_gate(1, 0, 0xFFFFF, 0x9A, 0xAF);

    // Kernel data segment (0x10)
    // Access: Present=1, DPL=00 (kernel), Type=0010 (data, read/write)
    gdt_set_gate(2, 0, 0xFFFFF, 0x92, 0xCF);

    // User code segment (0x18, with RPL=3 becomes 0x1B)
    // Access: Present=1, DPL=11 (user), Type=1010 (code, execute/read)
    gdt_set_gate(3, 0, 0xFFFFF, 0xFA, 0xAF);

    // User data segment (0x20, with RPL=3 becomes 0x23)
    // Access: Present=1, DPL=11 (user), Type=0010 (data, read/write)
    gdt_set_gate(4, 0, 0xFFFFF, 0xF2, 0xCF);

    // Load GDT
    __asm__ volatile("lgdt %0" : : "m"(gdt_ptr));

    // Reload segment registers
    __asm__ volatile(
        "mov $0x10, %%ax\n"    // Kernel data segment
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        ::: "ax"
    );

    // Reload code segment with far return
    __asm__ volatile(
        "pushq $0x08\n"        // Kernel code segment
        "lea 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        ::: "rax"
    );

    vga_printf("  GDT: Initialized (5 segments: null, kcode, kdata, ucode, udata)\n");
}
