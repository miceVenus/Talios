#ifndef TRAP_H
#define TRAP_H

#ifndef uint64_t
typedef unsigned long uint64_t;
#endif

extern void divide_error(void);
extern void invalid_TSS(void);
extern void page_fault(void);
extern void general_purpose(void);
extern void double_fault(void);
extern void alignment_check_fault(void);
extern void undefined_opcode_fault(void);

void SysVectorInit();

#endif