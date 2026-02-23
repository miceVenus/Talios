#include "linkage.h"

R15 = 0x00
R14 = 0x08
R13 = 0x10
R12 = 0x18
R11 = 0x20
R10 = 0x28
R9 = 0x30
R8 = 0x38
RBX = 0x40
RCX = 0x48
RDX = 0x50
RSI = 0x58
RDI = 0x60
RBP = 0x68
DS = 0x70
ES = 0x78
RAX = 0x80
FUNC = 0x88
ERRCODE = 0x90
RIP = 0x98
CS = 0xa0
RFLAGS = 0xa8
OLDRSP = 0xb0
OLDSS = 0xb8

ENTRY(Syscall)
    sti
    subq    $0x38,  %rsp
    cld

    pushq   %rax
    movq    %es,    %rax
    push    %rax
    movq    %ds,    %rax
    pushq   %rax
    pushq   %rbp
    pushq   %rdi
    pushq   %rsi
    pushq   %rdx
    pushq   %rcx
    pushq   %rbx
    pushq   %r8
    pushq   %r9
    pushq   %r10
    pushq   %r11
    pushq   %r12
    pushq   %r13
    pushq   %r14
    pushq   %r15

    movq    $0x10,  %rdx
    movq    %rdx,   %ds
    movq    %rdx,   %es

    movq    %rsp,   %rdi
    callq   SystemCallFunc



ENTRY(ret_system_call)
    movq    %rax,   0x80(%rsp)
    popq    %r15
    popq    %r14
    popq    %r13
    popq    %r12
    popq    %r11
    popq    %r10
    popq    %r9
    popq    %r8
    popq    %rbx
    popq    %rcx
    popq    %rdx
    popq    %rsi
    popq    %rdi
    popq    %rbp
    popq    %rax
    movq    %rax,   %ds
    popq    %rax
    movq    %rax,   %es
    popq    %rax
    addq    $0x38,  %rsp

    .byte   0x48
    sysexit

ENTRY(divide_error)
    pushq   $0                      # divide error's error code
    pushq   %rax
    leaq    DoDivideError(%rip),    %rax

    xchgq   %rax,   (%rsp)          # restore rax and push func into stack

    jmp     error_code

ENTRY(invalid_TSS)
    pushq   %rax
    leaq    DoInvalidTss(%rip),     %rax

    xchgq   %rax,   (%rsp)

    jmp     error_code

ENTRY(page_fault)
    pushq   %rax
    leaq    DoPageFault(%rip),      %rax

    xchgq   %rax,   (%rsp)

    jmp     error_code

ENTRY(general_purpose)
    pushq   %rax
    leaq    DoGeneralPurpose(%rip),      %rax

    xchgq   %rax,   (%rsp)

    jmp     error_code

ENTRY(double_fault)
    pushq   %rax
    leaq    DoDoubleFault(%rip),      %rax

    xchgq   %rax,   (%rsp)

    jmp     error_code

ENTRY(alignment_check_fault)
    pushq   %rax
    leaq    DoAlignmentCheckFault(%rip),      %rax

    xchgq   %rax,   (%rsp)

    jmp     error_code

ENTRY(undefined_opcode_fault)
    pushq   $0 
    pushq   %rax
    leaq    DoUndefinedOpcodeFault(%rip),      %rax

    xchgq   %rax,   (%rsp)

    jmp     error_code

ENTRY(nmi)

    pushq   %rax
    movq    %es,    %rax
    pushq   %rax
    movq    %ds,    %rax
    pushq   %rax

    pushq   %rbp
    pushq   %rdi
    pushq   %rsi
    pushq   %rdx
    pushq   %rcx
    pushq   %rbx
    pushq   %r8
    pushq   %r9
    pushq   %r10
    pushq   %r11
    pushq   %r12
    pushq   %r13
    pushq   %r14
    pushq   %r15

    cld

    movq    $0x10,          %rdx
    movq    %rdx,           %es
    movq    %rdx,           %ds

    movq    $0x0,           %rsi
    movq    %rsp,           %rdi

    callq   DoNmi   

    jmp     ret_from_exception



restore_all:
    popq    %r15
    popq    %r14
    popq    %r13
    popq    %r12
    popq    %r11
    popq    %r10
    popq    %r9
    popq    %r8
    popq    %rbx
    popq    %rcx
    popq    %rdx
    popq    %rsi
    popq    %rdi
    popq    %rbp
    popq    %rax
    movq    %rax,   %ds
    popq    %rax
    movq    %rax,   %es
    popq    %rax
    addq    $0x10,  %rsp # CLEAR FUNC AND ERROR CODE
    iretq

error_code:
    pushq   %rax
    movq    %es,    %rax
    pushq   %rax
    movq    %ds,    %rax
    pushq   %rax

    pushq   %rbp
    pushq   %rdi
    pushq   %rsi
    pushq   %rdx
    pushq   %rcx
    pushq   %rbx
    pushq   %r8
    pushq   %r9
    pushq   %r10
    pushq   %r11
    pushq   %r12
    pushq   %r13
    pushq   %r14
    pushq   %r15

    cld
    movq    ERRCODE(%rsp),  %rsi
    movq    FUNC(%rsp),     %rdx

    movq    $0x10,          %rdi
    movq    %rdi,           %ds
    movq    %rdi,           %es
    movq    %rsp,           %rdi

    callq   *%rdx

    jmp     ret_from_exception

ret_from_exception:

ENTRY(ret_from_intr)
    movq    $-1,                    %rcx
    testq   softirq_status(%rip),   %rcx
    jnz     softirq_handler
    jmp     restore_all

softirq_handler:
    callq   do_softirq
    jmp     restore_all