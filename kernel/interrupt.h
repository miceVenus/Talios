#ifndef INTERRUPT_H
#define INTERRUPT_H

#define NR_IRQS 24
#define SMP_IPI_IRQS 10

#include "task.h"

typedef void (*interrupt_t)(void);

typedef struct HwInterruptT{
    void (*enable)(unsigned long irq);
    void (*disable)(unsigned long irq);

    unsigned long (*install)(unsigned long irq, void *arg);
    void (*uninstall)(unsigned long irq);

    void (*ack)(unsigned long irq);
}HwInterruptT;

typedef struct IrqDescT{
    HwInterruptT * controller;
    char * IrqName;
    unsigned long parameter;
    void (*handler)(struct PtRegs * regs, unsigned long nr, unsigned long arg);
    unsigned long flags;
}IrqDescT;


extern interrupt_t smp_interrupt[SMP_IPI_IRQS];
extern IrqDescT smp_ipi_desc[SMP_IPI_IRQS];

int RegisterIrq(unsigned long irq, void *arg, void (*handler)(struct PtRegs* regs, unsigned long nr, unsigned long arg),
                unsigned long parameter, HwInterruptT * controller, char *IrqName);

int regitser_ipi(unsigned long irq, void *arg, void (*handler)(struct PtRegs *regs, unsigned long nr, unsigned long arg),
                unsigned long parameter, HwInterruptT * controller, char *IrqName);

int UnregisterIrq(unsigned long irq);

int unregister_ipi(unsigned long irq);

void BuildController(HwInterruptT * Controller);


#endif