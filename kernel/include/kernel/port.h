/**
 * Port I/O Operations
 *
 * Functions for reading and writing to I/O ports.
 */

#ifndef KERNEL_PORT_H
#define KERNEL_PORT_H

#include <stdint.h>

/**
 * Write byte to I/O port
 *
 * @param port Port number
 * @param value Byte to write
 */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * Write word (16-bit) to I/O port
 *
 * @param port Port number
 * @param value Word to write
 */
static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * Write dword (32-bit) to I/O port
 *
 * @param port Port number
 * @param value Dword to write
 */
static inline void outl(uint16_t port, uint32_t value) {
    __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * Read byte from I/O port
 *
 * @param port Port number
 * @return Byte read
 */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * Read word (16-bit) from I/O port
 *
 * @param port Port number
 * @return Word read
 */
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * Read dword (32-bit) from I/O port
 *
 * @param port Port number
 * @return Dword read
 */
static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * Wait for I/O operation to complete
 *
 * This is a small delay to ensure I/O operations complete.
 */
static inline void io_wait(void) {
    outb(0x80, 0);  // Write to unused port
}

#endif // KERNEL_PORT_H
