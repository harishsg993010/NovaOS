/**
 * Programmable Interrupt Controller (8259 PIC) Implementation
 */

#include <kernel/pic.h>
#include <kernel/port.h>
#include <kernel/vga.h>
#include <stdint.h>

/**
 * Initialize PIC
 */
void pic_init(uint8_t offset1, uint8_t offset2) {
    uint8_t mask1, mask2;

    // Save masks
    mask1 = inb(PIC1_DATA);
    mask2 = inb(PIC2_DATA);

    // Start initialization sequence (ICW1)
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    // ICW2: Set vector offsets
    outb(PIC1_DATA, offset1);
    io_wait();
    outb(PIC2_DATA, offset2);
    io_wait();

    // ICW3: Tell Master PIC there is a slave PIC at IRQ2
    outb(PIC1_DATA, 0x04);
    io_wait();
    // Tell Slave PIC its cascade identity
    outb(PIC2_DATA, 0x02);
    io_wait();

    // ICW4: Set 8086 mode
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    // Restore saved masks
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);

    vga_printf("  PIC: Initialized (Master: 0x%x, Slave: 0x%x)\n", offset1, offset2);
}

/**
 * Send End of Interrupt signal
 */
void pic_send_eoi(uint8_t irq) {
    // If IRQ came from slave PIC, send EOI to both
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    // Always send EOI to master
    outb(PIC1_COMMAND, PIC_EOI);
}

/**
 * Mask (disable) an IRQ
 */
void pic_mask_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port) | (1 << irq);
    outb(port, value);
}

/**
 * Unmask (enable) an IRQ
 */
void pic_unmask_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

/**
 * Disable all IRQs
 */
void pic_disable(void) {
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

/**
 * Get IRR (Interrupt Request Register)
 */
uint16_t pic_get_irr(void) {
    outb(PIC1_COMMAND, 0x0A);
    outb(PIC2_COMMAND, 0x0A);
    return (inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND);
}

/**
 * Get ISR (In-Service Register)
 */
uint16_t pic_get_isr(void) {
    outb(PIC1_COMMAND, 0x0B);
    outb(PIC2_COMMAND, 0x0B);
    return (inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND);
}
