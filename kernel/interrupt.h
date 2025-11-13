#ifndef INTERRUPT_H
#define INTERRUPT_H

typedef void (*interrupt_t)(void);

void InterruptInit();

#endif