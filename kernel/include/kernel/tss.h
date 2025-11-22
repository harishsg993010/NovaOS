/**
 * Task State Segment (TSS)
 */

#ifndef KERNEL_TSS_H
#define KERNEL_TSS_H

#include <stdint.h>

/**
 * Initialize TSS
 */
void tss_init(void);

/**
 * Set kernel stack for interrupts from user mode
 */
void tss_set_kernel_stack(uint64_t stack);

/**
 * Get TSS address for GDT
 */
uint64_t tss_get_address(void);

/**
 * Get TSS size
 */
uint32_t tss_get_size(void);

#endif // KERNEL_TSS_H
