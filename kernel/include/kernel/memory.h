/**
 * Memory Management - Common Definitions
 *
 * This header defines common memory management constants, macros,
 * and structures used across PMM, VMM, and heap allocator.
 */

#ifndef KERNEL_MEMORY_H
#define KERNEL_MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Page size and alignment
#define PAGE_SIZE 4096
#define PAGE_ALIGN(addr) (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#define PAGE_ALIGN_DOWN(addr) ((addr) & ~(PAGE_SIZE - 1))
#define IS_PAGE_ALIGNED(addr) (((addr) & (PAGE_SIZE - 1)) == 0)

// Convert between pages and bytes
#define BYTES_TO_PAGES(bytes) (((bytes) + PAGE_SIZE - 1) / PAGE_SIZE)
#define PAGES_TO_BYTES(pages) ((pages) * PAGE_SIZE)

// Page table entry flags (x86_64)
#define PAGE_PRESENT    (1ULL << 0)   // Page is present in memory
#define PAGE_WRITE      (1ULL << 1)   // Page is writable
#define PAGE_USER       (1ULL << 2)   // Page is user-accessible
#define PAGE_WRITETHROUGH (1ULL << 3) // Write-through caching
#define PAGE_CACHE_DISABLE (1ULL << 4) // Cache disabled
#define PAGE_ACCESSED   (1ULL << 5)   // Page has been accessed
#define PAGE_DIRTY      (1ULL << 6)   // Page has been written to
#define PAGE_HUGE       (1ULL << 7)   // Huge page (2MB/1GB)
#define PAGE_GLOBAL     (1ULL << 8)   // Global page (not flushed on CR3 reload)
#define PAGE_NX         (1ULL << 63)  // No execute

// Common page table flags
#define PAGE_FLAGS_KERNEL (PAGE_PRESENT | PAGE_WRITE)
#define PAGE_FLAGS_USER   (PAGE_PRESENT | PAGE_WRITE | PAGE_USER)

// Memory regions
#define KERNEL_PHYSICAL_START 0x100000  // 1MB
#define KERNEL_VIRTUAL_BASE   0xFFFF800000000000ULL  // Higher half

// Extract page table indices from virtual address
#define PML4_INDEX(addr) (((addr) >> 39) & 0x1FF)
#define PDPT_INDEX(addr) (((addr) >> 30) & 0x1FF)
#define PD_INDEX(addr)   (((addr) >> 21) & 0x1FF)
#define PT_INDEX(addr)   (((addr) >> 12) & 0x1FF)

// Page table entry structure
typedef uint64_t pte_t;

// Page table structure (512 entries)
typedef struct page_table {
    pte_t entries[512];
} __attribute__((aligned(PAGE_SIZE))) page_table_t;

// Memory statistics
typedef struct memory_stats {
    uint64_t total_memory;      // Total physical memory
    uint64_t used_memory;       // Used physical memory
    uint64_t free_memory;       // Free physical memory
    uint64_t kernel_memory;     // Memory used by kernel
    uint64_t heap_memory;       // Memory used by heap
    uint32_t total_pages;       // Total number of pages
    uint32_t used_pages;        // Number of used pages
    uint32_t free_pages;        // Number of free pages
} memory_stats_t;

/**
 * Get current memory statistics
 */
void memory_get_stats(memory_stats_t *stats);

/**
 * Initialize all memory management subsystems
 */
void memory_init(uint64_t total_memory);

#endif // KERNEL_MEMORY_H
