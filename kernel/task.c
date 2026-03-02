#include "task.h"
#include "gate.h"
#include "printk.h"
#include "lib.h"
#include "memory.h"

void    ListInit(struct List *list);
struct  TaskStruct * GetCurrent();
struct  List* ListNext(struct List *list);
void    ListForeAdd(struct List *new, struct List *list);


struct LocalMemManager InitLmm = {0};
struct ThreadStruct InitThread;

union TaskUnion InitTaskUnion __attribute__((__section__ (".data.init_task"))) = {INIT_TASK(InitTaskUnion.task)};

struct ThreadStruct InitThread = {
    .rsp0   =   (unsigned long)(InitTaskUnion.stack + STACK_SIZE / sizeof(unsigned long)),
    .rsp    =   (unsigned long)(InitTaskUnion.stack + STACK_SIZE / sizeof(unsigned long)),
    .fs     =   KERNEL_DS,
    .gs     =   KERNEL_DS,
    .cr2    =   0,
    .TrapNum    =   0,
    .ErrorCode  =   0
};

struct TaskStruct* InitTask[NR_CPUS] = {&InitTaskUnion.task, 0};

struct TssStruct InitTss[NR_CPUS] = {[0 ... NR_CPUS - 1] = INIT_TSS};

__asm__ (   
            ".global KernelThreadFunc      \n\t"
            "KernelThreadFunc:     \n\t"
            "   popq    %r15       \n\t"
            "   popq    %r14       \n\t"
            "   popq    %r13       \n\t"
            "   popq    %r12       \n\t"
            "   popq    %r11       \n\t"
            "   popq    %r10       \n\t"
            "   popq    %r9        \n\t"
            "   popq    %r8        \n\t"
            "   popq    %rbx       \n\t"
            "   popq    %rcx       \n\t"
            "   popq    %rdx       \n\t"
            "   popq    %rsi       \n\t"
            "   popq    %rdi       \n\t"
            "   popq    %rbp       \n\t"
            "   popq    %rax       \n\t"
            "   movq    %rax,   %ds       \n\t"
            "   popq    %rax       \n\t"
            "   movq    %rax,   %es       \n\t"
            "   popq    %rax       \n\t"
            "   addq    $0x38,  %rsp       \n\t"
            "   movq    %rdx,   %rdi       \n\t"
            "   callq   *%rbx       \n\t"
            "   movq    %rax,   %rdi       \n\t"
            "   callq   DoExit      \n\t");

void __Switch_To(struct TaskStruct *prev, struct TaskStruct *next){
    InitTss[0].rsp0 = next -> thread -> rsp0;
    SetTss( (unsigned int *)&InitTss[0], InitTss[0].rsp0, InitTss[0].rsp1, InitTss[0].rsp2, 
            InitTss[0].ist1, InitTss[0].ist2, InitTss[0].ist3,
            InitTss[0].ist4, InitTss[0].ist5, InitTss[0].ist6,
            InitTss[0].ist7);
    __asm__ volatile("movq %%fs,    %0" :"=r"(prev->thread->fs));
    __asm__ volatile("movq %%gs,    %0" :"=r"(prev->thread->gs));

    __asm__ volatile("movq %0,      %%fs":: "r"(next->thread->fs));
    __asm__ volatile("movq %0,      %%gs":: "r"(next->thread->gs));

    ColorPrintfk(BLUE, BLACK, "prev process rsp0 : %p\n", prev->thread->rsp0);
    ColorPrintfk(BLUE, BLACK, "next process rsp0 : %p\n", next->thread->rsp0);
}


/*          SYSCALL         */
unsigned long NoSystemCall(struct PtRegs* regs){
    ColorPrintfk(RED, BLACK, "There Is No System Call %D \n", regs->rax);
    return -1;
}

unsigned long SysPrint(struct PtRegs* regs){
    ColorPrintfk(WHITE, BLACK, (char *)regs->rdi);
    return 1;
}

system_call_t SystemCallTable[MAX_SYS_CALL] = {
    [0] = SysPrint,
    [1 ... MAX_SYS_CALL - 1] = NoSystemCall,
};

unsigned long SystemCallFunc(struct PtRegs* regs){
    return SystemCallTable[regs->rax](regs);
}


void UserLevelFunc(){
    // Can`t Be Called
    // ColorPrintfk(BLUE, BLACK, "In User Level\n");
    long ret = 0;
    __asm__ volatile(   "leaq sysexit_return_address(%%rip),   %%rdx    \n\t"
                        "movq   %%rsp,  %%rcx                           \n\t"
                        "sysenter                                       \n\t"
                        "sysexit_return_address:                        \n\t"
                        :"=a"(ret):"0"(0):"memory");
    while(1){

    };
}


unsigned long init(unsigned long arg){
    ColorPrintfk(BLUE, BLACK, "Init Process Is Runing .args %D\n", arg);

    struct PtRegs* regs;

    CURRENT->thread->rip = (unsigned long)ret_system_call;
    CURRENT->thread->rsp = (unsigned long)CURRENT + STACK_SIZE - sizeof(struct PtRegs);

    regs = (struct PtRegs*)CURRENT->thread->rsp;

    __asm__ volatile(   "movq   %1, %%rsp       \n\t"
                        "pushq  %2              \n\t"
                        "jmp    DoExecve        \n\t"
                    ::"D"(regs), "r"(CURRENT->thread->rsp), "r"(CURRENT->thread->rip));
    return 1;
}

unsigned long DoExecve(struct PtRegs* regs){
    regs->rdx   = 0x800000;   // RIP
    regs->rcx   = 0xa00000;   // RSP
    regs->rax   = 1;
    regs->es    = 0;
    regs->ds    = 0;
    ColorPrintfk(BLUE, BLACK, "execve is running\n");
    memcopy(UserLevelFunc, (void *)0x800000, 1024);
    return 0;
}

unsigned long KernelThread(unsigned long (*Func)(unsigned long), unsigned long args, unsigned long flag){
    struct PtRegs regs;
    memset(&regs, 0, sizeof(regs));

    regs.rbx = (unsigned long)Func;
    regs.rdx = (unsigned long)args;

    regs.ds  = KERNEL_DS;
    regs.es  = KERNEL_DS;
    regs.ss  = KERNEL_DS;

    regs.cs     = KERNEL_CS;
    regs.rflag  = (1 << 9);
    regs.rip    = (unsigned long)KernelThreadFunc;
    // ColorPrintfk(BLUE, BLACK, "rip: %p\n", KernelThreadFunc);

    return DoFork(&regs, flag, 0, 0);
}

// Uncomplete Function
unsigned long DoFork(struct PtRegs * regs, unsigned long CloneFlag, unsigned long StackStart, unsigned long StackSize){
    struct Page *p = AllocPage(ZONE_NORMAL_INDEX, 1, PATTR(PG_ACTIVE) | PATTR(PG_KERNEL) | PATTR(PG_PTABLE_MAPPED));

    if(p == NULL) return 0;

    struct TaskStruct *tsk = (struct TaskStruct *)PHY_TO_VIRT(p->PhyAddr);

    memset(tsk, 0, sizeof(struct TaskStruct));

    *tsk = *CURRENT;

    ListInit(&tsk->list);
    ListForeAdd(&(CURRENT->list), &tsk->list);
    tsk->pid++;
    tsk->state = TASK_UNINTERRUPTABLE;

    tsk->thread = (struct ThreadStruct*)(tsk + 1);
    memcopy(regs, (void *)((unsigned long)tsk + STACK_SIZE - sizeof(struct PtRegs)), sizeof(struct PtRegs));

    tsk->thread->rsp    = (unsigned long)tsk + STACK_SIZE - sizeof(struct PtRegs);
    tsk->thread->rip    = regs->rip;
    tsk->thread->rsp0   = (unsigned long)tsk + STACK_SIZE;

    if(!(tsk->flags & PF_KTHREAD))
        tsk->thread->rip = regs->rip = (unsigned long)ret_system_call;
    
    tsk->state = TASK_RUNING;

    return 1;
}

unsigned long DoExit(unsigned long code){
    ColorPrintfk(BLUE, BLACK, "init return as %D\n", code);
    while (1){

    }
}

/*
Init After Memory
*/
void TaskInit(){

    extern char _data;
    extern char _rodata;
    extern char _erodata;
    extern struct GlobalMemManager MMS;
    extern unsigned long _stack_start;

    struct TaskStruct *p = NULL;

    InitLmm.pgd         =   (pml4t_t *)GetCr3();
    InitLmm.StartCode   =   MMS.StartCode;
    InitLmm.EndCode     =   MMS.EndCode;
    InitLmm.StartData   =   (unsigned long)(&_data);
    InitLmm.EndData     =   MMS.EndData;
    InitLmm.StartROData =   (unsigned long)(&_rodata);
    InitLmm.EndROCode   =   (unsigned long)(&_erodata);
    InitLmm.StartBrk    =   0;
    InitLmm.EndBrk      =   MMS.EndBrk;
    InitLmm.StartStack  =   _stack_start;

    SetTss( (unsigned int *)&InitTss[0], InitThread.rsp0, InitTss[0].rsp1, InitTss[0].rsp2, 
            InitTss[0].ist1, InitTss[0].ist2, InitTss[0].ist3,
            InitTss[0].ist4, InitTss[0].ist5, InitTss[0].ist6,
            InitTss[0].ist7);
    
    InitTss[0].rsp0 = InitThread.rsp0;

    ListInit(&InitTaskUnion.task.list);

    wrmsr(0x174, KERNEL_CS);
    wrmsr(0x175, CURRENT->thread->rsp0);
    wrmsr(0x176, (unsigned long)Syscall);

    // Second Process Should Be User State
    KernelThread(init, 10, TATTR(CLONG_FS) | TATTR(CLONG_FS) | TATTR(CLONG_SIGNAL));

    InitTaskUnion.task.state = TASK_RUNING;

    p = ContainerOf(ListNext(&CURRENT->list), struct TaskStruct, list);

    SWITCH_TO(CURRENT, p);
}