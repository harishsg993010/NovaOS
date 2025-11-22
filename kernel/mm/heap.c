/**
 * Kernel Heap Allocator
 *
 * First-fit allocator with block headers and coalescing.
 *
 * Each allocated block has a header:
 * [Header][Data...]
 *
 * Header contains:
 * - Size of block (including header)
 * - Free flag
 * - Magic number (for validation)
 */

#include <kernel/heap.h>
#include <kernel/pmm.h>
#include <kernel/vmm.h>
#include <kernel/memory.h>
#include <kernel/string.h>
#include <kernel/vga.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define HEAP_MAGIC 0x48454150  // "HEAP"
#define MIN_BLOCK_SIZE 32      // Minimum allocation size

// Block header structure
typedef struct heap_block {
    uint32_t magic;              // Magic number for validation
    uint32_t size;               // Size of block (including header)
    bool free;                   // Is this block free?
    struct heap_block *next;     // Next block in list
    struct heap_block *prev;     // Previous block in list
} heap_block_t;

// Heap state
static uint64_t heap_start = 0;
static uint64_t heap_end = 0;
static size_t heap_size = 0;
static heap_block_t *first_block = NULL;
static uint32_t allocation_count = 0;

/**
 * Expand heap by allocating more pages
 */
int heap_expand(size_t additional_size) {
    // Round up to page boundary
    additional_size = PAGE_ALIGN(additional_size);
    size_t pages_needed = BYTES_TO_PAGES(additional_size);

    // Allocate physical pages
    for (size_t i = 0; i < pages_needed; i++) {
        uint64_t phys = pmm_alloc_page();
        if (phys == 0) {
            vga_printf("ERROR: Failed to expand heap (out of physical memory)\n");
            return -1;
        }

        // Map to virtual address
        uint64_t virt = heap_end + i * PAGE_SIZE;
        if (vmm_map_page(virt, phys, PAGE_FLAGS_KERNEL) != 0) {
            vga_printf("ERROR: Failed to map heap page\n");
            pmm_free_page(phys);
            return -1;
        }
    }

    heap_end += additional_size;
    heap_size += additional_size;

    return 0;
}

/**
 * Initialize kernel heap
 */
void heap_init(uint64_t start_addr, size_t initial_size) {
    heap_start = start_addr;
    heap_end = start_addr;
    heap_size = 0;

    // Expand heap to initial size
    if (heap_expand(initial_size) != 0) {
        vga_printf("ERROR: Failed to initialize heap\n");
        return;
    }

    // Create first block (entire heap is one big free block)
    first_block = (heap_block_t *)heap_start;
    first_block->magic = HEAP_MAGIC;
    first_block->size = heap_size;
    first_block->free = true;
    first_block->next = NULL;
    first_block->prev = NULL;

    allocation_count = 0;

    vga_printf("  Heap: Initialized at 0x%x, size %u KB\n",
               heap_start, (uint32_t)(heap_size / 1024));
}

/**
 * Find a free block that fits the requested size
 */
static heap_block_t *find_free_block(size_t size) {
    heap_block_t *current = first_block;

    while (current != NULL) {
        if (current->magic != HEAP_MAGIC) {
            vga_printf("ERROR: Heap corruption detected at 0x%x\n", (uint64_t)current);
            return NULL;
        }

        if (current->free && current->size >= size) {
            return current;
        }

        current = current->next;
    }

    return NULL;
}

/**
 * Split a block if it's large enough
 */
static void split_block(heap_block_t *block, size_t size) {
    // Only split if remainder is large enough for another block
    if (block->size >= size + sizeof(heap_block_t) + MIN_BLOCK_SIZE) {
        heap_block_t *new_block = (heap_block_t *)((uint8_t *)block + size);
        new_block->magic = HEAP_MAGIC;
        new_block->size = block->size - size;
        new_block->free = true;
        new_block->next = block->next;
        new_block->prev = block;

        if (block->next != NULL) {
            block->next->prev = new_block;
        }

        block->next = new_block;
        block->size = size;
    }
}

/**
 * Coalesce adjacent free blocks
 */
static void coalesce_blocks(void) {
    heap_block_t *current = first_block;

    while (current != NULL && current->next != NULL) {
        if (current->free && current->next->free) {
            // Merge with next block
            current->size += current->next->size;
            current->next = current->next->next;

            if (current->next != NULL) {
                current->next->prev = current;
            }
        } else {
            current = current->next;
        }
    }
}

/**
 * Allocate memory from kernel heap
 */
void *kmalloc(size_t size) {
    if (size == 0) return NULL;

    // Add header size and align
    size_t total_size = sizeof(heap_block_t) + size;
    if (total_size < MIN_BLOCK_SIZE) {
        total_size = MIN_BLOCK_SIZE;
    }
    total_size = (total_size + 7) & ~7;  // 8-byte alignment

    // Find free block
    heap_block_t *block = find_free_block(total_size);

    // If no block found, try expanding heap
    if (block == NULL) {
        size_t expand_size = total_size * 2;  // Expand by at least 2x requested size
        if (expand_size < PAGE_SIZE) {
            expand_size = PAGE_SIZE;
        }

        if (heap_expand(expand_size) != 0) {
            return NULL;  // Out of memory
        }

        // Create new block in expanded space
        block = (heap_block_t *)(heap_end - expand_size);
        block->magic = HEAP_MAGIC;
        block->size = expand_size;
        block->free = true;
        block->next = NULL;
        block->prev = NULL;

        // Add to block list
        if (first_block == NULL) {
            first_block = block;
        } else {
            heap_block_t *last = first_block;
            while (last->next != NULL) {
                last = last->next;
            }
            last->next = block;
            block->prev = last;
        }
    }

    // Split block if it's too large
    split_block(block, total_size);

    // Mark block as used
    block->free = false;
    allocation_count++;

    // Return pointer to data (after header)
    return (void *)((uint8_t *)block + sizeof(heap_block_t));
}

/**
 * Allocate and zero-initialize memory
 */
void *kzalloc(size_t size) {
    void *ptr = kmalloc(size);
    if (ptr != NULL) {
        memset(ptr, 0, size);
    }
    return ptr;
}

/**
 * Allocate aligned memory
 */
void *kmalloc_aligned(size_t size, size_t alignment) {
    // Simple implementation: allocate extra and adjust pointer
    // More efficient implementation would integrate alignment into allocator
    size_t total_size = size + alignment + sizeof(heap_block_t);
    void *ptr = kmalloc(total_size);
    if (ptr == NULL) return NULL;

    uint64_t addr = (uint64_t)ptr;
    uint64_t aligned = (addr + alignment - 1) & ~(alignment - 1);

    // For simplicity, we'll just return the oversized allocation
    // A better implementation would track the original pointer
    return (void *)aligned;
}

/**
 * Free memory allocated from kernel heap
 */
void kfree(void *ptr) {
    if (ptr == NULL) return;

    // Get block header
    heap_block_t *block = (heap_block_t *)((uint8_t *)ptr - sizeof(heap_block_t));

    // Validate magic number
    if (block->magic != HEAP_MAGIC) {
        vga_printf("ERROR: Invalid free (bad magic) at 0x%x\n", (uint64_t)ptr);
        return;
    }

    // Check for double free
    if (block->free) {
        vga_printf("ERROR: Double free detected at 0x%x\n", (uint64_t)ptr);
        return;
    }

    // Mark block as free
    block->free = true;
    allocation_count--;

    // Coalesce adjacent free blocks
    coalesce_blocks();
}

/**
 * Reallocate memory
 */
void *krealloc(void *ptr, size_t new_size) {
    if (ptr == NULL) {
        return kmalloc(new_size);
    }

    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }

    // Get current block
    heap_block_t *block = (heap_block_t *)((uint8_t *)ptr - sizeof(heap_block_t));

    // Validate
    if (block->magic != HEAP_MAGIC) {
        vga_printf("ERROR: Invalid realloc (bad magic) at 0x%x\n", (uint64_t)ptr);
        return NULL;
    }

    size_t current_size = block->size - sizeof(heap_block_t);

    // If new size fits in current block, just return same pointer
    if (new_size <= current_size) {
        return ptr;
    }

    // Allocate new block
    void *new_ptr = kmalloc(new_size);
    if (new_ptr == NULL) {
        return NULL;
    }

    // Copy data
    memcpy(new_ptr, ptr, current_size);

    // Free old block
    kfree(ptr);

    return new_ptr;
}

/**
 * Get total heap size
 */
size_t heap_get_total_size(void) {
    return heap_size;
}

/**
 * Get used heap size
 */
size_t heap_get_used_size(void) {
    size_t used = 0;
    heap_block_t *current = first_block;

    while (current != NULL) {
        if (!current->free) {
            used += current->size;
        }
        current = current->next;
    }

    return used;
}

/**
 * Get free heap size
 */
size_t heap_get_free_size(void) {
    return heap_size - heap_get_used_size();
}

/**
 * Get number of allocations
 */
uint32_t heap_get_allocation_count(void) {
    return allocation_count;
}

/**
 * Print heap statistics
 */
void heap_print_stats(void) {
    vga_printf("\nHeap Statistics:\n");
    vga_printf("  Total size:  %u KB\n", (uint32_t)(heap_size / 1024));
    vga_printf("  Used size:   %u KB\n", (uint32_t)(heap_get_used_size() / 1024));
    vga_printf("  Free size:   %u KB\n", (uint32_t)(heap_get_free_size() / 1024));
    vga_printf("  Allocations: %u\n", allocation_count);

    // Count blocks
    uint32_t total_blocks = 0;
    uint32_t free_blocks = 0;
    heap_block_t *current = first_block;

    while (current != NULL) {
        total_blocks++;
        if (current->free) free_blocks++;
        current = current->next;
    }

    vga_printf("  Total blocks: %u\n", total_blocks);
    vga_printf("  Free blocks:  %u\n", free_blocks);
}

/**
 * Validate heap integrity
 */
bool heap_validate(void) {
    heap_block_t *current = first_block;
    uint32_t count = 0;

    while (current != NULL) {
        if (current->magic != HEAP_MAGIC) {
            vga_printf("ERROR: Invalid magic at block %u (0x%x)\n", count, (uint64_t)current);
            return false;
        }

        if (current->next != NULL && current->next->prev != current) {
            vga_printf("ERROR: Broken link at block %u\n", count);
            return false;
        }

        count++;
        if (count > 100000) {  // Prevent infinite loop
            vga_printf("ERROR: Heap list too long or circular\n");
            return false;
        }

        current = current->next;
    }

    return true;
}
