OUTPUT_FORMAT("elf64-x86-64","elf64-x86-64","elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)

ENTRY(__start)

SECTIONS
{
    . = 0xffff800000000000 + 0x00100000;

    .text : {
        _text = .;
        KEEP(*(.text.boot))
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
        _bss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
        _end  = .;
    }

    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
    }
}