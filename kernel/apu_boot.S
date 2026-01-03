#include "linage.h"

.balign 0x1000

.text
.code16

ENTRY(_APU_boot_start)

_APU_boot_base = .

    cli
    wbinvd

    mov %cs,    %ax
    mov %ax,    %ds
    mov %ax,    %es
    mov %ax,    %ss
    mov %ax,    %fs
    mov %ax,    %gs

    movl    $(_APU_boot_tmp_stack_end - _APU_boot_base),    %esp

    mov %cs,    %ax
    movzx   %ax,    %esi
    shll    $4,     %esi

    # set gdt and 32&64 code address

    leal    (_APU_code32 - _APU_boot_base)(%esi),   %eax
    movl    %eax,   (_APU_code32_vector - _APU_boot_base)


    leal    (_APU_code64 - _APU_boot_base)(%esi),   %eax
    movl    %eax,   (_APU_code64_vector - _APU_boot_base)

    leal    (_APU_tmp_gdt - _APU_boot_base)(%esi),  %eax
    movl    %eax,   (_APU_tmp_gdt + 2 - _APU_boot_base)

    # load idt gdt

    lidtl   _APU_tmp_idt - _APU_boot_base
    lgdtl   _APU_tmp_gdt - _APU_boot_base

    # enable protected mode

    smsw    %ax
    bts     $0, %ax
    lmsw    %ax

    # go to 32 code 

    ljmpl   *(_APU_code32_vector - _APU_boot_base)


.code32
.balign 4
_APU_code32:

    mov $0x10,  %ax
    mov %ax,    %ds
    mov %ax,    %es
    mov %ax,    %ss
    mov %ax,    %fs
    mov %ax,    %gs

    leal    (_APU_boot_tmp_stack_end - _APU_boot_base)(%esi), %eax
    movl    %eax,   %esp

    # open pae

    movl    %cr4,   %eax
    bts     $5,     %eax
    movl    %eax,   %cr4

    # set page table

    movl    $0x90000,   %eax,
    movl    %eax,    %cr3

    # enable long mode
    movl    $0xC0000080,    %eax    
    rdmsr

    or      $0x00000100,    %eax
    wrmsr

    # enable PE & paging

    movl    %cr0,   %eax
    bts     $0,     %eax 
    bts     $31,    %eax        
    movl    %eax,   %cr0

    ljmp    *(_APU_code64 - _APU_boot_base)(%esi)


.code64
.balign 4
_APU_code64:

    movq    $0x20,  %rax
    movq    %rax,   %ds
    movq    %rax,   %es
    movq    %rax,   %fs
    movq    %rax,   %gs
    movq    %rax,   %es

    hlt


.balign 4

_APU_tmp_idt:
    .word   0
    .wr