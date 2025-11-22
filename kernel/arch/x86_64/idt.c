/**
 * Interrupt Descriptor Table (IDT) Implementation
 */

#include <kernel/idt.h>
#include <kernel/isr.h>
#include <kernel/string.h>
#include <kernel/vga.h>
#include <stdint.h>

// IDT with 256 entries
static idt_entry_t idt[256];

// IDT pointer for LIDT instruction
static idt_ptr_t idt_ptr;

/**
 * Set an IDT gate
 */
void idt_set_gate(uint8_t num, uint64_t handler, uint8_t type_attr) {
    idt[num].offset_low = handler & 0xFFFF;
    idt[num].offset_mid = (handler >> 16) & 0xFFFF;
    idt[num].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[num].selector = 0x08;  // Kernel code segment
    idt[num].ist = 0;          // No IST
    idt[num].type_attr = type_attr;
    idt[num].reserved = 0;
}

/**
 * Initialize IDT
 */
void idt_init(void) {
    // Clear IDT
    memset(&idt, 0, sizeof(idt));

    // Set up IDT pointer
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint64_t)&idt;

    // Install ISRs
    isr_init();

    // Load IDT
    __asm__ volatile("lidt %0" : : "m"(idt_ptr));

    vga_printf("  IDT: Initialized %u interrupt gates\n", 256);
}
