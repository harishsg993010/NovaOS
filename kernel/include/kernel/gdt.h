/**
 * Global Descriptor Table (GDT)
 */

#ifndef KERNEL_GDT_H
#define KERNEL_GDT_H

/**
 * Initialize GDT with kernel and user segments
 */
void gdt_init(void);

#endif // KERNEL_GDT_H
