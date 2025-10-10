ENTRY(__start)

SECTIONS
{
    . = 0xffff800000000000 + 0x00100000;

    .text : {
        build/head.o(__start)
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        *(.bss*)
        *(COMMON)
    }

    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
    }
}