/**
 * Virtual Memory Manager (VMM)
 *
 * Manages virtual memory using 4-level page tables (PML4 → PDPT → PD → PT)
 * Provides virtual-to-physical address mapping for both kernel and user space.
 */

#ifndef KERNEL_VMM_H
#define KERNEL_VMM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <kernel/memory.h>

/**
 * Initialize virtual memory manager
 *
 * Sets up kernel page tables and enables paging with proper mappings:
 * - Identity map first 4MB (for compatibility)
 * - Map kernel to higher half (0xFFFF800000000000+)
 * - Set up direct physical memory mapping
 */
void vmm_init(void);

/**
 * Map a virtual page to a physical page
 *
 * @param virt Virtual address to map
 * @param phys Physical address to map to
 * @param flags Page table entry flags (PAGE_PRESENT, PAGE_WRITE, etc.)
 * @return 0 on success, -1 on failure
 */
int vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags);

/**
 * Unmap a virtual page
 *
 * @param virt Virtual address to unmap
 */
void vmm_unmap_page(uint64_t virt);

/**
 * Map a range of virtual pages to physical pages
 *
 * @param virt Virtual address to start mapping
 * @param phys Physical address to map to
 * @param count Number of pages to map
 * @param flags Page table entry flags
 * @return 0 on success, -1 on failure
 */
int vmm_map_pages(uint64_t virt, uint64_t phys, size_t count, uint64_t flags);

/**
 * Unmap a range of virtual pages
 *
 * @param virt Virtual address to start unmapping
 * @param count Number of pages to unmap
 */
void vmm_unmap_pages(uint64_t virt, size_t count);

/**
 * Get physical address for a virtual address
 *
 * @param virt Virtual address to translate
 * @return Physical address, or 0 if not mapped
 */
uint64_t vmm_get_physical(uint64_t virt);

/**
 * Check if a virtual address is mapped
 *
 * @param virt Virtual address to check
 * @return true if mapped, false otherwise
 */
bool vmm_is_mapped(uint64_t virt);

/**
 * Create a new page directory (for new process)
 *
 * @return Physical address of new PML4, or 0 on failure
 */
uint64_t vmm_create_address_space(void);

/**
 * Destroy a page directory (for exiting process)
 *
 * @param pml4_phys Physical address of PML4 to destroy
 */
void vmm_destroy_address_space(uint64_t pml4_phys);

/**
 * Switch to a different page directory
 *
 * @param pml4_phys Physical address of PML4 to switch to
 */
void vmm_switch_page_directory(uint64_t pml4_phys);

/**
 * Get current page directory
 *
 * @return Physical address of current PML4
 */
uint64_t vmm_get_current_page_directory(void);

/**
 * Invalidate TLB entry for a virtual address
 *
 * @param virt Virtual address to invalidate
 */
static inline void vmm_invlpg(uint64_t virt) {
    __asm__ volatile("invlpg (%0)" :: "r"(virt) : "memory");
}

/**
 * Flush entire TLB
 */
static inline void vmm_flush_tlb(void) {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" :: "r"(cr3) : "memory");
}

/**
 * Convert physical address to virtual (using direct mapping)
 *
 * @param phys Physical address
 * @return Virtual address in direct map region
 */
static inline uint64_t vmm_phys_to_virt(uint64_t phys) {
    return phys + KERNEL_VIRTUAL_BASE;
}

/**
 * Convert virtual address to physical (from direct mapping)
 *
 * @param virt Virtual address in direct map region
 * @return Physical address
 */
static inline uint64_t vmm_virt_to_phys(uint64_t virt) {
    if (virt >= KERNEL_VIRTUAL_BASE) {
        return virt - KERNEL_VIRTUAL_BASE;
    }
    return virt;  // Identity mapped or low memory
}

#endif // KERNEL_VMM_H
