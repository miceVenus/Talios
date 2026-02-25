#ifndef TASK_H
#define TASK_H

#include "lib.h"

#define STACK_SIZE 32768
#define MAX_SYS_CALL 128

#define PF_KTHREAD 1
#define NR_CPUS 16
#define KERNEL_DS 0x10
#define KERNEL_CS 0x08

#ifndef NULL
#define NULL 0UL
#endif

#define TATTR(flag) (1UL << flag)

#define INIT_TASK(tsk) {    \
    .state  =   TASK_UNINTERRPTABLE,    \
    .flags  =   PF_KTHREAD,             \
    .lmm    =   &InitLmm,               \
    .thread =   &InitThread,            \
    .AddrLimit  = 0xffff800000000000,   \
    .pid        = 0,                    \
    .counter    = 1,                    \
    .signal     = 0,                    \
    .priority   = 0,                    \
}

#define STOP   \
    while(1){}


#define INIT_TSS {       \
    .reserved0  = 0,    \
    .rsp0       = (unsigned long)(InitTaskUnion.stack + STACK_SIZE / sizeof(unsigned long)),    \
    .rsp1       = (unsigned long)(InitTaskUnion.stack + STACK_SIZE / sizeof(unsigned long)),    \
    .rsp2       = (unsigned long)(InitTaskUnion.stack + STACK_SIZE / sizeof(unsigned long)),    \
    .reserved1  = 0,    \
    .ist1       = 0xffff800000007c00,   \
    .ist2       = 0xffff800000007c00,   \
    .ist3       = 0xffff800000007c00,   \
    .ist4       = 0xffff800000007c00,   \
    .ist5       = 0xffff800000007c00,   \
    .ist6       = 0xffff800000007c00,   \
    .ist7       = 0xffff800000007c00,   \
    .reserved2  = 0,    \
    .reserved3  = 0,    \
    .IoMapBaseAddr  = 0 \
}

#define CURRENT (GetCurrent())

#define GET_CURRENT     \
        "movq   %rsp,       %rbx    \n\t"\
        "andq   $-32768,    %rbx    \n\t"

#define SWITCH_TO(prev, next)   \
    do{                         \
        __asm__ volatile(       \
            "pushq  %%rax                       \n\t"           \
            "movq   %%rsp,          %0          \n\t"           \
            "movq   %2,             %%rsp       \n\t"           \
            "leaq   1f(%%rip),      %%rax       \n\t"           \
            "movq   %%rax,          %1          \n\t"           \
            "pushq  %3                          \n\t"           \
            "jmp    __Switch_To                 \n\t"           \
            "1:                                 \n\t"           \
            "popq   %%rax                       \n\t"           \
            :"=r"(prev->thread->rsp), "=r"(prev->thread->rip)                       \
            :"r"(next->thread->rsp), "r"(next->thread->rip), "D"(prev), "S"(next)   \
            :"memory", "rax");   \
    }while (0);


enum TASK_STATE{
    TASK_RUNING = 0,
    TASK_UNINTERRPTABLE
};

enum TASK_FLAG{
    CLONG_FS = 0,
    CLONG_FILES,
    CLONG_SIGNAL,
};

typedef unsigned long pml4t_t ;

struct LocalMemManager{
    pml4t_t *pgd;
    unsigned long StartCode,    EndCode;
    unsigned long StartData,    EndData;

    unsigned long StartROData,  EndROCode;
    unsigned long StartBrk,     EndBrk;

    unsigned long StartStack;
};

struct ThreadStruct{
    unsigned long rsp0;     // in Tss

    unsigned long rip;
    unsigned long rsp;

    unsigned long fs;
    unsigned long gs;

    unsigned long cr2;
    unsigned long TrapNum;
    unsigned long ErrorCode;

};

struct TaskStruct{
    volatile long state;
    unsigned long flags;

    struct List             list;
    struct LocalMemManager* lmm;
    struct ThreadStruct*    thread;

    unsigned long AddrLimit;

    long pid;
    long counter;
    long signal;
    long priority;
};

union TaskUnion{

    struct TaskStruct task;
    unsigned long stack[STACK_SIZE / sizeof(long)];

}__attribute__((aligned(8)));

struct TssStruct{
    unsigned int    reserved0;

    unsigned long   rsp0;
    unsigned long   rsp1;
    unsigned long   rsp2;

    unsigned long   reserved1;

    unsigned long   ist1; 
    unsigned long   ist2;
    unsigned long   ist3;
    unsigned long   ist4;
    unsigned long   ist5;
    unsigned long   ist6;
    unsigned long   ist7;

    unsigned long   reserved2;
    unsigned short  reserved3;
    unsigned short  IoMapBaseAddr;
}__attribute__((packed));

struct PtRegs{
    unsigned long r15;
    unsigned long r14;
    unsigned long r13;
    unsigned long r12;
    unsigned long r11;
    unsigned long r10;
    unsigned long r9;
    unsigned long r8;
    unsigned long rbx;
    unsigned long rcx;
    unsigned long rdx;
    unsigned long rsi;

    unsigned long rdi;
    unsigned long rbp;    
    unsigned long ds;
    unsigned long es;

    unsigned long rax;
    unsigned long func;
    unsigned long ErroCode;
    unsigned long rip;    
    unsigned long cs;
    unsigned long rflag;    
    unsigned long rsp;
    unsigned long ss;    
};

inline struct TaskStruct * GetCurrent(){
    struct TaskStruct* current = NULL;
    __asm__ volatile("andq %%rsp, %0": "=r"(current): "0"(~32767UL));
    return current;
}

/*          TASK         */
unsigned long DoFork(struct PtRegs * regs, unsigned long CloneFlag, unsigned long StackStart, unsigned long StackSize);
void TaskInit();

void    ret_from_intr(void);
void    ret_system_call(void);
void    KernelThreadFunc(void);
void    __Switch_To(struct TaskStruct *prev, struct TaskStruct *next);
void    KernelThreadFunc(void);


/*          SYSCALL         */
typedef unsigned long (*system_call_t)(struct PtRegs *regs);

void            Syscall(void);
unsigned long   SystemCallFunc(struct PtRegs* regs);
unsigned long   NoSystemCall(struct PtRegs* regs);

#endif