; Interrupt Service Routine (ISR) Stubs
; These are the low-level assembly handlers that save state and call C handlers

BITS 64

; External C function
extern isr_common_handler
extern pic_send_eoi

; Common ISR stub - saves all registers
global isr_common_stub
isr_common_stub:
    ; Save all general purpose registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Save segment registers
    mov rax, ds
    push rax
    mov rax, es
    push rax
    mov rax, fs
    push rax
    mov rax, gs
    push rax

    ; Load kernel data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Call C handler (passing pointer to register struct)
    mov rdi, rsp
    call isr_common_handler

    ; Restore segment registers
    pop rax
    mov gs, rax
    pop rax
    mov fs, rax
    pop rax
    mov es, rax
    pop rax
    mov ds, rax

    ; Restore general purpose registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; Remove error code and interrupt number
    add rsp, 16

    ; Return from interrupt
    iretq

; Macro for ISRs without error code
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push 0                  ; Push dummy error code
    push %1                 ; Push interrupt number
    jmp isr_common_stub
%endmacro

; Macro for ISRs with error code (CPU pushes it automatically)
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    push %1                 ; Push interrupt number
    jmp isr_common_stub
%endmacro

; Macro for IRQs
%macro IRQ 2
global irq%1
irq%1:
    push 0                  ; Dummy error code
    push %2                 ; Interrupt number (32 + IRQ number)
    jmp isr_common_stub
%endmacro

; CPU Exception Handlers (0-31)
ISR_NOERRCODE 0             ; Divide by zero
ISR_NOERRCODE 1             ; Debug
ISR_NOERRCODE 2             ; NMI
ISR_NOERRCODE 3             ; Breakpoint
ISR_NOERRCODE 4             ; Overflow
ISR_NOERRCODE 5             ; Bound range exceeded
ISR_NOERRCODE 6             ; Invalid opcode
ISR_NOERRCODE 7             ; Device not available
ISR_ERRCODE   8             ; Double fault (has error code)
ISR_NOERRCODE 9             ; Coprocessor segment overrun
ISR_ERRCODE   10            ; Invalid TSS (has error code)
ISR_ERRCODE   11            ; Segment not present (has error code)
ISR_ERRCODE   12            ; Stack-segment fault (has error code)
ISR_ERRCODE   13            ; General protection fault (has error code)
ISR_ERRCODE   14            ; Page fault (has error code)
ISR_NOERRCODE 15            ; Reserved
ISR_NOERRCODE 16            ; x87 FPU error
ISR_ERRCODE   17            ; Alignment check (has error code)
ISR_NOERRCODE 18            ; Machine check
ISR_NOERRCODE 19            ; SIMD floating-point exception
ISR_NOERRCODE 20            ; Virtualization exception
ISR_NOERRCODE 21            ; Reserved
ISR_NOERRCODE 22            ; Reserved
ISR_NOERRCODE 23            ; Reserved
ISR_NOERRCODE 24            ; Reserved
ISR_NOERRCODE 25            ; Reserved
ISR_NOERRCODE 26            ; Reserved
ISR_NOERRCODE 27            ; Reserved
ISR_NOERRCODE 28            ; Reserved
ISR_NOERRCODE 29            ; Reserved
ISR_ERRCODE   30            ; Security exception (has error code)
ISR_NOERRCODE 31            ; Reserved

; IRQ Handlers (32-47)
IRQ 0, 32                   ; Timer (IRQ0)
IRQ 1, 33                   ; Keyboard (IRQ1)
IRQ 2, 34                   ; Cascade (IRQ2)
IRQ 3, 35                   ; COM2 (IRQ3)
IRQ 4, 36                   ; COM1 (IRQ4)
IRQ 5, 37                   ; LPT2 (IRQ5)
IRQ 6, 38                   ; Floppy (IRQ6)
IRQ 7, 39                   ; LPT1 (IRQ7)
IRQ 8, 40                   ; RTC (IRQ8)
IRQ 9, 41                   ; ACPI (IRQ9)
IRQ 10, 42                  ; Available (IRQ10)
IRQ 11, 43                  ; Available (IRQ11)
IRQ 12, 44                  ; Mouse (IRQ12)
IRQ 13, 45                  ; FPU (IRQ13)
IRQ 14, 46                  ; Primary ATA (IRQ14)
IRQ 15, 47                  ; Secondary ATA (IRQ15)

; Syscall handler (0x80)
ISR_NOERRCODE 128
