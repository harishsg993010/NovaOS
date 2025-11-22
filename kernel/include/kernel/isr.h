/**
 * Interrupt Service Routines (ISRs)
 *
 * Handlers for CPU exceptions and hardware interrupts.
 */

#ifndef KERNEL_ISR_H
#define KERNEL_ISR_H

#include <stdint.h>

// CPU register state saved during interrupt
typedef struct registers {
    // Segment registers
    uint64_t ds;
    uint64_t es;
    uint64_t fs;
    uint64_t gs;

    // General purpose registers (pushed by our ISR stub)
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;

    // Interrupt number and error code
    uint64_t int_no;
    uint64_t err_code;

    // Pushed by CPU automatically
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) registers_t;

// Interrupt handler function type
typedef void (*isr_handler_t)(registers_t *regs);

/**
 * Initialize ISRs
 */
void isr_init(void);

/**
 * Register a custom interrupt handler
 *
 * @param num Interrupt number
 * @param handler Handler function
 */
void isr_register_handler(uint8_t num, isr_handler_t handler);

/**
 * Unregister an interrupt handler
 *
 * @param num Interrupt number
 */
void isr_unregister_handler(uint8_t num);

/**
 * Common interrupt handler (called from assembly stubs)
 */
void isr_common_handler(registers_t *regs);

// Exception handlers (implemented in assembly)
extern void isr0(void);   // Divide by zero
extern void isr1(void);   // Debug
extern void isr2(void);   // NMI
extern void isr3(void);   // Breakpoint
extern void isr4(void);   // Overflow
extern void isr5(void);   // Bound range exceeded
extern void isr6(void);   // Invalid opcode
extern void isr7(void);   // Device not available
extern void isr8(void);   // Double fault
extern void isr10(void);  // Invalid TSS
extern void isr11(void);  // Segment not present
extern void isr12(void);  // Stack-segment fault
extern void isr13(void);  // General protection fault
extern void isr14(void);  // Page fault
extern void isr16(void);  // x87 FPU error
extern void isr17(void);  // Alignment check
extern void isr18(void);  // Machine check
extern void isr19(void);  // SIMD exception
extern void isr20(void);  // Virtualization exception
extern void isr30(void);  // Security exception

// IRQ handlers (implemented in assembly)
extern void irq0(void);   // Timer
extern void irq1(void);   // Keyboard
extern void irq2(void);   // Cascade
extern void irq3(void);   // COM2
extern void irq4(void);   // COM1
extern void irq5(void);   // LPT2
extern void irq6(void);   // Floppy
extern void irq7(void);   // LPT1
extern void irq8(void);   // RTC
extern void irq9(void);   // ACPI
extern void irq10(void);  // Available
extern void irq11(void);  // Available
extern void irq12(void);  // Mouse
extern void irq13(void);  // FPU
extern void irq14(void);  // Primary ATA
extern void irq15(void);  // Secondary ATA

// Syscall handler
extern void isr128(void); // Syscall (int 0x80)

#endif // KERNEL_ISR_H
