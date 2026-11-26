#ifndef TASK_H
#define TASK_H

#include "lib.h"
#include "memory.h"

#include "vfs.h"
#include "spin_lock.h"

#define STACK_SIZE 32768
#define MAX_SYS_CALL 128
#define MAX_HANDLE_PER_TASK 20
#define TASK_SIZE 0x00007fffffffffff


// struct task_struct->flags
#define PF_KTHREAD      (1UL << 0)
#define NEED_SCHEDULE   (1UL << 1)
#define PF_VFORK        (1UL << 2)

#define NR_CPUS 16
#define KERNEL_DS 0x10
#define KERNEL_CS 0x08
#define USER_DS     0x30
#define USER_CS     0x28

#define TATTR(flag) (1UL << flag)

#define INIT_TASK(tsk) {    \
    .state  =   TASK_UNINTERRUPTABLE,    \
    .flags  =   PF_KTHREAD,             \
    .lmm    =   &InitLmm,               \
    .thread =   &InitThread,            \
    .AddrLimit  = TASK_SIZE,            \
    .pid        = 0,                    \
    .signal     = 0,                    \
    .priority   = 0,                    \
    .vrun_time  = 0,                    \
    .preempt_count = 0,                 \
    .cpu_id     = 0,                    \
    .handle_array = {0},                \
    .parent     = &tsk,                 \
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

enum TASK_STATE{
    TASK_RUNNING = 0,
    TASK_UNINTERRUPTABLE,
    TASK_DEAD,
    TASK_RUNNABLE
};

enum TASK_FLAG{
    CLONG_FS = 0,
    CLONG_FILES,
    CLONG_SIGNAL,
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


// linkage.h have some definition of offset which is one-to-one correspondent with this struct
typedef struct TaskStruct{
    volatile long state;
    unsigned long flags;
    long signal;
    long preempt_count;

    SpinLock_T lock;
    struct List             list;
    struct LocalMemManager* lmm;
    struct ThreadStruct*    thread;

    unsigned long AddrLimit;

    long pid;
    long priority;
    long vrun_time;
    long cpu_id;    /* Owner until the CPU has left this task's stack. */

    file * handle_array[MAX_HANDLE_PER_TASK];
    struct TaskStruct * parent;
    struct TaskStruct *vfork_parent;
    unsigned char fxstate[512] __attribute__((aligned(16)));

}TaskStruct;

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
unsigned long do_fork(struct PtRegs * regs, unsigned long CloneFlag, unsigned long StackStart, unsigned long StackSize);
unsigned long do_execve(struct PtRegs * regs, char *name);
void TaskInit(void);
void task_cpu_init(void);
unsigned long init(unsigned long arg);
unsigned long KernelThread(unsigned long (*func)(unsigned long), unsigned long arg, unsigned long flags);

void switch_context(struct TaskStruct *prev, struct TaskStruct *next, unsigned long *rsp,
                    unsigned long *rip, unsigned long next_rsp, unsigned long next_rip);
void    ret_from_intr(void);
void    ret_system_call(void);
void    KernelThreadFunc(void);
void    __Switch_To(struct TaskStruct *prev, struct TaskStruct *next);
void    wakeup_process(TaskStruct * task);
void DoExit(unsigned long code) __attribute__((noreturn));


/*          SYSCALL         */
typedef unsigned long (*system_call_t)(struct PtRegs *regs);

void            Syscall(void);
unsigned long   SystemCallFunc(struct PtRegs* regs);
unsigned long   NoSystemCall(struct PtRegs* regs);

#endif
