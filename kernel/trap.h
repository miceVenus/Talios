#ifndef TRAP_H
#define TRAP_H

#ifndef uint64_t
typedef unsigned long uint64_t;
#endif

extern void divide_error(void);
extern void invalid_TSS(void);
extern void page_fault(void);

void SysVectorInit();

#endif