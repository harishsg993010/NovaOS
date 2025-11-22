/**
 * VGA Text Mode Driver Header
 */

#ifndef KERNEL_VGA_H
#define KERNEL_VGA_H

#include <stdint.h>
#include <stddef.h>

// VGA color codes
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_YELLOW = 14,
    VGA_COLOR_WHITE = 15,
};

/**
 * Initialize VGA driver
 */
void vga_init(void);

/**
 * Clear the screen
 */
void vga_clear(void);

/**
 * Set text color
 */
void vga_setcolor(uint8_t color);

/**
 * Put character at current cursor position
 */
void vga_putchar(char c);

/**
 * Put string to screen
 */
void vga_puts(const char *str);

/**
 * Put string with specific length
 */
void vga_write(const char *data, size_t size);

/**
 * Formatted output (simple printf)
 */
void vga_printf(const char *format, ...);

#endif // KERNEL_VGA_H
