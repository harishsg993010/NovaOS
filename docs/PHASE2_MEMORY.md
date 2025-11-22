# Phase 2: Memory Management - Complete Documentation

This document describes the memory management implementation in NovaeOS.

## Overview

Phase 2 implements three critical memory management subsystems:

1. **Physical Memory Manager (PMM)** - Manages physical RAM pages
2. **Virtual Memory Manager (VMM)** - Manages virtual address space and paging
3. **Kernel Heap Allocator** - Provides dynamic memory allocation (kmalloc/kfree)

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                   Application Layer                      │
│                 (uses kmalloc/kfree)                     │
└─────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│              Kernel Heap Allocator (heap.c)              │
│  - First-fit allocation algorithm                        │
│  - Block headers with size and free flags                │
│  - Automatic coalescing of free blocks                   │
│  - Heap expansion on demand                              │
└─────────────────────────────────────────────────────────┘
                          │
                          ▼
┌──────────────────────┬──────────────────────────────────┐
│  Virtual Memory      │    Physical Memory               │
│  Manager (vmm.c)     │    Manager (pmm.c)               │
│                      │                                  │
│  - 4-level paging    │    - Bitmap allocator            │
│  - PML4→PDPT→PD→PT   │    - Page allocation/free       │
│  - Map/unmap pages   │    - 4KB page granularity        │
│  - Address           │    - Tracks free/used pages      │
│    translation       │                                  │
└──────────────────────┴──────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│                   Physical RAM                           │
│                  (512 MB default)                        │
└─────────────────────────────────────────────────────────┘
```

## Physical Memory Manager (PMM)

### Implementation: Bitmap Allocator

**File**: `kernel/mm/pmm.c`

**Concept**: Each bit in a bitmap represents one 4KB page.
- Bit = 0: Page is free
- Bit = 1: Page is used

**Data Structures**:
```c
static uint8_t page_bitmap[BITMAP_SIZE];  // 128KB for 4GB RAM
static uint32_t total_pages;               // Total number of pages
static uint32_t used_pages;                // Number of used pages
```

**Key Functions**:

```c
// Initialize PMM with total memory and kernel end address
void pmm_init(uint64_t mem_size, uint64_t kernel_end);

// Allocate a single 4KB page
uint64_t pmm_alloc_page(void);

// Allocate n contiguous pages
uint64_t pmm_alloc_pages(size_t count);

// Free a page
void pmm_free_page(uint64_t addr);

// Free multiple pages
void pmm_free_pages(uint64_t addr, size_t count);

// Get statistics
uint32_t pmm_get_free_pages(void);
uint64_t pmm_get_free_memory(void);
```

**Algorithm**:
1. **Initialization**:
   - Clear bitmap (all pages free)
   - Mark kernel pages as used
   - Mark first page (IVT) as used
   - Mark bitmap itself as used

2. **Allocation** (`pmm_alloc_page`):
   - Scan bitmap for first free bit
   - Set bit to 1 (used)
   - Increment used_pages counter
   - Return physical address (bit_index × PAGE_SIZE)

3. **Free** (`pmm_free_page`):
   - Calculate bit index from address
   - Clear bit to 0 (free)
   - Decrement used_pages counter

**Complexity**:
- Allocation: O(n) worst case (linear search)
- Free: O(1)
- Space overhead: 1 bit per page = 0.003% of RAM

**Example Usage**:
```c
// Allocate a page
uint64_t phys_addr = pmm_alloc_page();
if (phys_addr == 0) {
    // Out of memory
}

// Use the page...

// Free the page
pmm_free_page(phys_addr);
```

## Virtual Memory Manager (VMM)

### Implementation: 4-Level Paging

**File**: `kernel/mm/vmm.c`

**Concept**: x86_64 uses 4-level page tables to translate virtual addresses to physical addresses.

**Page Table Hierarchy**:
```
Virtual Address (64-bit):
┌─────┬─────┬─────┬─────┬─────┬──────────┐
│Sign │PML4 │PDPT │ PD  │ PT  │  Offset  │
│ Ext │Index│Index│Index│Index│ (12 bits)│
└─────┴─────┴─────┴─────┴─────┴──────────┘
  16    9     9     9     9        12

CR3 Register → PML4 (Page Map Level 4)
                 ↓
               PDPT (Page Directory Pointer Table)
                 ↓
               PD (Page Directory)
                 ↓
               PT (Page Table)
                 ↓
            Physical Address
```

**Data Structures**:
```c
typedef uint64_t pte_t;  // Page table entry (64-bit)

typedef struct page_table {
    pte_t entries[512];  // 512 entries per table
} page_table_t;
```

**Page Table Entry Flags**:
```
┌───────────────┬─┬─┬─┬─┬─┬─┬─┬─┬────────────────────┐
│Physical Addr  │G│H│D│A│C│W│U│R│        Flags       │
└───────────────┴─┴─┴─┴─┴─┴─┴─┴─┴────────────────────┘
                 8 7 6 5 4 3 2 1 0

Bit 0 (P):  Present (1 = present in memory)
Bit 1 (W):  Writable (1 = read/write, 0 = read-only)
Bit 2 (U):  User (1 = user accessible, 0 = kernel only)
Bit 3 (WT): Write-through caching
Bit 4 (CD): Cache disabled
Bit 5 (A):  Accessed (set by CPU)
Bit 6 (D):  Dirty (set by CPU on write)
Bit 7 (H):  Huge page (2MB or 1GB)
Bit 8 (G):  Global (not flushed on CR3 reload)
Bit 63 (NX): No execute
```

**Key Functions**:

```c
// Initialize VMM (called during kernel init)
void vmm_init(void);

// Map virtual page to physical page
int vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags);

// Unmap virtual page
void vmm_unmap_page(uint64_t virt);

// Translate virtual address to physical
uint64_t vmm_get_physical(uint64_t virt);

// Check if address is mapped
bool vmm_is_mapped(uint64_t virt);

// Create new address space (for processes)
uint64_t vmm_create_address_space(void);

// Switch page directory
void vmm_switch_page_directory(uint64_t pml4_phys);
```

**Algorithm** (`vmm_map_page`):
1. Extract indices from virtual address
2. Navigate page table hierarchy:
   - If table doesn't exist, allocate new page for it
   - Clear new table to zeros
   - Set entry in parent table
3. Set final page table entry with physical address and flags
4. Invalidate TLB for this address

**Memory Layout**:
```
Virtual Address Space (64-bit):

0x0000000000000000 - 0x00007FFFFFFFFFFF  User Space (128 TB)
  0x0000000000400000                     User code (.text)
  0x0000000000600000                     User data (.data, .bss)
  0x0000700000000000                     User heap (grows up)
  0x00007FFFFFFFFFFF                     User stack (grows down)

0xFFFF800000000000 - 0xFFFFFFFFFFFFFFFF  Kernel Space (128 TB)
  0xFFFF800000000000                     Direct map of physical RAM
  0xFFFF800100000000                     Kernel code (.text)
  0xFFFF800100100000                     Kernel data (.data, .rodata)
  0xFFFF800200000000                     Kernel heap
  0xFFFFFFFF80000000                     Kernel stack
```

**Example Usage**:
```c
// Allocate physical page
uint64_t phys = pmm_alloc_page();

// Map to virtual address
uint64_t virt = 0xFFFF800300000000ULL;
vmm_map_page(virt, phys, PAGE_PRESENT | PAGE_WRITE);

// Access memory through virtual address
uint64_t *ptr = (uint64_t *)virt;
*ptr = 0x12345678;

// Translate back to physical
uint64_t phys_check = vmm_get_physical(virt);
// phys_check == phys

// Unmap when done
vmm_unmap_page(virt);
pmm_free_page(phys);
```

## Kernel Heap Allocator

### Implementation: First-Fit with Coalescing

**File**: `kernel/mm/heap.c`

**Concept**: Heap is divided into blocks. Each block has a header followed by data.

**Block Structure**:
```
┌──────────────────┬────────────────────┐
│   Block Header   │    Data Area       │
│  (heap_block_t)  │   (user data)      │
└──────────────────┴────────────────────┘

Header contains:
- magic:  0x48454150 ("HEAP") for validation
- size:   Total size of block (header + data)
- free:   Boolean flag (true = free, false = used)
- next:   Pointer to next block
- prev:   Pointer to previous block
```

**Data Structures**:
```c
typedef struct heap_block {
    uint32_t magic;              // Magic number (0x48454150)
    uint32_t size;               // Size including header
    bool free;                   // Is this block free?
    struct heap_block *next;     // Next block
    struct heap_block *prev;     // Previous block
} heap_block_t;
```

**Key Functions**:

```c
// Initialize heap at given address with initial size
void heap_init(uint64_t start_addr, size_t initial_size);

// Allocate memory
void *kmalloc(size_t size);

// Allocate and zero-initialize
void *kzalloc(size_t size);

// Allocate aligned memory
void *kmalloc_aligned(size_t size, size_t alignment);

// Free memory
void kfree(void *ptr);

// Reallocate memory
void *krealloc(void *ptr, size_t new_size);

// Get statistics
size_t heap_get_used_size(void);
size_t heap_get_free_size(void);
```

**Algorithms**:

**1. Allocation** (`kmalloc`):
```
1. Add sizeof(heap_block_t) to requested size
2. Search free list for first block ≥ size (first-fit)
3. If found:
   - If block is much larger, split it
   - Mark block as used
   - Return pointer to data (after header)
4. If not found:
   - Expand heap by allocating more pages
   - Create new block
   - Retry allocation
```

**2. Free** (`kfree`):
```
1. Validate magic number
2. Check for double free
3. Mark block as free
4. Coalesce with adjacent free blocks
```

**3. Coalescing**:
```
Walk through block list:
  If current block is free AND next block is free:
    - Merge blocks (add sizes)
    - Remove next block from list
    - Continue
```

**4. Splitting**:
```
If block size ≥ requested size + header + MIN_BLOCK_SIZE:
  - Create new block at (current + requested)
  - Set new block size = remaining space
  - Mark new block as free
  - Update linked list
  - Set current block size = requested
```

**Example Usage**:
```c
// Allocate a string
char *str = (char *)kmalloc(256);
strcpy(str, "Hello, heap!");

// Allocate an array
int *numbers = (int *)kmalloc(100 * sizeof(int));
for (int i = 0; i < 100; i++) {
    numbers[i] = i * i;
}

// Allocate and zero
struct my_data *data = (struct my_data *)kzalloc(sizeof(struct my_data));

// Free memory
kfree(str);
kfree(numbers);
kfree(data);

// Reallocate
str = (char *)kmalloc(64);
str = (char *)krealloc(str, 128);  // Expand to 128 bytes
kfree(str);
```

## Integration

**Initialization Order** (in `kernel/main.c`):

```c
void init_subsystems(void) {
    // 1. Initialize PMM
    pmm_init(TOTAL_MEMORY, kernel_end);

    // 2. Initialize VMM (uses PMM for page tables)
    vmm_init();

    // 3. Initialize Heap (uses VMM for mapping, PMM for pages)
    heap_init(heap_start, heap_size);
}
```

**Dependencies**:
- VMM depends on PMM (to allocate physical pages for page tables)
- Heap depends on both VMM (for virtual mappings) and PMM (for physical pages)

## Testing

**Test Suite** (in `kernel/main.c`):

```c
void test_memory_management(void) {
    // Test 1: PMM allocation and free
    uint64_t page = pmm_alloc_page();
    pmm_free_page(page);

    // Test 2: Heap allocation
    char *str = (char *)kmalloc(64);
    strcpy(str, "Test string");
    kfree(str);

    // Test 3: VMM mapping
    uint64_t virt = 0x400000;
    uint64_t phys = pmm_alloc_page();
    vmm_map_page(virt, phys, PAGE_FLAGS_KERNEL);
    vmm_unmap_page(virt);
    pmm_free_page(phys);
}
```

**Expected Output**:
```
Initializing Kernel Subsystems:
  [ OK ] VGA Text Mode
  PMM: Managing 512 MB (131072 pages)
  PMM: Kernel occupies 64 KB (16 pages)
  PMM: 16 pages used, 131056 pages free
  [ OK ] Physical Memory Manager (PMM)
  VMM: Current PML4 at 0x...
  VMM: Kernel mapped to higher half
  VMM: Paging enabled with 4-level page tables
  [ OK ] Virtual Memory Manager (VMM)
  Heap: Initialized at 0x..., size 16384 KB
  [ OK ] Kernel Heap Allocator

Testing Memory Management:
  PMM: Allocating 3 pages...
    Allocated: 0x..., 0x..., 0x...
  PMM: Freeing middle page...
  PMM: Free pages: 131055 / 131072
  Heap: Allocating memory...
    str1: Memory allocation works!
    str2: Heap allocator is functional!
    numbers[5] = 25
  Heap: Freeing memory...
  Heap: 0 KB used, 16384 KB free, 0 allocations
  VMM: Testing virtual memory mapping...
    Mapped 0x400000 -> 0x... (verified)

All tests passed! Kernel initialized successfully!
```

## Performance Characteristics

**PMM**:
- Allocation: O(n) worst case, typically O(1) for consecutive allocations
- Free: O(1)
- Memory overhead: ~0.003% (1 bit per page)

**VMM**:
- Map/unmap: O(1) amortized (may need to allocate tables)
- Translation: O(1) with TLB hit, O(4) on TLB miss
- Memory overhead: ~0.2% (page tables)

**Heap**:
- Allocation: O(n) worst case (linear search for free block)
- Free: O(n) (coalescing requires list traversal)
- Reallocate: O(n) (may need to copy data)
- Memory overhead: ~16 bytes per allocation (header)

## Future Optimizations

1. **PMM**: Use buddy allocator for better fragmentation resistance
2. **VMM**: Implement lazy allocation and copy-on-write
3. **Heap**: Use segregated free lists or slab allocator for common sizes
4. **All**: Add statistics and profiling

## Files Created

```
kernel/include/kernel/memory.h   - Common definitions
kernel/include/kernel/pmm.h      - PMM interface
kernel/include/kernel/vmm.h      - VMM interface
kernel/include/kernel/heap.h     - Heap interface
kernel/include/kernel/string.h   - String utilities
kernel/mm/pmm.c                  - PMM implementation
kernel/mm/vmm.c                  - VMM implementation
kernel/mm/heap.c                 - Heap implementation
lib/string.c                     - String utilities implementation
```

---

**Phase 2 Complete!** ✓

Next: **Phase 3 - Interrupts & Scheduling**
