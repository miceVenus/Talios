.text 

.global     __start

__start:

    mov     $SelectorData64,    %rax

    mov     %rax,    %ds
    mov     %rax,    %es
    mov     %rax,    %fs
    mov     %rax,    %gs
    mov     %rax,    %ss

    mov     $0x7E00,    %esp  # Set stack pointer

    # Reload GDT

    lgdt    GdtPtr(%rip) # For the purpose of dynamic memory allocation

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

    movq    $SelectorData64,    %rax

    movq    %rax,   %ds
    movq    %rax,   %es
    movq    %rax,   %fs
    movq    %rax,   %gs
    movq    %rax,   %ss

setup_IDT:
    leaq    ignore_IDT(%rip),   %rdx
    mov     %dx,    %ax
    shrq    $16,    %rdx
    mov     %dx,    %cx
    shrq    $16,    %rdx
    shlq    $48,    %rcx
    addq    %rcx,   %rax
    mov     $(0x8e00 << 32),    %rcx
    addq    %rcx,   %rax
    mov     $(0x8 << 16),        %rcx
    addq    %rcx,   %rax        # Init IDT rdx:rax

    leaq    IdtTable(%rip),     %rdi
    mov     $256,   %rcx

    st_IDT:

        dec     %rcx
        movq    %rax,   (%rdi)
        movq    %rdx,   8(%rdi)
        addq    $0x10,  %rdi
        cmp     $0,     %rcx
        jne     st_IDT

    # Reload IDT after setup

    lidt    IdtPtr(%rip)

setup_TSS64:
	leaq	TssTable(%rip),	    %rdx
	xorq	%rax,	    %rax
	xorq	%rcx,	    %rcx
	movq	$0x89,	    %rax
	shlq	$40,	    %rax
	movl	%edx,	    %ecx
	shrl	$24,	    %ecx
	shlq	$56,	    %rcx
	addq	%rcx,	    %rax
	xorq	%rcx,	    %rcx
	movl	%edx,	    %ecx
	andl	$0xffffff,	%ecx
	shlq	$16,	    %rcx
	addq	%rcx,	    %rax
	addq	$103,	    %rax
	leaq	GdtTable(%rip),	    %rdi
	movq	%rax,	    64(%rdi)
	shrq	$32,	    %rdx
	movq	%rdx,	    72(%rdi)

	# mov	$0x40,	%ax
	# ltr	%ax
    
    movq    $0xffff800000007E00,    %rsp  # Set stack pointer

    movq    go_to_kernel(%rip), %rax
    pushq   $0x8
    pushq   %rax
    lretq

go_to_kernel:
    .quad  main





ignore_IDT:
    cld
    pushq   %rbx
    pushq   %rbp
    pushq   %r12
    pushq   %r13
    pushq   %r14
    pushq   %r15

    movq    %es,    %rax
    pushq   %rax
    movq    %ds,    %rax
    pushq   %rax

    movq    $0x10,  %rax    # Just For Safe When Privilege level Switch
    movq    %rax,   %es
    movq    %rax,   %ds

    mov     $0xff0000,      %rdi
    mov     $0x0,           %rsi
    leaq    ignore_msg(%rip),   %rdx

    movq    $0x0,   %rax    # Must Be Set !!!!!!!!!

    callq   ColorPrintfk    # system ABI request Stack must be aligned by 16B When calling C

    loop:   
        jmp loop

    popq    %rax
    movq    %rax,   %ds
    popq    %rax
    movq    %rax,   %es

    popq    %r15
    popq    %r14
    popq    %r13
    popq    %r12
    popq    %rbp
    popq    %rbx

    iretq
# Page Table

.align  8
.org    0x1000

__PML4E:

    .quad   0x102007
    .fill   255,     8,  0
    .quad   0x102007
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
    .quad   0xe0000083 # 0xa00000  offset = 0x101 PDE[5] framebuffer start 0xfd000000
    .quad   0xe0200083
    .quad   0xe0400083
    .quad   0xe0600083 # 0x1000000 offset = 0x1000 PDE[8]
    .quad   0xe0800083
    .quad   0xe0a00083
    .quad   0xe0c00083
    .quad   0xe0e00083
    .fill   499,     8,  0



.data

.global     GdtTable


# Global Descriptor Table (GDT)

GdtPtr:

    .word GdtLen - 1
    .quad GdtTable

.align 8
GdtTable:    
    .quad   0x0000000000000000  # NULL Offset 0
    .quad   0x0020980000000000  # Kernel Code Segment Offset 0x8 Would Be Changed After Access
    .quad   0x0000920000000000  # Kernel Data Segment Offset 0x10
    .quad   0x0020f80000000000  # User Code Segment Offset 0x18
    .quad   0x0000f20000000000  # User Data Segment Offset 0x20
    .quad   0x00cf9a000000ffff  # Kernel Code Segment 32-bit Offset 0x28
    .quad   0x00cf92000000ffff  # Kernel Data Segment 32-bit Offset 0x30
    .fill   10, 8, 0             # TSS Offset 0x40 jump 0x7


.set GdtLen,    . - GdtTable


.set SelectorCode64, 0x8
.set SelectorData64, 0x10


# Interrupt Descriptor Table (IDT)

.global     IdtTable

IdtPtr:             
    
    .word      IdtLen - 1
    .quad      IdtTable

.align 8
IdtTable:
    
    .fill   512, 8, 0

.set IdtLen,     . - IdtTable


# Task State Segment Table (TSS)

.global     TssTable

TssPtr:

    .word      TssLen - 1
    .quad      TssTable

.align 8
TssTable:
    
    .fill   13, 8, 0

.set TssLen,     . - TssTable

ignore_msg: 
    .asciz  "Not Implemented Exception Or Interrupt!!!! I'm Gone PLZ \n"