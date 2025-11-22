/**
 * Interrupt Descriptor Table (IDT)
 *
 * The IDT contains 256 entries describing how to handle interrupts and exceptions.
 * Each entry points to an interrupt service routine (ISR).
 */

#ifndef KERNEL_IDT_H
#define KERNEL_IDT_H

#include <stdint.h>

// IDT entry (gate descriptor)
typedef struct idt_entry {
    uint16_t offset_low;    // Offset bits 0-15
    uint16_t selector;      // Code segment selector
    uint8_t ist;            // Interrupt Stack Table offset (0-7)
    uint8_t type_attr;      // Type and attributes
    uint16_t offset_mid;    // Offset bits 16-31
    uint32_t offset_high;   // Offset bits 32-63
    uint32_t reserved;      // Reserved (must be 0)
} __attribute__((packed)) idt_entry_t;

// IDT pointer structure (for LIDT instruction)
typedef struct idt_ptr {
    uint16_t limit;         // Size of IDT - 1
    uint64_t base;          // Base address of IDT
} __attribute__((packed)) idt_ptr_t;

// IDT gate types
#define IDT_TYPE_INTERRUPT  0x8E    // 64-bit interrupt gate (present, DPL=0)
#define IDT_TYPE_TRAP       0x8F    // 64-bit trap gate (present, DPL=0)
#define IDT_TYPE_USER_INT   0xEE    // 64-bit interrupt gate (present, DPL=3)

// CPU exception numbers
#define EXCEPTION_DIVIDE_ERROR          0
#define EXCEPTION_DEBUG                 1
#define EXCEPTION_NMI                   2
#define EXCEPTION_BREAKPOINT            3
#define EXCEPTION_OVERFLOW              4
#define EXCEPTION_BOUND_RANGE           5
#define EXCEPTION_INVALID_OPCODE        6
#define EXCEPTION_DEVICE_NOT_AVAILABLE  7
#define EXCEPTION_DOUBLE_FAULT          8
#define EXCEPTION_INVALID_TSS           10
#define EXCEPTION_SEGMENT_NOT_PRESENT   11
#define EXCEPTION_STACK_FAULT           12
#define EXCEPTION_GENERAL_PROTECTION    13
#define EXCEPTION_PAGE_FAULT            14
#define EXCEPTION_FPU_ERROR             16
#define EXCEPTION_ALIGNMENT_CHECK       17
#define EXCEPTION_MACHINE_CHECK         18
#define EXCEPTION_SIMD_ERROR            19

// Hardware interrupt numbers (IRQs)
#define IRQ_BASE        32      // IRQs start at interrupt 32
#define IRQ_TIMER       0       // PIT timer
#define IRQ_KEYBOARD    1       // PS/2 keyboard
#define IRQ_CASCADE     2       // Cascade (for slave PIC)
#define IRQ_COM2        3       // COM2
#define IRQ_COM1        4       // COM1
#define IRQ_LPT2        5       // LPT2
#define IRQ_FLOPPY      6       // Floppy disk
#define IRQ_LPT1        7       // LPT1
#define IRQ_RTC         8       // RTC
#define IRQ_ACPI        9       // ACPI
#define IRQ_AVAILABLE1  10      // Available
#define IRQ_AVAILABLE2  11      // Available
#define IRQ_MOUSE       12      // PS/2 mouse
#define IRQ_FPU         13      // FPU
#define IRQ_PRIMARY_ATA 14      // Primary ATA
#define IRQ_SECONDARY_ATA 15    // Secondary ATA

// Software interrupt numbers
#define INT_SYSCALL     0x80    // System call interrupt

/**
 * Initialize IDT
 */
void idt_init(void);

/**
 * Set an IDT entry
 *
 * @param num Interrupt number (0-255)
 * @param handler Address of ISR handler
 * @param type_attr Gate type and attributes
 */
void idt_set_gate(uint8_t num, uint64_t handler, uint8_t type_attr);

/**
 * Enable interrupts
 */
static inline void interrupts_enable(void) {
    __asm__ volatile("sti");
}

/**
 * Disable interrupts
 */
static inline void interrupts_disable(void) {
    __asm__ volatile("cli");
}

/**
 * Check if interrupts are enabled
 */
static inline int interrupts_enabled(void) {
    uint64_t flags;
    __asm__ volatile("pushfq; pop %0" : "=r"(flags));
    return flags & (1 << 9);
}

#endif // KERNEL_IDT_H
