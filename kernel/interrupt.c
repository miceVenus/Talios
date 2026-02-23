#include "linkage.h"
#include "gate.h"
#include "printk.h"
#include "interrupt.h"
#include "memory.h"
#include "apic.h"
#include "8259a.h"
#include "lib.h"

#define SAVE_ALL_REGS    \
        "cld;"                  \
        "pushq  %rax;"         \
        "movq   %es,   %rax;" \
        "pushq  %rax;"         \
        "movq   %ds,   %rax;" \
        "pushq  %rax;"         \
        "pushq  %rbp;"         \
        "pushq  %rdi;"         \
        "pushq  %rsi;"         \
        "pushq  %rdx;"         \
        "pushq  %rcx;"         \
        "pushq  %rbx;"         \
        "pushq  %r8;"          \
        "pushq  %r9;"          \
        "pushq  %r10;"         \
        "pushq  %r11;"         \
        "pushq  %r12;"         \
        "pushq  %r13;"         \
        "pushq  %r14;"         \
        "pushq  %r15;"         \
        "movq   $0x10,  %rdx;" \
        "movq   %rdx,  %es;"  \
        "movq   %rdx,  %ds;"

#define IRQ_NAME_(nr)   nr##_interrupt(void)
#define IRQ_NAME(nr)    IRQ_NAME_(IRQ##nr)

#define BUILD_IRQ(nr)                                   \
    void IRQ_NAME(nr);                                  \
    __asm__ (   SYMBOL_NAME_STR(IRQ)#nr"_interrupt:"    \
                "pushq  $0x00;"                         \
                "pushq  %rax;"                          \
                SAVE_ALL_REGS                           \
                "leaq   DoIRQ(%rip), %rax;"             \
                "movq   %rsp,  %rdi;"                   \
                "movq   $"#nr", %rsi;"                  \
                "leaq   ret_from_intr(%rip), %rax;"     \
                "pushq  %rax;"                          \
                "jmp    DoIRQ");

BUILD_IRQ(0x20);
BUILD_IRQ(0x21);
BUILD_IRQ(0x22);
BUILD_IRQ(0x23);
BUILD_IRQ(0x24);
BUILD_IRQ(0x25);
BUILD_IRQ(0x26);
BUILD_IRQ(0x27);
BUILD_IRQ(0x28);
BUILD_IRQ(0x29);
BUILD_IRQ(0x2a);
BUILD_IRQ(0x2b);
BUILD_IRQ(0x2c);
BUILD_IRQ(0x2d);
BUILD_IRQ(0x2e);
BUILD_IRQ(0x2f);
BUILD_IRQ(0x30);
BUILD_IRQ(0x31);
BUILD_IRQ(0x32);
BUILD_IRQ(0x33);
BUILD_IRQ(0x34);
BUILD_IRQ(0x35);
BUILD_IRQ(0x36);
BUILD_IRQ(0x37);
BUILD_IRQ(0xc8);
BUILD_IRQ(0xc9);
BUILD_IRQ(0xca);
BUILD_IRQ(0xcb);
BUILD_IRQ(0xcc);
BUILD_IRQ(0xcd);
BUILD_IRQ(0xce);
BUILD_IRQ(0xcf);
BUILD_IRQ(0xd0);
BUILD_IRQ(0xd1);
    
interrupt_t interrupt[NR_IRQS] = {
    IRQ0x20_interrupt,
    IRQ0x21_interrupt,
    IRQ0x22_interrupt,
    IRQ0x23_interrupt,
    IRQ0x24_interrupt,
    IRQ0x25_interrupt,
    IRQ0x26_interrupt,
    IRQ0x27_interrupt,
    IRQ0x28_interrupt,
    IRQ0x29_interrupt,
    IRQ0x2a_interrupt,
    IRQ0x2b_interrupt,
    IRQ0x2c_interrupt,
    IRQ0x2d_interrupt,
    IRQ0x2e_interrupt,
    IRQ0x2f_interrupt,
    IRQ0x30_interrupt,
    IRQ0x31_interrupt,
    IRQ0x32_interrupt,
    IRQ0x33_interrupt,
    IRQ0x34_interrupt,
    IRQ0x35_interrupt,
    IRQ0x36_interrupt,
    IRQ0x37_interrupt,
};

interrupt_t smp_interrupt[SMP_IPI_IRQS] = {
    IRQ0xc8_interrupt,
    IRQ0xc9_interrupt,
    IRQ0xca_interrupt,
    IRQ0xcb_interrupt,
    IRQ0xcc_interrupt,
    IRQ0xcd_interrupt,
    IRQ0xce_interrupt,
    IRQ0xcf_interrupt,
    IRQ0xd0_interrupt,
    IRQ0xd1_interrupt,
};

IrqDescT InterruptDesc[NR_IRQS] = {0};

IrqDescT smp_ipi_desc[SMP_IPI_IRQS] = {0};

void DefaultEnable(unsigned long irq){
    #ifdef APIC
        ApicEnable(irq);
    #else
        // Clear Mask Bit 8259a PIC
        ClearMask8259a(irq - 0x20);
    #endif
}

void DefaultAck(unsigned long irq){

    #ifdef APIC
        // Send EOI TO IOAPIC
        ApicAck(irq);
    #else
        // Send EOI TO 8259a PIC
        Ack8259a(irq);
    #endif

}

void DefaultInstall(unsigned long irq, void * arg){
    #ifdef APIC
        // Enable Interrupt Vector[irq]
        ApicInstall(irq, arg);
    #else
        // Send EOI TO 8259a PIC
        ClearMask8259a(irq - 0x20);
    #endif
}

void DefaultUninstall(unsigned long irq){
    #ifdef APIC
        ApicUninstall(irq);
    #else
        // Send EOI TO 8259a PIC
        SetMask8259a(irq - 0x20);
    #endif
}
void DefaultDisable(unsigned long irq){
    #ifndef APIC
        // disable Interrupt Vector[irq]
        ApicDisable(irq);
    #else
        // Send EOI TO 8259a PIC
        SetMask8259a(irq - 0x20);
    #endif
}


void BuildController(HwInterruptT * Controller){

    Controller->enable  = &DefaultEnable;
    Controller->ack     = &DefaultAck;
    Controller->disable = &DefaultDisable;
    Controller->install = &DefaultInstall;
    Controller->uninstall = &DefaultUninstall;

}

int RegisterIrq(unsigned long irq, void *arg, void (*handler)(struct PtRegs *regs, unsigned long nr, unsigned long arg),
                unsigned long parameter, HwInterruptT * controller, char *IrqName){
    IrqDescT * p = &InterruptDesc[irq - 32];
    p->handler = handler;
    p->parameter = parameter;
    p->IrqName = IrqName;
    p->flags = 0;
    p->controller = controller;

    p->controller->install(irq, arg);
    p->controller->enable(irq);

    return 1;
}

int UnregisterIrq(unsigned long irq){

    IrqDescT * p = &InterruptDesc[irq - 32];

    p->controller->disable(irq);
    p->controller->uninstall(irq);

    p->parameter = 0;
    p->IrqName = NULL;
    p->flags = 0;
    p->handler = NULL;
    p->controller = NULL;

    return 1;
}

// Uncompleted !!!!!
void DoIRQ(struct PtRegs * regs, unsigned long nr){

    IrqDescT * irq;

    if(nr <= 0x80){
        irq = &InterruptDesc[nr - 32];
        ColorPrintfk(BLUE, BLACK, "normal have IRQ nr : %D", nr);
    }else{
        irq = &smp_ipi_desc[nr - 200];
        ColorPrintfk(BLUE, BLACK, "smp ipi have IRQ nr : %D", nr);
        wrmsr(EOIR_MSR, 0x0);
    }

    if(irq->handler != NULL) irq->handler(regs, nr, irq->parameter);
    if(irq->controller && irq->controller->ack) irq->controller->ack(nr);

    // OUT8b(0x20, 0x20); // Send INTR To CPU R 8259a

    // bochs_bp();
}
