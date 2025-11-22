/**
 * Task State Segment (TSS)
 *
 * In x86-64 long mode, TSS is used for:
 * - Stack switching on privilege level changes (RSP0, RSP1, RSP2)
 * - Interrupt Stack Table (IST) for critical interrupts
 */

#include <stdint.h>
#include <string.h>

// TSS structure for x86-64
typedef struct tss {
    uint32_t reserved0;
    uint64_t rsp0;          // Kernel stack pointer (Ring 0)
    uint64_t rsp1;          // Ring 1 stack (unused in x86-64)
    uint64_t rsp2;          // Ring 2 stack (unused in x86-64)
    uint64_t reserved1;
    uint64_t ist[7];        // Interrupt Stack Table
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;    // I/O permission bitmap
} __attribute__((packed)) tss_t;

// Global TSS
static tss_t kernel_tss;

// Kernel stack for interrupt handling (16KB)
static uint8_t interrupt_stack[16384] __attribute__((aligned(16)));

/**
 * Initialize TSS
 */
void tss_init(void) {
    // Clear TSS
    memset(&kernel_tss, 0, sizeof(tss_t));

    // Set kernel stack pointer (RSP0) - stack grows down
    kernel_tss.rsp0 = (uint64_t)interrupt_stack + sizeof(interrupt_stack);

    // Set I/O map base beyond TSS limit (no I/O permission bitmap)
    kernel_tss.iomap_base = sizeof(tss_t);

    // Load TSS into GDT and TR register
    // The TSS descriptor should be in the GDT at index 5
    // This will be done in gdt_load_tss()
}

/**
 * Set kernel stack for interrupts from user mode
 */
void tss_set_kernel_stack(uint64_t stack) {
    kernel_tss.rsp0 = stack;
}

/**
 * Get TSS address for GDT
 */
uint64_t tss_get_address(void) {
    return (uint64_t)&kernel_tss;
}

/**
 * Get TSS size
 */
uint32_t tss_get_size(void) {
    return sizeof(tss_t);
}
