/**
 * Programmable Interval Timer (PIT) Implementation
 */

#include <kernel/timer.h>
#include <kernel/isr.h>
#include <kernel/pic.h>
#include <kernel/idt.h>
#include <kernel/port.h>
#include <kernel/vga.h>
#include <stdint.h>
#include <stddef.h>

// Global tick counter
static volatile uint64_t timer_ticks = 0;

// Timer frequency in Hz
static uint32_t timer_frequency = 0;

// Optional callback function
static void (*timer_callback)(void) = NULL;

/**
 * Timer interrupt handler
 */
static void timer_handler(registers_t *regs) {
    (void)regs;  // Unused

    // Increment tick counter
    timer_ticks++;

    // Call registered callback if any
    if (timer_callback != NULL) {
        timer_callback();
    }

    // Send EOI to PIC
    pic_send_eoi(IRQ_TIMER);
}

/**
 * Initialize PIT
 */
void timer_init(uint32_t frequency) {
    // Register timer interrupt handler
    isr_register_handler(IRQ_BASE + IRQ_TIMER, timer_handler);

    // Calculate divisor
    uint32_t divisor = PIT_FREQUENCY / frequency;

    // Send command byte
    uint8_t cmd = PIT_CMD_CHAN0 | PIT_CMD_RW_BOTH | PIT_CMD_MODE3 | PIT_CMD_BINARY;
    outb(PIT_COMMAND, cmd);

    // Send frequency divisor
    outb(PIT_CHANNEL0, divisor & 0xFF);         // Low byte
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);  // High byte

    // Save frequency
    timer_frequency = frequency;

    // Unmask IRQ0 (timer)
    pic_unmask_irq(IRQ_TIMER);

    vga_printf("  Timer: Initialized at %u Hz (%u ms per tick)\n",
               frequency, 1000 / frequency);
}

/**
 * Get system tick count
 */
uint64_t timer_get_ticks(void) {
    return timer_ticks;
}

/**
 * Get system uptime in milliseconds
 */
uint64_t timer_get_uptime_ms(void) {
    if (timer_frequency == 0) return 0;
    return (timer_ticks * 1000) / timer_frequency;
}

/**
 * Wait for a number of ticks
 */
void timer_wait_ticks(uint64_t ticks) {
    uint64_t target = timer_ticks + ticks;
    while (timer_ticks < target) {
        __asm__ volatile("hlt");  // Wait for interrupt
    }
}

/**
 * Sleep for a number of milliseconds
 */
void timer_sleep_ms(uint64_t ms) {
    if (timer_frequency == 0) return;
    uint64_t ticks = (ms * timer_frequency) / 1000;
    timer_wait_ticks(ticks);
}

/**
 * Register a timer callback
 */
void timer_register_callback(void (*callback)(void)) {
    timer_callback = callback;
}
