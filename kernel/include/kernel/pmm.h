/**
 * Physical Memory Manager (PMM)
 *
 * Manages physical memory pages using a bitmap allocator.
 * Each bit in the bitmap represents one 4KB page.
 */

#ifndef KERNEL_PMM_H
#define KERNEL_PMM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * Initialize physical memory manager
 *
 * @param mem_size Total physical memory size in bytes
 * @param kernel_end End address of kernel in physical memory
 */
void pmm_init(uint64_t mem_size, uint64_t kernel_end);

/**
 * Allocate a single physical page (4KB)
 *
 * @return Physical address of allocated page, or 0 if out of memory
 */
uint64_t pmm_alloc_page(void);

/**
 * Allocate multiple contiguous physical pages
 *
 * @param count Number of pages to allocate
 * @return Physical address of first allocated page, or 0 if out of memory
 */
uint64_t pmm_alloc_pages(size_t count);

/**
 * Free a single physical page
 *
 * @param addr Physical address of page to free
 */
void pmm_free_page(uint64_t addr);

/**
 * Free multiple contiguous physical pages
 *
 * @param addr Physical address of first page to free
 * @param count Number of pages to free
 */
void pmm_free_pages(uint64_t addr, size_t count);

/**
 * Mark a physical page as used (for boot-time allocations)
 *
 * @param addr Physical address of page to mark as used
 */
void pmm_mark_used(uint64_t addr);

/**
 * Mark a range of physical pages as used
 *
 * @param addr Physical address of first page
 * @param count Number of pages to mark as used
 */
void pmm_mark_used_range(uint64_t addr, size_t count);

/**
 * Get total number of physical pages
 *
 * @return Total number of 4KB pages
 */
uint32_t pmm_get_total_pages(void);

/**
 * Get number of free physical pages
 *
 * @return Number of free 4KB pages
 */
uint32_t pmm_get_free_pages(void);

/**
 * Get number of used physical pages
 *
 * @return Number of used 4KB pages
 */
uint32_t pmm_get_used_pages(void);

/**
 * Get total physical memory size
 *
 * @return Total memory in bytes
 */
uint64_t pmm_get_total_memory(void);

/**
 * Get free physical memory size
 *
 * @return Free memory in bytes
 */
uint64_t pmm_get_free_memory(void);

/**
 * Check if a physical page is free
 *
 * @param addr Physical address to check
 * @return true if page is free, false otherwise
 */
bool pmm_is_free(uint64_t addr);

#endif // KERNEL_PMM_H
