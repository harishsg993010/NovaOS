/**
 * Physical Memory Manager (PMM)
 *
 * Bitmap-based physical page allocator. Each bit represents one 4KB page.
 * Bit = 0: Page is free
 * Bit = 1: Page is used
 */

#include <kernel/pmm.h>
#include <kernel/memory.h>
#include <kernel/string.h>
#include <kernel/vga.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Bitmap to track page allocation (static for simplicity)
#define MAX_MEMORY_SIZE (4ULL * 1024 * 1024 * 1024)  // 4GB max
#define MAX_PAGES (MAX_MEMORY_SIZE / PAGE_SIZE)
#define BITMAP_SIZE (MAX_PAGES / 8)  // 8 bits per byte

static uint8_t page_bitmap[BITMAP_SIZE];
static uint32_t total_pages = 0;
static uint32_t used_pages = 0;
static uint64_t total_memory = 0;

/**
 * Set a bit in the bitmap
 */
static inline void bitmap_set(uint32_t bit) {
    page_bitmap[bit / 8] |= (1 << (bit % 8));
}

/**
 * Clear a bit in the bitmap
 */
static inline void bitmap_clear(uint32_t bit) {
    page_bitmap[bit / 8] &= ~(1 << (bit % 8));
}

/**
 * Test a bit in the bitmap
 */
static inline bool bitmap_test(uint32_t bit) {
    return page_bitmap[bit / 8] & (1 << (bit % 8));
}

/**
 * Find first free page in bitmap
 */
static uint32_t find_free_page(void) {
    for (uint32_t i = 0; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            return i;
        }
    }
    return (uint32_t)-1;  // No free pages
}

/**
 * Find n contiguous free pages in bitmap
 */
static uint32_t find_free_pages(size_t count) {
    if (count == 0) return (uint32_t)-1;
    if (count == 1) return find_free_page();

    for (uint32_t i = 0; i < total_pages - count + 1; i++) {
        bool found = true;
        for (size_t j = 0; j < count; j++) {
            if (bitmap_test(i + j)) {
                found = false;
                i += j;  // Skip ahead
                break;
            }
        }
        if (found) {
            return i;
        }
    }
    return (uint32_t)-1;  // Not enough contiguous pages
}

/**
 * Initialize physical memory manager
 */
void pmm_init(uint64_t mem_size, uint64_t kernel_end) {
    // Calculate total pages
    total_memory = mem_size;
    total_pages = mem_size / PAGE_SIZE;

    if (total_pages > MAX_PAGES) {
        total_pages = MAX_PAGES;
        total_memory = MAX_PAGES * PAGE_SIZE;
    }

    // Clear bitmap (all pages free initially)
    memset(page_bitmap, 0, BITMAP_SIZE);
    used_pages = 0;

    // Mark first page as used (real mode IVT, etc.)
    pmm_mark_used(0);

    // Mark kernel pages as used (from 1MB to kernel_end)
    uint64_t kernel_start = KERNEL_PHYSICAL_START;
    uint64_t kernel_size = kernel_end - kernel_start;
    size_t kernel_pages = BYTES_TO_PAGES(kernel_size);

    for (size_t i = 0; i < kernel_pages; i++) {
        pmm_mark_used(kernel_start + i * PAGE_SIZE);
    }

    // Mark bitmap itself as used
    uint64_t bitmap_addr = (uint64_t)page_bitmap;
    size_t bitmap_pages = BYTES_TO_PAGES(BITMAP_SIZE);

    for (size_t i = 0; i < bitmap_pages; i++) {
        pmm_mark_used(bitmap_addr + i * PAGE_SIZE);
    }

    vga_printf("  PMM: Managing %u MB (%u pages)\n",
               (uint32_t)(total_memory / (1024 * 1024)), total_pages);
    vga_printf("  PMM: Kernel occupies %u KB (%u pages)\n",
               (uint32_t)(kernel_size / 1024), (uint32_t)kernel_pages);
    vga_printf("  PMM: %u pages used, %u pages free\n",
               used_pages, total_pages - used_pages);
}

/**
 * Allocate a single physical page
 */
uint64_t pmm_alloc_page(void) {
    uint32_t page = find_free_page();
    if (page == (uint32_t)-1) {
        return 0;  // Out of memory
    }

    bitmap_set(page);
    used_pages++;

    return page * PAGE_SIZE;
}

/**
 * Allocate multiple contiguous physical pages
 */
uint64_t pmm_alloc_pages(size_t count) {
    if (count == 0) return 0;

    uint32_t page = find_free_pages(count);
    if (page == (uint32_t)-1) {
        return 0;  // Out of memory
    }

    // Mark all pages as used
    for (size_t i = 0; i < count; i++) {
        bitmap_set(page + i);
    }
    used_pages += count;

    return page * PAGE_SIZE;
}

/**
 * Free a single physical page
 */
void pmm_free_page(uint64_t addr) {
    if (addr == 0) return;  // Don't free NULL

    uint32_t page = addr / PAGE_SIZE;
    if (page >= total_pages) return;  // Invalid address

    if (!bitmap_test(page)) {
        // Double free - this is a bug!
        vga_printf("WARNING: Double free of page 0x%x\n", addr);
        return;
    }

    bitmap_clear(page);
    used_pages--;
}

/**
 * Free multiple contiguous physical pages
 */
void pmm_free_pages(uint64_t addr, size_t count) {
    for (size_t i = 0; i < count; i++) {
        pmm_free_page(addr + i * PAGE_SIZE);
    }
}

/**
 * Mark a physical page as used
 */
void pmm_mark_used(uint64_t addr) {
    uint32_t page = addr / PAGE_SIZE;
    if (page >= total_pages) return;

    if (!bitmap_test(page)) {
        bitmap_set(page);
        used_pages++;
    }
}

/**
 * Mark a range of physical pages as used
 */
void pmm_mark_used_range(uint64_t addr, size_t count) {
    for (size_t i = 0; i < count; i++) {
        pmm_mark_used(addr + i * PAGE_SIZE);
    }
}

/**
 * Get total number of physical pages
 */
uint32_t pmm_get_total_pages(void) {
    return total_pages;
}

/**
 * Get number of free physical pages
 */
uint32_t pmm_get_free_pages(void) {
    return total_pages - used_pages;
}

/**
 * Get number of used physical pages
 */
uint32_t pmm_get_used_pages(void) {
    return used_pages;
}

/**
 * Get total physical memory size
 */
uint64_t pmm_get_total_memory(void) {
    return total_memory;
}

/**
 * Get free physical memory size
 */
uint64_t pmm_get_free_memory(void) {
    return (total_pages - used_pages) * PAGE_SIZE;
}

/**
 * Check if a physical page is free
 */
bool pmm_is_free(uint64_t addr) {
    uint32_t page = addr / PAGE_SIZE;
    if (page >= total_pages) return false;
    return !bitmap_test(page);
}
