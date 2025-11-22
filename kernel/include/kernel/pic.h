/**
 * Programmable Interrupt Controller (PIC) - 8259
 *
 * Manages hardware interrupts (IRQs) from devices.
 * The PC has two PICs (master and slave) cascaded together.
 */

#ifndef KERNEL_PIC_H
#define KERNEL_PIC_H

#include <stdint.h>

// PIC I/O ports
#define PIC1_COMMAND    0x20    // Master PIC command
#define PIC1_DATA       0x21    // Master PIC data
#define PIC2_COMMAND    0xA0    // Slave PIC command
#define PIC2_DATA       0xA1    // Slave PIC data

// PIC commands
#define PIC_EOI         0x20    // End of interrupt

// ICW1 (Initialization Command Word 1)
#define ICW1_ICW4       0x01    // ICW4 needed
#define ICW1_SINGLE     0x02    // Single (cascade) mode
#define ICW1_INTERVAL4  0x04    // Call address interval 4 (8)
#define ICW1_LEVEL      0x08    // Level triggered (edge) mode
#define ICW1_INIT       0x10    // Initialization required

// ICW4 (Initialization Command Word 4)
#define ICW4_8086       0x01    // 8086/88 mode
#define ICW4_AUTO       0x02    // Auto EOI
#define ICW4_BUF_SLAVE  0x08    // Buffered mode (slave)
#define ICW4_BUF_MASTER 0x0C    // Buffered mode (master)
#define ICW4_SFNM       0x10    // Special fully nested mode

/**
 * Initialize PIC
 *
 * @param offset1 Vector offset for master PIC (usually 0x20)
 * @param offset2 Vector offset for slave PIC (usually 0x28)
 */
void pic_init(uint8_t offset1, uint8_t offset2);

/**
 * Send End of Interrupt (EOI) signal
 *
 * @param irq IRQ number (0-15)
 */
void pic_send_eoi(uint8_t irq);

/**
 * Mask (disable) an IRQ
 *
 * @param irq IRQ number (0-15)
 */
void pic_mask_irq(uint8_t irq);

/**
 * Unmask (enable) an IRQ
 *
 * @param irq IRQ number (0-15)
 */
void pic_unmask_irq(uint8_t irq);

/**
 * Disable all IRQs
 */
void pic_disable(void);

/**
 * Get IRR (Interrupt Request Register)
 *
 * @return 16-bit IRR (master in low byte, slave in high byte)
 */
uint16_t pic_get_irr(void);

/**
 * Get ISR (In-Service Register)
 *
 * @return 16-bit ISR (master in low byte, slave in high byte)
 */
uint16_t pic_get_isr(void);

#endif // KERNEL_PIC_H
