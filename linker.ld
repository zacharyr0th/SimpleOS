/* This is the linker script for SimpleOS, defining the memory layout of the kernel.
   It specifies the entry point and organizes various sections of the kernel in memory. */

/* SimpleOS/linker.ld */
OUTPUT_FORMAT("elf64-x86-64")
ENTRY(kernel_main)

SECTIONS {
    . = 1M;

    .text : {
        *(.multiboot)
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(COMMON)
        *(.bss)
    }
}