/**
 * Interrupt Service Routines (ISR) Implementation
 */

#include <kernel/isr.h>
#include <kernel/idt.h>
#include <kernel/vga.h>
#include <kernel/process.h>
#include <stdint.h>
#include <stddef.h>

// ISR handler table
static isr_handler_t interrupt_handlers[256] = {0};

// Exception messages
static const char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 FPU Error",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Security Exception",
    "Reserved"
};

/**
 * Register an interrupt handler
 */
void isr_register_handler(uint8_t num, isr_handler_t handler) {
    interrupt_handlers[num] = handler;
}

/**
 * Unregister an interrupt handler
 */
void isr_unregister_handler(uint8_t num) {
    interrupt_handlers[num] = NULL;
}

/**
 * Common interrupt handler
 *
 * Called from assembly ISR stubs.
 */
void isr_common_handler(registers_t *regs) {
    // Check if we have a custom handler
    if (interrupt_handlers[regs->int_no] != NULL) {
        interrupt_handlers[regs->int_no](regs);
        return;
    }

    // Handle CPU exceptions
    if (regs->int_no < 32) {
        vga_setcolor(VGA_COLOR_WHITE | (VGA_COLOR_RED << 4));
        vga_printf("\n\n*** CPU EXCEPTION: %s ***\n", exception_messages[regs->int_no]);
        vga_printf("Interrupt: %u, Error Code: 0x%x\n", regs->int_no, regs->err_code);
        vga_printf("RIP: 0x%x, CS: 0x%x, RFLAGS: 0x%x\n", regs->rip, regs->cs, regs->rflags);
        vga_printf("RSP: 0x%x, SS: 0x%x\n", regs->rsp, regs->ss);
        vga_printf("RAX: 0x%x, RBX: 0x%x, RCX: 0x%x, RDX: 0x%x\n",
                   regs->rax, regs->rbx, regs->rcx, regs->rdx);
        vga_printf("RSI: 0x%x, RDI: 0x%x, RBP: 0x%x\n",
                   regs->rsi, regs->rdi, regs->rbp);

        // Special handling for page fault
        if (regs->int_no == EXCEPTION_PAGE_FAULT) {
            uint64_t faulting_address;
            __asm__ volatile("mov %%cr2, %0" : "=r"(faulting_address));
            vga_printf("Faulting Address: 0x%x\n", faulting_address);
            vga_printf("Error Code: %s %s %s\n",
                       (regs->err_code & 0x1) ? "Protection Violation" : "Non-present Page",
                       (regs->err_code & 0x2) ? "Write" : "Read",
                       (regs->err_code & 0x4) ? "User-mode" : "Kernel-mode");
        }

        vga_puts("\nSystem Halted.\n");

        // Halt
        while (1) {
            __asm__ volatile("cli; hlt");
        }
    }

    // Unhandled interrupt
    vga_printf("Unhandled interrupt: %u\n", regs->int_no);
}

/**
 * Initialize ISRs
 */
void isr_init(void) {
    // Install exception handlers (0-31)
    idt_set_gate(0, (uint64_t)isr0, IDT_TYPE_INTERRUPT);
    idt_set_gate(1, (uint64_t)isr1, IDT_TYPE_INTERRUPT);
    idt_set_gate(2, (uint64_t)isr2, IDT_TYPE_INTERRUPT);
    idt_set_gate(3, (uint64_t)isr3, IDT_TYPE_INTERRUPT);
    idt_set_gate(4, (uint64_t)isr4, IDT_TYPE_INTERRUPT);
    idt_set_gate(5, (uint64_t)isr5, IDT_TYPE_INTERRUPT);
    idt_set_gate(6, (uint64_t)isr6, IDT_TYPE_INTERRUPT);
    idt_set_gate(7, (uint64_t)isr7, IDT_TYPE_INTERRUPT);
    idt_set_gate(8, (uint64_t)isr8, IDT_TYPE_INTERRUPT);
    idt_set_gate(10, (uint64_t)isr10, IDT_TYPE_INTERRUPT);
    idt_set_gate(11, (uint64_t)isr11, IDT_TYPE_INTERRUPT);
    idt_set_gate(12, (uint64_t)isr12, IDT_TYPE_INTERRUPT);
    idt_set_gate(13, (uint64_t)isr13, IDT_TYPE_INTERRUPT);
    idt_set_gate(14, (uint64_t)isr14, IDT_TYPE_INTERRUPT);
    idt_set_gate(16, (uint64_t)isr16, IDT_TYPE_INTERRUPT);
    idt_set_gate(17, (uint64_t)isr17, IDT_TYPE_INTERRUPT);
    idt_set_gate(18, (uint64_t)isr18, IDT_TYPE_INTERRUPT);
    idt_set_gate(19, (uint64_t)isr19, IDT_TYPE_INTERRUPT);
    idt_set_gate(20, (uint64_t)isr20, IDT_TYPE_INTERRUPT);
    idt_set_gate(30, (uint64_t)isr30, IDT_TYPE_INTERRUPT);

    // Install IRQ handlers (32-47)
    idt_set_gate(32, (uint64_t)irq0, IDT_TYPE_INTERRUPT);
    idt_set_gate(33, (uint64_t)irq1, IDT_TYPE_INTERRUPT);
    idt_set_gate(34, (uint64_t)irq2, IDT_TYPE_INTERRUPT);
    idt_set_gate(35, (uint64_t)irq3, IDT_TYPE_INTERRUPT);
    idt_set_gate(36, (uint64_t)irq4, IDT_TYPE_INTERRUPT);
    idt_set_gate(37, (uint64_t)irq5, IDT_TYPE_INTERRUPT);
    idt_set_gate(38, (uint64_t)irq6, IDT_TYPE_INTERRUPT);
    idt_set_gate(39, (uint64_t)irq7, IDT_TYPE_INTERRUPT);
    idt_set_gate(40, (uint64_t)irq8, IDT_TYPE_INTERRUPT);
    idt_set_gate(41, (uint64_t)irq9, IDT_TYPE_INTERRUPT);
    idt_set_gate(42, (uint64_t)irq10, IDT_TYPE_INTERRUPT);
    idt_set_gate(43, (uint64_t)irq11, IDT_TYPE_INTERRUPT);
    idt_set_gate(44, (uint64_t)irq12, IDT_TYPE_INTERRUPT);
    idt_set_gate(45, (uint64_t)irq13, IDT_TYPE_INTERRUPT);
    idt_set_gate(46, (uint64_t)irq14, IDT_TYPE_INTERRUPT);
    idt_set_gate(47, (uint64_t)irq15, IDT_TYPE_INTERRUPT);

    // Install syscall handler
    idt_set_gate(0x80, (uint64_t)isr128, IDT_TYPE_USER_INT);

    vga_printf("  ISR: Registered %u exception handlers\n", 32);
    vga_printf("  ISR: Registered %u IRQ handlers\n", 16);
}
