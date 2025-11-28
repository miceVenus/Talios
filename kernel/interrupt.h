#ifndef INTERRUPT_H
#define INTERRUPT_H

#define NR_IRQS 24

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

#endif