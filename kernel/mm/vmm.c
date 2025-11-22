/**
 * Virtual Memory Manager (VMM)
 *
 * Manages virtual memory using 4-level page tables:
 * PML4 → PDPT → PD → PT
 *
 * Each level has 512 entries, each covering:
 * - PML4 entry: 512 GB
 * - PDPT entry: 1 GB
 * - PD entry:   2 MB
 * - PT entry:   4 KB
 */

#include <kernel/vmm.h>
#include <kernel/pmm.h>
#include <kernel/memory.h>
#include <kernel/string.h>
#include <kernel/vga.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Current page directory (kernel)
static uint64_t *current_pml4 = NULL;

/**
 * Get or create page table at given index
 *
 * @param table Parent table
 * @param index Index in parent table
 * @param flags Flags to set if creating new table
 * @return Physical address of page table, or 0 on failure
 */
static uint64_t get_or_create_table(uint64_t *table, uint32_t index, uint64_t flags) {
    pte_t entry = table[index];

    if (entry & PAGE_PRESENT) {
        // Table already exists
        return entry & ~0xFFF;  // Mask off flags
    }

    // Allocate new page table
    uint64_t phys = pmm_alloc_page();
    if (phys == 0) {
        return 0;  // Out of memory
    }

    // Clear the new table
    uint64_t virt = vmm_phys_to_virt(phys);
    memset((void *)virt, 0, PAGE_SIZE);

    // Set entry in parent table
    table[index] = phys | flags;

    return phys;
}

/**
 * Initialize virtual memory manager
 */
void vmm_init(void) {
    // Get current CR3 (boot page tables are already set up in boot.S)
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));

    // Convert to virtual address for kernel access
    current_pml4 = (uint64_t *)vmm_phys_to_virt(cr3);

    vga_printf("  VMM: Current PML4 at 0x%x (phys: 0x%x)\n",
               (uint64_t)current_pml4, cr3);

    // Identity map first 4MB for compatibility
    for (uint64_t addr = 0; addr < 0x400000; addr += PAGE_SIZE) {
        vmm_map_page(addr, addr, PAGE_FLAGS_KERNEL);
    }

    // Map kernel to higher half (already done in boot.S, but ensure it's complete)
    extern uint8_t _kernel_start[];
    extern uint8_t _kernel_end[];
    uint64_t kernel_phys = (uint64_t)_kernel_start;
    uint64_t kernel_virt = vmm_phys_to_virt(kernel_phys);
    uint64_t kernel_size = (uint64_t)_kernel_end - (uint64_t)_kernel_start;
    size_t kernel_pages = BYTES_TO_PAGES(kernel_size);

    for (size_t i = 0; i < kernel_pages; i++) {
        uint64_t phys = kernel_phys + i * PAGE_SIZE;
        uint64_t virt = kernel_virt + i * PAGE_SIZE;
        vmm_map_page(virt, phys, PAGE_FLAGS_KERNEL);
    }

    vga_printf("  VMM: Kernel mapped to higher half (0x%x+)\n", kernel_virt);
    vga_printf("  VMM: Paging enabled with 4-level page tables\n");
}

/**
 * Map a virtual page to a physical page
 */
int vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags) {
    // Ensure addresses are page-aligned
    virt = PAGE_ALIGN_DOWN(virt);
    phys = PAGE_ALIGN_DOWN(phys);

    // Extract indices
    uint32_t pml4_idx = PML4_INDEX(virt);
    uint32_t pdpt_idx = PDPT_INDEX(virt);
    uint32_t pd_idx = PD_INDEX(virt);
    uint32_t pt_idx = PT_INDEX(virt);

    // Get or create PDPT
    uint64_t pdpt_phys = get_or_create_table(current_pml4, pml4_idx, PAGE_FLAGS_KERNEL);
    if (pdpt_phys == 0) return -1;
    uint64_t *pdpt = (uint64_t *)vmm_phys_to_virt(pdpt_phys);

    // Get or create PD
    uint64_t pd_phys = get_or_create_table(pdpt, pdpt_idx, PAGE_FLAGS_KERNEL);
    if (pd_phys == 0) return -1;
    uint64_t *pd = (uint64_t *)vmm_phys_to_virt(pd_phys);

    // Get or create PT
    uint64_t pt_phys = get_or_create_table(pd, pd_idx, PAGE_FLAGS_KERNEL);
    if (pt_phys == 0) return -1;
    uint64_t *pt = (uint64_t *)vmm_phys_to_virt(pt_phys);

    // Set page table entry
    pt[pt_idx] = phys | flags | PAGE_PRESENT;

    // Invalidate TLB for this address
    vmm_invlpg(virt);

    return 0;
}

/**
 * Unmap a virtual page
 */
void vmm_unmap_page(uint64_t virt) {
    virt = PAGE_ALIGN_DOWN(virt);

    // Extract indices
    uint32_t pml4_idx = PML4_INDEX(virt);
    uint32_t pdpt_idx = PDPT_INDEX(virt);
    uint32_t pd_idx = PD_INDEX(virt);
    uint32_t pt_idx = PT_INDEX(virt);

    // Navigate page tables
    pte_t pml4_entry = current_pml4[pml4_idx];
    if (!(pml4_entry & PAGE_PRESENT)) return;

    uint64_t *pdpt = (uint64_t *)vmm_phys_to_virt(pml4_entry & ~0xFFF);
    pte_t pdpt_entry = pdpt[pdpt_idx];
    if (!(pdpt_entry & PAGE_PRESENT)) return;

    uint64_t *pd = (uint64_t *)vmm_phys_to_virt(pdpt_entry & ~0xFFF);
    pte_t pd_entry = pd[pd_idx];
    if (!(pd_entry & PAGE_PRESENT)) return;

    uint64_t *pt = (uint64_t *)vmm_phys_to_virt(pd_entry & ~0xFFF);

    // Clear page table entry
    pt[pt_idx] = 0;

    // Invalidate TLB
    vmm_invlpg(virt);

    // TODO: Free empty page tables (optimization)
}

/**
 * Map a range of virtual pages to physical pages
 */
int vmm_map_pages(uint64_t virt, uint64_t phys, size_t count, uint64_t flags) {
    for (size_t i = 0; i < count; i++) {
        if (vmm_map_page(virt + i * PAGE_SIZE, phys + i * PAGE_SIZE, flags) != 0) {
            return -1;
        }
    }
    return 0;
}

/**
 * Unmap a range of virtual pages
 */
void vmm_unmap_pages(uint64_t virt, size_t count) {
    for (size_t i = 0; i < count; i++) {
        vmm_unmap_page(virt + i * PAGE_SIZE);
    }
}

/**
 * Get physical address for a virtual address
 */
uint64_t vmm_get_physical(uint64_t virt) {
    uint64_t page_offset = virt & (PAGE_SIZE - 1);
    virt = PAGE_ALIGN_DOWN(virt);

    // Extract indices
    uint32_t pml4_idx = PML4_INDEX(virt);
    uint32_t pdpt_idx = PDPT_INDEX(virt);
    uint32_t pd_idx = PD_INDEX(virt);
    uint32_t pt_idx = PT_INDEX(virt);

    // Navigate page tables
    pte_t pml4_entry = current_pml4[pml4_idx];
    if (!(pml4_entry & PAGE_PRESENT)) return 0;

    uint64_t *pdpt = (uint64_t *)vmm_phys_to_virt(pml4_entry & ~0xFFF);
    pte_t pdpt_entry = pdpt[pdpt_idx];
    if (!(pdpt_entry & PAGE_PRESENT)) return 0;

    uint64_t *pd = (uint64_t *)vmm_phys_to_virt(pdpt_entry & ~0xFFF);
    pte_t pd_entry = pd[pd_idx];
    if (!(pd_entry & PAGE_PRESENT)) return 0;

    uint64_t *pt = (uint64_t *)vmm_phys_to_virt(pd_entry & ~0xFFF);
    pte_t pt_entry = pt[pt_idx];
    if (!(pt_entry & PAGE_PRESENT)) return 0;

    uint64_t phys = pt_entry & ~0xFFF;
    return phys + page_offset;
}

/**
 * Check if a virtual address is mapped
 */
bool vmm_is_mapped(uint64_t virt) {
    return vmm_get_physical(virt) != 0;
}

/**
 * Create a new page directory (for new process)
 */
uint64_t vmm_create_address_space(void) {
    // Allocate new PML4
    uint64_t pml4_phys = pmm_alloc_page();
    if (pml4_phys == 0) return 0;

    uint64_t *pml4 = (uint64_t *)vmm_phys_to_virt(pml4_phys);
    memset(pml4, 0, PAGE_SIZE);

    // Copy kernel mappings from current PML4 (upper half)
    for (uint32_t i = 256; i < 512; i++) {
        pml4[i] = current_pml4[i];
    }

    return pml4_phys;
}

/**
 * Destroy a page directory
 */
void vmm_destroy_address_space(uint64_t pml4_phys) {
    uint64_t *pml4 = (uint64_t *)vmm_phys_to_virt(pml4_phys);

    // Free user-space page tables (lower half only)
    for (uint32_t i = 0; i < 256; i++) {
        if (pml4[i] & PAGE_PRESENT) {
            uint64_t *pdpt = (uint64_t *)vmm_phys_to_virt(pml4[i] & ~0xFFF);

            for (uint32_t j = 0; j < 512; j++) {
                if (pdpt[j] & PAGE_PRESENT) {
                    uint64_t *pd = (uint64_t *)vmm_phys_to_virt(pdpt[j] & ~0xFFF);

                    for (uint32_t k = 0; k < 512; k++) {
                        if (pd[k] & PAGE_PRESENT) {
                            uint64_t *pt = (uint64_t *)vmm_phys_to_virt(pd[k] & ~0xFFF);
                            pmm_free_page((uint64_t)pt - KERNEL_VIRTUAL_BASE);
                        }
                    }
                    pmm_free_page((uint64_t)pd - KERNEL_VIRTUAL_BASE);
                }
            }
            pmm_free_page((uint64_t)pdpt - KERNEL_VIRTUAL_BASE);
        }
    }

    // Free PML4 itself
    pmm_free_page(pml4_phys);
}

/**
 * Switch to a different page directory
 */
void vmm_switch_page_directory(uint64_t pml4_phys) {
    __asm__ volatile("mov %0, %%cr3" :: "r"(pml4_phys) : "memory");
    current_pml4 = (uint64_t *)vmm_phys_to_virt(pml4_phys);
}

/**
 * Get current page directory
 */
uint64_t vmm_get_current_page_directory(void) {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}
