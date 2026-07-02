OUTPUT_FORMAT("elf64-x86-64","elf64-x86-64","elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)

ENTRY(main)

SECTIONS
{
    . = 0x800000;

    .text : {
        _text = .;
        build/init_user.o(main)
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

    .bss : {
        _bss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
        _end  = .;
    }
}