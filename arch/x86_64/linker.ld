ENTRY(_start)

PHDRS
{
    text PT_LOAD FLAGS(5);
    rodata PT_LOAD FLAGS(4);
    data PT_LOAD FLAGS(6);
}

SECTIONS {
    . = 1M;

    .multiboot ALIGN(4) : {
        KEEP(*(.multiboot))
    } :text

    .text ALIGN(4) : {
        *(.text*)
    } :text

    . = ALIGN(4096);

    .rodata ALIGN(4) : {
        *(.rodata*)
    } :rodata

    . = ALIGN(4096);

    .data ALIGN(4) : {
        *(.data*)
    } :data

    .bss ALIGN(4) : {
        *(.bss*)
    } :data

    . = ALIGN(4096);
    kernel_end = .;
}
