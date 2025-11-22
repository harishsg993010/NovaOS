; NovaeOS x86_64 Bootstrap Code
; Sets up the initial environment and jumps to kernel_main()

BITS 32

; Multiboot2 header
section .multiboot
align 8
multiboot_start:
    dd 0xe85250d6                ; Magic number
    dd 0                         ; Architecture (0 = i386 protected mode)
    dd multiboot_end - multiboot_start  ; Header length
    dd 0x100000000 - (0xe85250d6 + 0 + (multiboot_end - multiboot_start))  ; Checksum

    ; Framebuffer tag (optional, for graphics mode)
    align 8
    dw 5                         ; Type: framebuffer
    dw 1                         ; Flags: optional
    dd 20                        ; Size
    dd 1024                      ; Width
    dd 768                       ; Height
    dd 32                        ; Depth

    ; End tag
    align 8
    dw 0                         ; Type: end
    dw 0                         ; Flags
    dd 8                         ; Size
multiboot_end:

; Bootstrap stack (16KB)
section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

; Temporary page tables for identity mapping and higher-half
align 4096
boot_pml4:
    resb 4096
boot_pdpt:
    resb 4096
boot_pd:
    resb 4096

; Code section
section .text
global _start
extern kernel_main

_start:
    ; Disable interrupts
    cli

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
    mov ecx, 3072                ; 3 pages * 1024 dwords
    xor eax, eax
    rep stosd

    ; Set up identity mapping and higher-half mapping
    ; PML4[0] -> PDPT (identity map first 2MB)
    mov eax, boot_pdpt
    or eax, 0b11                 ; Present + writable
    mov [boot_pml4], eax

    ; PDPT[0] -> PD
    mov eax, boot_pd
    or eax, 0b11                 ; Present + writable
    mov [boot_pdpt], eax

    ; Map first 2MB using 2MB pages
    mov eax, 0x83                ; Present + writable + huge page
    mov [boot_pd], eax

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

    ; Clear the screen (VGA text buffer)
    mov rdi, 0xb8000
    mov rcx, 80 * 25
    mov rax, 0x0f200f20          ; Two spaces with white on black
    rep stosq

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
