/**
 * VGA Text Mode Driver
 * Provides basic text output functionality
 */

#include <kernel/vga.h>
#include <stdint.h>
#include <stddef.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

// Current cursor position
static uint16_t cursor_x = 0;
static uint16_t cursor_y = 0;

// Current color
static uint8_t current_color = VGA_COLOR_WHITE | (VGA_COLOR_BLACK << 4);

// VGA text buffer (initialized in vga_init to avoid issues with static initialization)
static uint16_t *vga_buffer;

/**
 * Make VGA entry from character and color
 */
static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

/**
 * Set character at position
 */
static void vga_putentryat(char c, uint8_t color, uint16_t x, uint16_t y) {
    const uint16_t index = y * VGA_WIDTH + x;
    vga_buffer[index] = vga_entry(c, color);
}

/**
 * Initialize VGA driver
 */
void vga_init(void) {
    // Initialize VGA buffer pointer (cast to ensure proper addressing)
    vga_buffer = (uint16_t *)((uint64_t)VGA_MEMORY);

    cursor_x = 0;
    cursor_y = 0;
    current_color = VGA_COLOR_WHITE | (VGA_COLOR_BLACK << 4);
    vga_clear();
}

/**
 * Clear the screen
 */
void vga_clear(void) {
    for (uint16_t y = 0; y < VGA_HEIGHT; y++) {
        for (uint16_t x = 0; x < VGA_WIDTH; x++) {
            vga_putentryat(' ', current_color, x, y);
        }
    }
    cursor_x = 0;
    cursor_y = 0;
}

/**
 * Set text color
 */
void vga_setcolor(uint8_t color) {
    current_color = color;
}

/**
 * Scroll screen up by one line
 */
static void vga_scroll(void) {
    // Move all lines up
    for (uint16_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (uint16_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }

    // Clear last line
    for (uint16_t x = 0; x < VGA_WIDTH; x++) {
        vga_putentryat(' ', current_color, x, VGA_HEIGHT - 1);
    }

    cursor_y = VGA_HEIGHT - 1;
}

/**
 * Move cursor to next line
 */
static void vga_newline(void) {
    cursor_x = 0;
    cursor_y++;
    if (cursor_y >= VGA_HEIGHT) {
        vga_scroll();
    }
}

/**
 * Put character at current cursor position
 */
void vga_putchar(char c) {
    if (c == '\n') {
        vga_newline();
        return;
    }

    if (c == '\r') {
        cursor_x = 0;
        return;
    }

    if (c == '\t') {
        cursor_x = (cursor_x + 4) & ~(4 - 1);
        if (cursor_x >= VGA_WIDTH) {
            vga_newline();
        }
        return;
    }

    if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            vga_putentryat(' ', current_color, cursor_x, cursor_y);
        }
        return;
    }

    vga_putentryat(c, current_color, cursor_x, cursor_y);
    cursor_x++;

    if (cursor_x >= VGA_WIDTH) {
        vga_newline();
    }
}

/**
 * Put string to screen
 */
void vga_puts(const char *str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

/**
 * Put string with specific length
 */
void vga_write(const char *data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        vga_putchar(data[i]);
    }
}

/**
 * Simple integer to string conversion
 */
static void itoa(int value, char *str, int base) {
    char *ptr = str;
    char *ptr1 = str;
    char tmp_char;
    int tmp_value;

    if (base < 2 || base > 36) {
        *str = '\0';
        return;
    }

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"
                 [35 + (tmp_value - value * base)];
    } while (value);

    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';

    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
}

/**
 * Simple printf implementation
 */
void vga_printf(const char *format, ...) {
    const char *p;
    int i;
    unsigned int u;
    char *s;
    char buf[32];

    __builtin_va_list args;
    __builtin_va_start(args, format);

    for (p = format; *p != '\0'; p++) {
        if (*p != '%') {
            vga_putchar(*p);
            continue;
        }

        p++;
        switch (*p) {
            case 'c':
                i = __builtin_va_arg(args, int);
                vga_putchar((char)i);
                break;

            case 's':
                s = __builtin_va_arg(args, char *);
                vga_puts(s);
                break;

            case 'd':
            case 'i':
                i = __builtin_va_arg(args, int);
                itoa(i, buf, 10);
                vga_puts(buf);
                break;

            case 'u':
                u = __builtin_va_arg(args, unsigned int);
                itoa(u, buf, 10);
                vga_puts(buf);
                break;

            case 'x':
                u = __builtin_va_arg(args, unsigned int);
                itoa(u, buf, 16);
                vga_puts(buf);
                break;

            case 'p':
                vga_puts("0x");
                u = (unsigned long)__builtin_va_arg(args, void *);
                itoa(u, buf, 16);
                vga_puts(buf);
                break;

            case '%':
                vga_putchar('%');
                break;

            default:
                vga_putchar('%');
                vga_putchar(*p);
                break;
        }
    }

    __builtin_va_end(args);
}
