#include "trap.h"
#include "lib.h"
#include "printk.h"
#include "gate.h"
#include "memory.h"
#include "task.h"




void SysVectorInit(){
    /* Recoverable user faults use the owning task's kernel stack. Reserve
     * the emergency stack for exceptions from which the CPU cannot resume. */
    SetIntrGate(0,  0, divide_error);
    SetIntrGate(10, 1, invalid_TSS);
    SetIntrGate(14, 0, page_fault);
    SetIntrGate(13, 0, general_purpose);
    SetIntrGate(8,  1, double_fault);
    SetIntrGate(17, 0, alignment_check_fault);
    SetIntrGate(6,  0, undefined_opcode_fault);
}

static void exit_if_user_fault(unsigned long rsp, int trap, int status){
    struct PtRegs *regs = (void *)rsp;
    if((regs->cs & 3) == 3){
        sti();
        ColorPrintfk(RED, BLACK, "user trap %d pid %D at %p\n", trap, CURRENT->pid, (void *)regs->rip);
        DoExit(status);
    }
}

void DoDivideError(unsigned long rsp, unsigned long ErroCode){
    exit_if_user_fault(rsp, 0, 128 + 8);
    uint64_t* rip = (uint64_t *)(rsp + 0x98);
    ColorPrintfk(RED, BLACK, "Bad Division(0) In RIP: %p, RSP: %p, ERRCODE: %x \n", \
        (void *)*rip, (void *)rsp, ErroCode);
    while(1){
        hlt();
    }
}



void DoInvalidTss(unsigned long rsp, unsigned long ErroCode){
    uint64_t* rip = (uint64_t *)(rsp + 0x98);

    uint16_t Index               = ErroCode & 0xfff8;
    
    ColorPrintfk(RED, BLACK, "do_invalid_tss(10) In RIP: %p, RSP: %p, ERRCODE: %X \n", \
        rip, rsp, ErroCode);

    if(ErroCode & 0x1){
        ColorPrintfk(RED, BLACK, "This Exception Occurs During Delivery Of An Event External \
         To The Program, Such as an Interrupt or an earlier Exception RIP \n");
    }

    if(ErroCode & 0x2){
        ColorPrintfk(RED, BLACK, "Refers To The Descriptor In The Gate IDT Index In: %x \n", Index);
    }else if (ErroCode & 0x4){
        ColorPrintfk(RED, BLACK, "Refers To The Descriptor In The Gate LDT Index In: %x \n", Index);
    }else{
        ColorPrintfk(RED, BLACK, "Refers To The Descriptor In The Gate GDT Index In: %x \n", Index);
    }
    while(1){
        hlt();
    }
}



void DoPageFault(unsigned long rsp, unsigned long ErroCode){
    unsigned long cr2 = 0;
    __asm__ volatile("movq %%cr2,   %0":"=r"(cr2)::"memory");

    unsigned long *rip = (unsigned long *)(rsp + 0x98);

    if(((struct PtRegs *)rsp)->rflag & (1UL << 9)) sti();
    if(handle_page_fault(CURRENT->lmm, cr2, ErroCode) == 0)
        return;
    if((ErroCode & 4) && (((struct PtRegs *)rsp)->cs & 3) == 3){
        ColorPrintfk(RED, BLACK, "user page fault pid %d addr %p code %x\n",
                     CURRENT->pid, (void *)cr2, ErroCode);
        DoExit(128 + 11);
    }
    
    ColorPrintfk(RED, BLACK, "do_pageFault(14) In RIP: %p, RSP: %p, ERRCODE: %X \n", \
        (void *)*rip, (void *)rsp, ErroCode);
    
    if(ErroCode & 0x1){
        ColorPrintfk(RED, BLACK, "This Exception Occurs During Access An Protected Page \n");
    }else{
        ColorPrintfk(RED, BLACK, "This Exception Occurs Because Of Page Missing \n");
    }

    if(ErroCode & 0x2){
        ColorPrintfk(RED, BLACK, "This Exception Occurs During Writing A Page \n");
    }else{
        ColorPrintfk(RED, BLACK, "This Exception Occurs During Reading A Page \n");
    }

    if(ErroCode & 0x4){
        ColorPrintfk(RED, BLACK, "A Civilian Occurs This Exception \n");
    }else{
        ColorPrintfk(RED, BLACK, "A Super User Occurs This Exception What a Pity \n");
    }

    if(ErroCode & 0x8){
        ColorPrintfk(RED, BLACK, "Oh no I Can`t Translate this Addr Because Some Reserved Bit \
            are Set \n");
    }

    if(ErroCode & 0x10){
        ColorPrintfk(RED, BLACK, "This Exception Occurs During fetching An Instruction Or \
            CR4.SMEP = 1 Or IA32_EFER.NXE = 1 And CR4.PAE = 1 \n");
    }

    ColorPrintfk(RED, BLACK, "Page Fault Addr: %p \n", (void*)cr2);

    while(1) hlt();
}



void DoNmi(unsigned long rsp, unsigned long ErroCode){
    uint64_t* rip = (uint64_t *)(rsp + 0x98);
    ColorPrintfk(0xFF0000, 0x0000, "Bad Division In RIP: %p, RSP: %p, ERRCODE: %X\n", rip, rsp, ErroCode);
    while(1){
        hlt();
    }
}


void DoGeneralPurpose(unsigned long rsp, unsigned long ErroCode){
    exit_if_user_fault(rsp, 13, 128 + 11);
    uint64_t* rip = (uint64_t *)(rsp + 0x98);
    ColorPrintfk(0xFF0000, 0x0000, "GPF In RIP: %p, RSP: %p, ERRCODE: %X\n", (void *)*rip, (void *)rsp, ErroCode);
    while(1){
        hlt();
    }
}


void DoDoubleFault(unsigned long rsp, unsigned long ErroCode){
    uint64_t* rip = (uint64_t *)(rsp + 0x98);
    ColorPrintfk(0xFF0000, 0x0000, "DF In RIP: %p, RSP: %p, ERRCODE: %X\n", rip, rsp, ErroCode);
    while(1){
        hlt();
    }
}

void DoAlignmentCheckFault(unsigned long rsp, unsigned long ErroCode){
    exit_if_user_fault(rsp, 17, 128 + 7);
    uint64_t* rip = (uint64_t *)(rsp + 0x98);
    ColorPrintfk(0xFF0000, 0x0000, "ACF In RIP: %p, RSP: %p, ERRCODE: %X\n", (void *)*rip, (void *)rsp, ErroCode);
    while(1){
        hlt();
    }
}

void DoUndefinedOpcodeFault(unsigned long rsp, unsigned long ErroCode){
    exit_if_user_fault(rsp, 6, 128 + 4);
    uint64_t* rip = (uint64_t *)(rsp + 0x98);
    ColorPrintfk(0xFF0000, 0x0000, "UOF In RIP: %p, RSP: %p, ERRCODE: %X\n", (void *)*rip, (void *)rsp, ErroCode);
    while(1){
        hlt();
    }
}
