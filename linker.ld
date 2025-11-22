/* NovaeOS Kernel Linker Script
 * Defines the memory layout for the x86_64 kernel
 */

/* Entry point */
ENTRY(_start)

/* Define sections */
SECTIONS
{
    /* Kernel starts at 1MB physical address */
    . = 0x100000;

    _kernel_start = .;

    /* Multiboot header must be early in the file */
    .boot :
    {
        *(.multiboot)
    }

    /* Read-only code section */
    .text ALIGN(4K) : AT(ADDR(.text))
    {
        _text_start = .;
        *(.text)
        *(.text.*)
        _text_end = .;
    }

    /* Read-only data section */
    .rodata ALIGN(4K) : AT(ADDR(.rodata))
    {
        _rodata_start = .;
        *(.rodata)
        *(.rodata.*)
        _rodata_end = .;
    }

    /* Initialized data section */
    .data ALIGN(4K) : AT(ADDR(.data))
    {
        _data_start = .;
        *(.data)
        *(.data.*)
        _data_end = .;
    }

    /* Uninitialized data section (BSS) */
    .bss ALIGN(4K) : AT(ADDR(.bss))
    {
        _bss_start = .;
        *(COMMON)
        *(.bss)
        *(.bss.*)
        _bss_end = .;
    }

    _kernel_end = .;

    /* Discard unnecessary sections */
    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note.*)
    }
}
