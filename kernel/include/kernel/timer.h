/**
 * Programmable Interval Timer (PIT)
 *
 * Provides periodic timer interrupts for scheduling and timekeeping.
 */

#ifndef KERNEL_TIMER_H
#define KERNEL_TIMER_H

#include <stdint.h>

// PIT I/O ports
#define PIT_CHANNEL0    0x40    // Channel 0 data port (IRQ0)
#define PIT_CHANNEL1    0x41    // Channel 1 data port (RAM refresh)
#define PIT_CHANNEL2    0x42    // Channel 2 data port (PC speaker)
#define PIT_COMMAND     0x43    // Mode/Command register

// PIT frequency
#define PIT_FREQUENCY   1193182 // Base frequency in Hz

// PIT command bits
#define PIT_CMD_BINARY  0x00    // Binary mode
#define PIT_CMD_BCD     0x01    // BCD mode
#define PIT_CMD_MODE0   0x00    // Interrupt on terminal count
#define PIT_CMD_MODE1   0x02    // Hardware re-triggerable one-shot
#define PIT_CMD_MODE2   0x04    // Rate generator
#define PIT_CMD_MODE3   0x06    // Square wave generator
#define PIT_CMD_MODE4   0x08    // Software triggered strobe
#define PIT_CMD_MODE5   0x0A    // Hardware triggered strobe
#define PIT_CMD_LATCH   0x00    // Latch count value
#define PIT_CMD_RW_LSB  0x10    // Read/Write LSB only
#define PIT_CMD_RW_MSB  0x20    // Read/Write MSB only
#define PIT_CMD_RW_BOTH 0x30    // Read/Write LSB then MSB
#define PIT_CMD_CHAN0   0x00    // Select channel 0
#define PIT_CMD_CHAN1   0x40    // Select channel 1
#define PIT_CMD_CHAN2   0x80    // Select channel 2

/**
 * Initialize PIT
 *
 * @param frequency Desired frequency in Hz (e.g., 100 for 100Hz/10ms ticks)
 */
void timer_init(uint32_t frequency);

/**
 * Get system tick count
 *
 * @return Number of timer ticks since boot
 */
uint64_t timer_get_ticks(void);

/**
 * Get system uptime in milliseconds
 *
 * @return Uptime in milliseconds
 */
uint64_t timer_get_uptime_ms(void);

/**
 * Sleep for a number of ticks
 *
 * @param ticks Number of ticks to sleep
 */
void timer_wait_ticks(uint64_t ticks);

/**
 * Sleep for a number of milliseconds
 *
 * @param ms Milliseconds to sleep
 */
void timer_sleep_ms(uint64_t ms);

/**
 * Register a timer callback
 *
 * @param callback Function to call on each tick
 */
void timer_register_callback(void (*callback)(void));

#endif // KERNEL_TIMER_H
