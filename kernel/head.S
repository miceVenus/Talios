#include "linkage.h"

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


    movq    $0x1b,  %rcx
    rdmsr
    bt      $8, %rax
    jnc     smp_entry

    movq    switch_seg(%rip),  %rax
    pushq   $0x8
    pushq   %rax
    lretq                       # in order to change register cs

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
	movq	%rax,	    80(%rdi)
	shrq	$32,	    %rdx
	movq	%rdx,	    88(%rdi)

	# mov	$0x50,	%ax
	# ltr	%ax
    
    movq    _stack_start(%rip),    %rsp  # Set stack pointer

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

smp_entry:

    lidt    IdtPtr(%rip)

    movq    _stack_start(%rip),    %rsp
    
    movq    go_to_smp_kernel(%rip), %rax
    pushq   $0x08
    pushq   %rax
    lretq

go_to_smp_kernel:
    .quad   start_smp

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

    .quad   0x103007
    .fill   511,     8,  0

.org    0x3000

__PDE:

    .quad   0x000087
    .quad   0x200087
    .quad   0x400087
    .quad   0x600087
    .quad   0x800087
    .quad   0xa00087
    .quad   0xc00087
    .quad   0xe00087
    .quad   0x1000087
    .quad   0x1200087
    .quad   0x1400087
    .quad   0x1600087
    .quad   0x1800087
    .quad   0x1a00087
    .quad   0x1c00087
    .quad   0x1e00087
    .quad   0x2000087
    .quad   0x2200087
    .quad   0x2400087
    .quad   0x2600087
    .quad   0x2800087
    .quad   0x2a00087
    .quad   0x2c00087
    .quad   0x2e00087
    .quad   0xe0000087 # 0x3000000 framebuffer start 0xe0000000
    .quad   0xe0200087
    .quad   0xe0400087
    .quad   0xe0600087 # 0x3600000 
    .quad   0xe0a00087
    .quad   0xe0c00087
    .quad   0xe0e00087
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
    .quad   0x0020980000000000  # Kernel Code Segment Offset 0x8 Would Be Changed After Access  64
    .quad   0x0000920000000000  # Kernel Data Segment Offset 0x10                               64
    .quad   0x0000000000000000  # User Code Segment Offset  0x18                                32
    .quad   0x0000000000000000  # User Data Segment Offset  0x20                                32
    .quad   0x0020f80000000000  # User Code Segment Offset 0x28                                 64
    .quad   0x0000f20000000000  # User Data Segment Offset 0x30                                 64
    .quad   0x00cf9a000000ffff  # Kernel Code Segment 32-bit Offset 0x38                        32
    .quad   0x00cf92000000ffff  # Kernel Data Segment 32-bit Offset 0x40                        32

    .fill   100, 8, 0            # TSS Offset 0x50   jmp 0xa


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

; TssPtr:

;     .word      TssLen - 1
;     .quad      TssTable

.align 8
TssTable:
    
    .fill   13, 8, 0

.set TssLen,     . - TssTable

ENTRY(_stack_start)
    .quad   InitTaskUnion + 32768

    
ignore_msg: 
    .asciz  "Not Implemented Exception Or Interrupt!!!! I'm Gone PLZ \n"