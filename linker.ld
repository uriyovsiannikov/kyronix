OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(kmain)

PHDRS {
    text   PT_LOAD FLAGS((1 << 0) | (1 << 2));
    rodata PT_LOAD FLAGS((1 << 2));
    data   PT_LOAD FLAGS((1 << 1) | (1 << 2));
}

SECTIONS {
    . = 0xffffffff80000000;

    __kernel_start = .;

    .text : {
        *(.text .text.*)
    } :text

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    .limine_requests_start : {
        KEEP(*(.limine_requests_start))
    } :rodata

    .limine_requests : {
        KEEP(*(.limine_requests))
    } :rodata

    .limine_requests_end : {
        KEEP(*(.limine_requests_end))
    } :rodata

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .data : {
        *(.data .data.*)
    } :data

    .bss : {
        *(COMMON)
        *(.bss .bss.*)
    } :data

    __kernel_end = .;

    /DISCARD/ : {
        *(.eh_frame*)
        *(.note .note.*)
        *(.comment)
    }
}
