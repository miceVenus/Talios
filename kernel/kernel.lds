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

    . = ALIGN(8);

    .rodata : {
        _rodata  = .;
        *(.rodata*)
        _erodata = .;
    }

    .data : {
        _data = .;
        *(.data)
        _edata = .;
    }

    . = ALIGN(32768);
    
    .data.init_task : {
        *(.data.init_task)
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