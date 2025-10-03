.data

# Page Table

.align  8
.org    0x1000

__PML4E:

    .quad   0x102003
    .fill   255,     8,  0
    .quad   0x102003
    .fill   255,     8,  0

.org    0x2000

__PDPTE:

    .quad   0x103003
    .fill   511,     8,  0

.org    0x3000

__PDE:

    .quad   0x000083
    .quad   0x200083
    .quad   0x400083
    .quad   0x600083
    .quad   0x800083
    .quad   0xe0000083 # 0xa00000  offset = 0x101 PDE[5]
    .quad   0xe0200083
    .quad   0xe0400083
    .quad   0xe0600083 # 0x1000000 offset = 0x1000 PDE[8]
    .quad   0xe0800083
    .quad   0xe0a00083
    .quad   0xe0c00083
    .quad   0xe0e00083
    .fill   499,     8,  0


.global     GdtTable


# Global Descriptor Table (GDT)

GdtPtr:

    .word GdtLen - 1
    .quad GdtTable

GdtTable:    
    .quad   0x0000000000000000  # NULL Offset 0
    .quad   0x0020980000000000  # Kernel Code Segment Offset 0x8
    .quad   0x0000920000000000  # Kernel Data Segment Offset 0x10
    .quad   0x0020f80000000000  # User Code Segment Offset 0x18
    .quad   0x0000f20000000000  # User Data Segment Offset 0x20
    .quad   0x00cf9a000000ffff  # Kernel Code Segment 32-bit Offset 0x28
    .quad   0x00cf92000000ffff  # Kernel Data Segment 32-bit Offset 0x30
    .fill   10, 8, 0             # TSS Offset 0x38


.set GdtLen,    . - GdtTable


.set SelectorCode64, 0x8
.set SelectorData64, 0x10


# Interrupt Descriptor Table (IDT)

.global     IdtTable

IdtPtr:             
    
    .word      IdtLen - 1
    .quad      IdtTable

IdtTable:
    
    .fill   512, 8, 0

.set IdtLen,     . - IdtTable


# Task State Segment Table (TSS)

.global     TssTable

TssPtr:

    .word      TssLen - 1
    .quad      TssTable

TssTable:
    
    .fill   10, 8, 0

.set TssLen,     . - TssTable

.text

.global     __start

__start:

    mov     $SelectorData64,    %ax

    mov     %ax,    %ds
    mov     %ax,    %es
    mov     %ax,    %fs
    mov     %ax,    %gs
    mov     %ax,    %ss

    mov     $0x7E00,    %esp  # Set stack pointer

    # Reload GDT

    lgdt    GdtPtr(%rip) # For the purpose of dynamic memory allocation

    # Reload IDT

    lidt    IdtPtr(%rip)

    # Reload Page Table

    movq    $0x101000,  %rax  # PML4 table physic address
    movq    %rax,   %cr3

    movq    switch_seg(%rip),  %rax
    pushq   $0x8
    pushq   %rax
    lretq

switch_seg:
    .quad entry_64

entry_64:

    movq $SelectorData64,    %rax

    movq    %rax,   %ds
    movq    %rax,   %es
    movq    %rax,   %fs
    movq    %rax,   %gs
    movq    %rax,   %ss

    movq    $0xffff800000007E00,    %rsp  # Set stack pointer

    movq    go_to_kernel(%rip), %rax
    pushq   $0x8
    pushq   %rax
    lretq

go_to_kernel:
    .quad  kernel_start

