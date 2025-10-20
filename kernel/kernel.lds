ENTRY(__start)

SECTIONS
{
    . = 0xffff800000000000 + 0x00100000;

    .text : {
        _text = .;
        build/head.o(__start)
        *(.text*)
        _etext = .;
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
        _edata = .;
    }

    .bss : {
        *(.bss*)
        *(COMMON)
        _end = .;
    }

    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
    }
}