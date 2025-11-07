#include "linkage.h"
#include "gate.h"
#include "printk.h"
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
                "leaq   DoIRQ(%rip), %rax;"             \
                "pushq  %rax;"                          \
                SAVE_ALL_REGS                           \
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
    
void (* interrupt[24]) (void) = {
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

void InterruptInit(){
    for(unsigned int i = 32; i < (32 + 24); i++){
        SetIntrGate(i, 2, interrupt[i-32]);
    }

    ColorPrintfk(BLUE, BLACK, "PIC 8259A Init\n");

    // Init Master PIC ICW1 ~ 4 Good Chip Have A "Smart" Protocol To
    // Select Right Register

    OUT8b(0x20, 0x11);
    OUT8b(0x21, 0x20);
    OUT8b(0x21, 0x04);
    OUT8b(0x21, 0x01);

    // Init Slave PIC ICW1 ~ 4

    OUT8b(0xa0, 0x11);
    OUT8b(0xa1, 0x28);
    OUT8b(0xa1, 0x02);
    OUT8b(0xa1, 0x01);

    // Init Master/Slave PIC OCW1 

    // There Is No Timer Interrupt
    OUT8b(0x21, 0x01);
    OUT8b(0xa1, 0x00);

    sti();
}

// Uncompleted !!!!!
void DoIRQ(unsigned long rsp, unsigned long nr){
    switch (nr){
    case 0x21: {
            unsigned char KBCode = IN8b(0x60);
            ColorPrintfk(BLUE, BLACK, "get keyboard code %x\n", KBCode);
        };
        break;
    
    default:
        break;
    }
    OUT8b(0x20, 0x20); // Send INTR To CPU Rest ISR
}
