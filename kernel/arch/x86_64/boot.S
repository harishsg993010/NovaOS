; NovaeOS x86_64 Bootstrap Code
; Sets up the initial environment and jumps to kernel_main()

BITS 32

; Bootstrap stack (16KB)
section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

; Temporary page tables for identity mapping and higher-half
; We need multiple PD tables to map enough memory
align 4096
boot_pml4:
    resb 4096
boot_pdpt:
    resb 4096
boot_pd:
    resb 4096
boot_pt_low:
    resb 4096  ; Page table for first 2MB (includes VGA at 0xB8000)

; Code section
section .text
global _start
extern kernel_main

; Multiboot2 header - MUST be at the very start of .text section
align 8
multiboot_start:
    dd 0xe85250d6                ; Magic number
    dd 0                         ; Architecture (0 = i386 protected mode)
    dd multiboot_end - multiboot_start  ; Header length
    dd 0x100000000 - (0xe85250d6 + 0 + (multiboot_end - multiboot_start))  ; Checksum

    ; REMOVED framebuffer tag - we want VGA text mode, not graphics!

    ; End tag
    align 8
    dw 0                         ; Type: end
    dw 0                         ; Flags
    dd 8                         ; Size
multiboot_end:

align 16
_start:
    ; Disable interrupts
    cli

    ; TEST: Write to VGA immediately (before paging)
    mov dword [0xb8000], 0x4f544f53  ; 'ST' in white on red
    mov dword [0xb8004], 0x4f524f54  ; 'TR' in white on red

    ; Save multiboot info
    mov edi, ebx                 ; Multiboot info structure
    mov esi, eax                 ; Multiboot magic value

    ; Set up stack
    mov esp, stack_top

    ; Check for long mode support
    call check_cpuid
    call check_long_mode

    ; Set up page tables
    call setup_page_tables

    ; Enable PAE (Physical Address Extension)
    mov eax, cr4
    or eax, (1 << 5)             ; Set PAE bit
    mov cr4, eax

    ; Load PML4 address into CR3
    mov eax, boot_pml4
    mov cr3, eax

    ; Enable long mode in EFER MSR
    mov ecx, 0xC0000080          ; EFER MSR
    rdmsr
    or eax, (1 << 8)             ; Set LM bit
    wrmsr

    ; Enable paging
    mov eax, cr0
    or eax, (1 << 31)            ; Set PG bit
    mov cr0, eax

    ; Load 64-bit GDT
    lgdt [gdt64_pointer]

    ; Far jump to 64-bit code
    jmp 0x08:long_mode_start

check_cpuid:
    ; Check if CPUID is supported by attempting to flip the ID bit (bit 21)
    ; in the FLAGS register
    pushfd
    pop eax
    mov ecx, eax
    xor eax, (1 << 21)
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je .no_cpuid
    ret
.no_cpuid:
    mov al, 'C'
    jmp error

check_long_mode:
    ; Check for extended processor info
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode

    ; Check for long mode
    mov eax, 0x80000001
    cpuid
    test edx, (1 << 29)
    jz .no_long_mode
    ret
.no_long_mode:
    mov al, 'L'
    jmp error

setup_page_tables:
    ; Zero out page tables
    mov edi, boot_pml4
    mov ecx, 4096                ; 4 pages * 1024 dwords
    xor eax, eax
    rep stosd

    ; Set up IDENTITY mapping for first 1GB (needed during boot)
    ; PML4[0] -> PDPT
    mov eax, boot_pdpt
    or eax, 0b11                 ; Present + writable
    mov [boot_pml4], eax

    ; PDPT[0] -> PD
    mov eax, boot_pd
    or eax, 0b11                 ; Present + writable
    mov [boot_pdpt], eax

    ; First PD entry: point to page table for first 2MB (includes VGA)
    ; PD[0] -> boot_pt_low (NOT a huge page, use 4KB pages)
    mov eax, boot_pt_low
    or eax, 0b11                 ; Present + writable (NO huge page bit)
    mov [boot_pd], eax

    ; Map first 2MB using 512 * 4KB pages with write-through for VGA
    mov edi, boot_pt_low
    xor ebx, ebx                 ; Physical address counter
    mov ecx, 512                 ; Map 512 * 4KB pages = 2MB
.map_pt_low:
    mov eax, ebx
    or eax, 0x0b                 ; Present + writable + write-through (PWT=bit3)
    xor edx, edx                 ; High 32 bits
    mov [edi], eax
    mov [edi + 4], edx
    add ebx, 0x1000              ; Next 4KB page address
    add edi, 8                   ; Next page table entry
    loop .map_pt_low

    ; Map 2MB-1GB using 511 * 2MB huge pages (start from PD[1])
    mov edi, boot_pd + 8         ; Start at PD[1] (skip PD[0])
    mov eax, 0x200083            ; Start at 2MB, Present + writable + huge page
    xor edx, edx
    mov ecx, 511                 ; Map 511 * 2MB pages = 1022MB
.map_pd_huge:
    mov [edi], eax
    mov [edi + 4], edx
    add eax, 0x200000            ; Next 2MB page
    adc edx, 0
    add edi, 8
    loop .map_pd_huge

    ; ALSO set up HIGHER-HALF mapping at 0xFFFF800000000000
    ; PML4[256] -> same PDPT (maps higher half to same physical memory)
    mov eax, boot_pdpt
    or eax, 0b11                 ; Present + writable
    mov [boot_pml4 + 256*8], eax ; PML4 entry 256

    ret

error:
    ; Display error code in al
    mov dword [0xb8000], 0x4f524f45  ; 'ER' in red
    mov byte [0xb8004], al
    hlt
    jmp error

; 64-bit code starts here
BITS 64
section .text
long_mode_start:
    ; Load data segment selectors
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Send 'B' to serial port COM1 (0x3F8) - diagnostic
    mov dx, 0x3F8
    mov al, 'B'
    out dx, al

    ; Clear the screen (VGA text buffer)
    mov rdi, 0xb8000
    mov rcx, 80 * 25
    mov rax, 0x0f200f20          ; Two spaces with white on black
    rep stosq

    ; Send 'V' to serial - VGA clear done
    mov dx, 0x3F8
    mov al, 'V'
    out dx, al

    ; Write "BOOT OK" to top-left of screen for debugging
    ; Use register addressing to avoid 64-bit mode addressing issues
    mov rbx, 0xb8000
    mov dword [rbx], 0x0f4f0f42      ; 'BO' in white on black
    mov dword [rbx + 4], 0x0f540f4f  ; 'OT' in white on black
    mov dword [rbx + 8], 0x0f4f0f20  ; ' O' in white on black
    mov dword [rbx + 12], 0x07200f4b ; 'K ' in white on black

    ; Send 'K' to serial - VGA write done
    mov dx, 0x3F8
    mov al, 'K'
    out dx, al
    mov al, 0x0A  ; Newline
    out dx, al

    ; Call kernel main
    call kernel_main

    ; Halt if kernel returns
.hang:
    cli
    hlt
    jmp .hang

; 64-bit Global Descriptor Table
section .data
align 16
gdt64:
    dq 0                         ; Null descriptor
    dq 0x00af9a000000ffff        ; Code segment (64-bit)
    dq 0x00af92000000ffff        ; Data segment (64-bit)
gdt64_end:

gdt64_pointer:
    dw gdt64_end - gdt64 - 1     ; Limit
    dq gdt64                     ; Base
