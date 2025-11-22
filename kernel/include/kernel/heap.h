/**
 * Kernel Heap Allocator
 *
 * Provides dynamic memory allocation for the kernel using a first-fit
 * algorithm with block headers and coalescing of free blocks.
 */

#ifndef KERNEL_HEAP_H
#define KERNEL_HEAP_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * Initialize kernel heap
 *
 * @param start_addr Virtual address where heap should start
 * @param initial_size Initial heap size in bytes
 */
void heap_init(uint64_t start_addr, size_t initial_size);

/**
 * Allocate memory from kernel heap
 *
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL if out of memory
 */
void *kmalloc(size_t size);

/**
 * Allocate and zero-initialize memory from kernel heap
 *
 * @param size Number of bytes to allocate
 * @return Pointer to allocated and zeroed memory, or NULL if out of memory
 */
void *kzalloc(size_t size);

/**
 * Allocate aligned memory from kernel heap
 *
 * @param size Number of bytes to allocate
 * @param alignment Required alignment (must be power of 2)
 * @return Pointer to allocated aligned memory, or NULL if out of memory
 */
void *kmalloc_aligned(size_t size, size_t alignment);

/**
 * Free memory allocated from kernel heap
 *
 * @param ptr Pointer to memory to free (can be NULL)
 */
void kfree(void *ptr);

/**
 * Reallocate memory from kernel heap
 *
 * @param ptr Pointer to previously allocated memory (can be NULL)
 * @param new_size New size in bytes
 * @return Pointer to reallocated memory, or NULL if out of memory
 */
void *krealloc(void *ptr, size_t new_size);

/**
 * Get total heap size
 *
 * @return Total heap size in bytes
 */
size_t heap_get_total_size(void);

/**
 * Get used heap size
 *
 * @return Used heap size in bytes
 */
size_t heap_get_used_size(void);

/**
 * Get free heap size
 *
 * @return Free heap size in bytes
 */
size_t heap_get_free_size(void);

/**
 * Get number of allocations
 *
 * @return Number of active allocations
 */
uint32_t heap_get_allocation_count(void);

/**
 * Expand heap by allocating more pages
 *
 * @param additional_size Additional size needed in bytes
 * @return 0 on success, -1 on failure
 */
int heap_expand(size_t additional_size);

/**
 * Print heap statistics (for debugging)
 */
void heap_print_stats(void);

/**
 * Validate heap integrity (for debugging)
 *
 * @return true if heap is valid, false if corrupted
 */
bool heap_validate(void);

#endif // KERNEL_HEAP_H
