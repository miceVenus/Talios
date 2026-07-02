#include "task.h"
#include "gate.h"
#include "printk.h"
#include "memory.h"
#include "schedule.h"
#include "smp.h"
#include "fat32.h"
#include "unistd.h"
#include "stdio.h"
#include "errno.h"
#include "sched.h"


#define MSR_IA32_SYSENTER_CS    (0x174)
#define MSR_IA32_SYSENTER_ESP   (0x175)
#define MSR_IA32_SYSENTER_EIP   (0x176)



void    ListInit(struct List *list);
struct  TaskStruct * GetCurrent();
struct  List* ListNext(struct List *list);
void    ListForeAdd(struct List *new, struct List *list);

long global_pid;
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

struct TssStruct InitTss[NR_CPUS] = {[0 ... NR_CPUS - 1] = INIT_TSS};
struct TaskStruct* InitTask[NR_CPUS] = {&InitTaskUnion.task, 0};
struct ThreadStruct* InitThreads[NR_CPUS] = {&InitThread, 0};
union TaskUnion* InitTaskUnions[NR_CPUS] = {&InitTaskUnion, 0};

extern struct GlobalMemManager MMS;


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

long copy_flags(CloneFlag, tsk);
long copy_mm(CloneFlag, tsk);
long copy_files(CloneFlag, tsk);
long copy_thread(CloneFlag, tsk, StackStart, StackSize, regs); 
void wakeup_process(tsk);
void exit_mm(tsk);
void exit_files(tsk);
void exit_thread(tsk);


TaskStruct * get_task(long pid){
    TaskStruct * task_h = &(InitTaskUnions[CURRENT->cpu_id]->task);

    for(TaskStruct * task = task_h; task->next != task_h; task = task->next){
        if(task->pid == pid) return task;
    }
    return NULL;
}

void __Switch_To(struct TaskStruct *prev, struct TaskStruct *next){
    long cpu_id = smp_cpu_id();
    int color   = cpu_id ? WHITE : BLUE;

    InitTss[cpu_id].rsp0 = next -> thread -> rsp0;
    SetTss( (unsigned int*)&InitTss[cpu_id], InitTss[cpu_id].rsp0, InitTss[cpu_id].rsp1, InitTss[cpu_id].rsp2, 
            InitTss[cpu_id].ist1, InitTss[cpu_id].ist2, InitTss[cpu_id].ist3,
            InitTss[cpu_id].ist4, InitTss[cpu_id].ist5, InitTss[cpu_id].ist6,
            InitTss[cpu_id].ist7);
    
    wrmsr(MSR_IA32_SYSENTER_ESP, next->thread->rsp0);

    __asm__ volatile("movq %%fs,    %0" :"=r"(prev->thread->fs));
    __asm__ volatile("movq %%gs,    %0" :"=r"(prev->thread->gs));

    __asm__ volatile("movq %0,      %%fs":: "r"(next->thread->fs));
    __asm__ volatile("movq %0,      %%gs":: "r"(next->thread->gs));

    prev->preempt_count--;
    // ColorPrintfk(color, BLACK, "prev process rsp0 : %p\n", prev->thread->rsp0);
    // ColorPrintfk(color, BLACK, "next process rsp0 : %p\n", next->thread->rsp0);
    // bochs_bp();
}

void UserLevelFunc(){

    while(1){

    };
}

void make_keyboard_file(){
    char string[] = "/KEYBOARD.DEV";

    dir_entry * dentry = path_walk(string, 0);

    if(!dentry){ColorPrintfk(RED, BLACK, "error in create keyboardfile which located in %s\n", string);}

    ((FAT32_inode_info *)dentry->dir_node->private_index_info)->first_cluster |= 0xf0000000;
    dentry->dir_node->sb->sb_ops->write_inode(dentry->dir_node);
}

unsigned long init(unsigned long arg){
    ColorPrintfk(BLUE, BLACK, "Init Process Is Runing .args %D\n", arg);

    struct PtRegs* regs;
    DISK1_FAT32_FS_INIT();
    make_keyboard_file();

    CURRENT->thread->rip = (unsigned long)ret_system_call;
    CURRENT->thread->rsp = (unsigned long)CURRENT + STACK_SIZE - sizeof(struct PtRegs);
    CURRENT->flags = 0;
    CURRENT->thread->gs = USER_DS;
    CURRENT->thread->fs = USER_DS;
    CURRENT->flags      &= ~PF_KTHREAD;

    regs = (struct PtRegs*)CURRENT->thread->rsp;

    __asm__ volatile(   "movq   %1, %%rsp       \n\t"
                        "pushq  %2              \n\t"
                        "jmp    do_execve        \n\t"
                    ::"D"(regs), "r"(CURRENT->thread->rsp), "m"(CURRENT->thread->rip), "S"("/init.bin"));
    return 1;
}

file * open_exec_file(char *path){
    dir_entry * dentry = path_walk(path, 0);

    if(dentry == NULL) {
        ColorPrintfk(BLUE, BLACK, "Can`t Find file %s", path);
        return -ENOENT;
    }

    if(dentry->dir_node->attribute == FS_ATTR_DIR){
        ColorPrintfk(BLUE, BLACK, "This File Is A Dir%s", path);
        return -EISDIR;
    }


    ColorPrintfk(BLUE, BLACK, "entry name : %s, entry size : %d\n", dentry->name, dentry->dir_node->file_size);

    file * filp     = (file *) kmalloc(sizeof(file), 0);
    memset(filp, 0, sizeof(file));
    filp->dentry    = dentry;
    filp->f_ops     = dentry->dir_node->f_ops;
    filp->mode      = 0;
    filp->position  = 0;

    return filp;
}

unsigned long do_execve(struct PtRegs* regs, char *name){

    unsigned long code_start_addr   = 0x800000;
    unsigned long stack_start_addr  = 0xa00000;
    unsigned long brk_start_addr    = 0xc00000;
    unsigned long *tmp;
    unsigned long *virtual  = NULL;
    file *filp              = NULL;
    long retval;

    long pos = 0;

    regs->rdx   = code_start_addr;          // sysexit RIP
    regs->rcx   = stack_start_addr;         // sysexit RSP
    regs->rax   = 1;
    regs->r10   = code_start_addr;          // sysexit RIP
    regs->r11   = stack_start_addr;         // sysexit RSP
    regs->es    = USER_DS;
    regs->ds    = USER_DS;
    regs->cs    = USER_CS;
    regs->ss    = USER_DS;

    ColorPrintfk(BLUE, BLACK, "execve is running\n");

    // sub process should have an independent vaddr to run
    // but process gen by vfork would not have this feature

    if(CURRENT->flags & PF_VFORK){

        CURRENT->lmm = (struct LocalMemManager * )kmalloc(sizeof(struct LocalMemManager), 0);
        memset(CURRENT->lmm, 0 ,sizeof(struct LocalMemManager));

        CURRENT->lmm->pgd = (pml4t_t *)VIRT_TO_PHY(kmalloc(PAGE_4K_SIZE, 0));

        // copy entries which map the addr above 0xffff800000000000(kernel space)
        memcopy(PHY_TO_VIRT(InitTaskUnions[CURRENT->cpu_id]->task.lmm->pgd) + 256, PHY_TO_VIRT(CURRENT->lmm->pgd) + 256, PAGE_4K_SIZE >> 1);

        memset(PHY_TO_VIRT(CURRENT->lmm->pgd), 0, PAGE_4K_SIZE / 2);
    }

    tmp = (unsigned long *)PHY_TO_VIRT((unsigned long)CURRENT->lmm->pgd & (~0xfffUL)) + GetBits(code_start_addr, PAGE_GDT_SHIFT, 9);

    if(*tmp == NULL){
        unsigned long * virtual = (unsigned long *)kmalloc(PAGE_4K_SIZE, 0);
        memset(virtual, 0, PAGE_4K_SIZE);
        SetPML4E(tmp, VIRT_TO_PHY(virtual), PEA_USER_TABLE);
    }

    tmp = (unsigned long *)PHY_TO_VIRT((unsigned long)(*tmp) & (~0xfffUL)) + GetBits(code_start_addr, PAGE_1G_SHIFT, 9);

    if(*tmp == NULL){
        virtual = (unsigned long *)kmalloc(PAGE_4K_SIZE, 0);
        memset(virtual, 0, PAGE_4K_SIZE);
        SetPDPTE(tmp, VIRT_TO_PHY(virtual), PEA_USER_TABLE);
    }

    tmp = (unsigned long *)PHY_TO_VIRT((unsigned long)(*tmp) & (~0xfffUL)) + GetBits(code_start_addr, PAGE_2M_SHIFT, 9);
    
    if(*tmp == NULL){
        struct Page * p = AllocPage(ZONE_NORMAL_INDEX, 1, PG_PTABLE_MAPPED);
        SetPDE(tmp, p->PhyAddr, PEA_USER_ENTRY);
    }

    SetCr3(CURRENT->lmm->pgd);

    if(!(CURRENT->flags & PF_KTHREAD))
    CURRENT->AddrLimit = TASK_SIZE;

    CURRENT->lmm->StartCode     = code_start_addr;
    CURRENT->lmm->StartStack    = stack_start_addr;
    CURRENT->lmm->StartBrk      = brk_start_addr;
    CURRENT->lmm->EndBrk        = brk_start_addr;

    exit_files(CURRENT);

    CURRENT->flags &= ~PF_VFORK;

    filp = open_exec_file(name);

    if((unsigned long)filp > -0x1000UL)
        return (unsigned long)filp;

    memset((void *)code_start_addr, 0, PAGE_2M_SIZE);
    retval = filp->f_ops->read(filp, (void *)code_start_addr, filp->dentry->dir_node->file_size, &pos);

    return retval;
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

    return do_fork(&regs, flag | CLONE_VM, 0, 0);
}

void wakeup_process(TaskStruct * task){
    task->state = TASK_RUNING;
    insert_task_queue(task);
    CURRENT->flags |= NEED_SCHEDULE;
}

inline long copy_flags(unsigned long flags, TaskStruct * task){
    if(flags & CLONE_VM) task->flags |= PF_VFORK;
    return 0;
}

inline long copy_files(unsigned long flags, TaskStruct * task){
    int error   = 0;

    if(flags & CLONE_FS)
        goto out;

    for(int i = 0; i < MAX_HANDLE_PER_TASK; i++){
        if(CURRENT->handle_array[i] != NULL){
            task->handle_array[i] = (file *)kmalloc(sizeof(file), 0);
            if(task->handle_array[i] == NULL){
                error = 1;
                break;
            }
            memcopy(CURRENT->handle_array[i], task->handle_array[i] , sizeof(file));
        }
    }

    out:
        return error;
}

inline void exit_files(TaskStruct * task){
    if(task->flags & PF_VFORK); // do nothing
    else
        for(int i = 0; i < MAX_HANDLE_PER_TASK; i++){
            if(task->handle_array[i] != NULL) kfree(task->handle_array[i]);
        }
    
    memset(task->handle_array, 0 , sizeof(file *) * MAX_HANDLE_PER_TASK);
}


inline long copy_mm(unsigned long flags, TaskStruct * task){
    int error = 0;

    unsigned long code_start_addr   = 0x800000;
    unsigned long stack_start_addr  = 0xa00000;
    unsigned long brk_start_addr    = 0xc00000;
    struct LocalMemManager * t_mm;


    if(flags & CLONE_VM){
        t_mm = CURRENT->lmm;
        goto out;
    }

    t_mm = (struct LocalMemManager * )kmalloc(sizeof(struct LocalMemManager), 0);
    memcopy(CURRENT->lmm, t_mm, sizeof(struct LocalMemManager));

    t_mm->pgd = (pml4t_t *)VIRT_TO_PHY(kmalloc(PAGE_4K_SIZE, 0));

    // copy entries which map the addr above 0xffff800000000000(kernel space)
    memcopy(PHY_TO_VIRT(InitTaskUnions[CURRENT->cpu_id]->task.lmm->pgd) + 256, PHY_TO_VIRT(t_mm->pgd) + 256, PAGE_4K_SIZE >> 1);

    memset(PHY_TO_VIRT(t_mm->pgd), 0, PAGE_4K_SIZE / 2);

    unsigned long * tmp = (unsigned long *)PHY_TO_VIRT((unsigned long)t_mm->pgd & (~0xfffUL)) + GetBits(code_start_addr, PAGE_GDT_SHIFT, 9);

    unsigned long * virtual = (unsigned long *)kmalloc(PAGE_4K_SIZE, 0);
    memset(virtual, 0, PAGE_4K_SIZE);
    SetPML4E(tmp, VIRT_TO_PHY(virtual), PEA_USER_TABLE);

    tmp = (unsigned long *)PHY_TO_VIRT((unsigned long)(*tmp) & (~0xfffUL)) + GetBits(code_start_addr, PAGE_1G_SHIFT, 9);
    virtual = (unsigned long *)kmalloc(PAGE_4K_SIZE, 0);
    memset(virtual, 0, PAGE_4K_SIZE);
    SetPDPTE(tmp, VIRT_TO_PHY(virtual), PEA_USER_TABLE);

    tmp = (unsigned long *)PHY_TO_VIRT((unsigned long)(*tmp) & (~0xfffUL)) + GetBits(code_start_addr, PAGE_2M_SHIFT, 9);
    struct Page * p = AllocPage(ZONE_NORMAL_INDEX, 1, PG_PTABLE_MAPPED);
    SetPDE(tmp, p->PhyAddr, PEA_USER_ENTRY);

    memcopy((void*)code_start_addr, PHY_TO_VIRT(p->PhyAddr), stack_start_addr - code_start_addr);

    if(CURRENT->lmm->StartBrk - CURRENT->lmm->EndBrk != 0){

        tmp = (unsigned long *)PHY_TO_VIRT((unsigned long)t_mm->pgd & (~0xfffUL)) + GetBits(brk_start_addr, PAGE_GDT_SHIFT, 9);
        tmp = (unsigned long *)PHY_TO_VIRT((unsigned long)(*tmp) & (~0xfffUL)) + GetBits(brk_start_addr, PAGE_1G_SHIFT, 9);
        tmp = (unsigned long *)PHY_TO_VIRT((unsigned long)(*tmp) & (~0xfffUL)) + GetBits(brk_start_addr, PAGE_2M_SHIFT, 9);
        struct Page * p = AllocPage(ZONE_NORMAL_INDEX, 1, PG_PTABLE_MAPPED);
        SetPDE(tmp, p->PhyAddr, PEA_USER_ENTRY);

        memcopy((void *)brk_start_addr, PHY_TO_VIRT(p->PhyAddr), PAGE_2M_SIZE);
    }

    out:
        task->lmm = t_mm;
        return error;

}

inline void exit_mm(TaskStruct * task){
    unsigned long code_start_addr = 0x800000;
    unsigned long *tmp1;
    unsigned long *tmp2;
    unsigned long *tmp3;
    if(task->flags & PF_VFORK)
        return;

    if(task->lmm->pgd != NULL){
        tmp1 = PHY_TO_VIRT((unsigned long *)((unsigned long)task->lmm->pgd & (~0xfffUL) + GetBits(code_start_addr, PAGE_GDT_SHIFT, 9)));
        tmp2 = PHY_TO_VIRT((unsigned long *)((unsigned long)*tmp1 & (~0xfffUL) + GetBits(code_start_addr, PAGE_1G_SHIFT, 9)));
        tmp3 = PHY_TO_VIRT((unsigned long *)((unsigned long)*tmp2 & (~0xfffUL) + GetBits(code_start_addr, PAGE_2M_SHIFT, 9)));

        FreePage((struct Page *)(MMS.PagesGroup + PAGE_2M_INDEX(*tmp3)), 1);
        kfree(tmp2);
        kfree(tmp1);
        kfree(PHY_TO_VIRT((unsigned long*)task->lmm->pgd));
    }

    if(task->lmm != NULL)
        kfree(task->lmm);
}


inline long copy_thread(unsigned long flags, TaskStruct * task, unsigned long stack_start, unsigned long stack_size, struct PtRegs* regs){
    struct ThreadStruct *thd = NULL;
    struct PtRegs *childregs = NULL;

    childregs = (struct PtRegs *)((unsigned long)task + STACK_SIZE) - 1;
    memcopy(regs, childregs, sizeof(struct PtRegs));

    thd = (struct ThreadStruct *)(task + 1);
    memset(thd, 0, sizeof(struct ThreadStruct));
    task->thread = thd;

    // here is magic happens in sub process it would call ret_system_call and return 0
    childregs->rax = 0;
    childregs->rsp = stack_start;

    thd->rsp0   = (unsigned long)task + STACK_SIZE;
    thd->rsp    = (unsigned long)childregs;
    thd->fs     = (unsigned long)CURRENT->thread->fs;
    thd->gs     = (unsigned long)CURRENT->thread->gs;

    if(task->flags & PF_KTHREAD)
        thd->rip = (unsigned long)KernelThreadFunc;
    else{
        thd->rip = (unsigned long)ret_system_call;
    }

    return 0;
}

inline void exit_thread(TaskStruct * task){

}

// Uncomplete Function
unsigned long do_fork(struct PtRegs * regs, unsigned long CloneFlag, unsigned long StackStart, unsigned long StackSize){

    int retval = 0;
    TaskStruct * current = CURRENT;

    TaskStruct * tsk = (struct TaskStruct *)kmalloc(sizeof(TaskStruct), 0);
    if(tsk == 0){
        retval = -EAGAIN;
        goto alloc_copy_fail;
    }
    memset(tsk, 0, sizeof(struct TaskStruct));
    memcopy(current, tsk, sizeof(TaskStruct));
    ListInit(&tsk->list);
    // ListForeAdd(&(current->list), &tsk->list);
    tsk->next = current->next;
    current->next = tsk;
    tsk->parent = current;
    tsk->pid = global_pid++;
    tsk->priority = 2;
    tsk->cpu_id = smp_cpu_id();
    tsk->state = TASK_UNINTERRUPTABLE;

    retval = -ENOMEM;

    if(copy_flags(CloneFlag, tsk))  goto copy_flags_fail;
    if(copy_mm(CloneFlag, tsk))     goto copy_mm_fail;
    if(copy_files(CloneFlag, tsk))  goto copy_files_fail;
    if(copy_thread(CloneFlag, tsk, StackStart, StackSize, regs)) 
    goto copy_thread_fail;

    retval = tsk->pid;
    wakeup_process(tsk);

    fork_ok:
        return retval;
    copy_mm_fail:
        exit_mm(tsk);
    copy_files_fail:
        exit_files(tsk);
    copy_thread_fail:
        exit_thread(tsk);
    copy_flags_fail:
    alloc_copy_fail:
        kfree(tsk);
        return retval;



    tsk->thread = (struct ThreadStruct*)(tsk + 1);
    memcopy(regs, (void *)((unsigned long)tsk + STACK_SIZE - sizeof(struct PtRegs)), sizeof(struct PtRegs));

    tsk->thread->rsp    = (unsigned long)tsk + STACK_SIZE - sizeof(struct PtRegs);
    tsk->thread->rip    = regs->rip;
    tsk->thread->rsp0   = (unsigned long)tsk + STACK_SIZE;

    if(!(tsk->flags & PF_KTHREAD))
        tsk->thread->rip = regs->rip = (unsigned long)ret_system_call;

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
    extern char _bss;
    extern char _ebss;
    extern unsigned long _stack_start;

    struct TaskStruct *p = NULL;
    long cpu_id         = smp_cpu_id();

    unsigned long * vaddr   = NULL;
    unsigned long * tmp     = NULL;

    vaddr = (unsigned long *)PHY_TO_VIRT(((unsigned long)GetCr3()) & (~0xfffUL));
    *vaddr = 0UL;

    for(int i = 256; i < 512; i++){
        tmp = vaddr + i;
        if(*tmp == 0){
            unsigned long *virtual = kmalloc(PAGE_4K_SIZE, 0);
            memset(virtual, 0, PAGE_4K_SIZE);
            SetPML4E(tmp, VIRT_TO_PHY(virtual), PEA_SUPERVISOR_TABLE);
        }
    }


    InitLmm.pgd         =   (pml4t_t *)GetCr3();
    InitLmm.StartCode   =   MMS.StartCode;
    InitLmm.EndCode     =   MMS.EndCode;
    InitLmm.StartData   =   (unsigned long)(&_data);
    InitLmm.EndData     =   MMS.EndData;
    InitLmm.StartRoData =   (unsigned long)(&_rodata);
    InitLmm.EndRoData   =   MMS.EndRoData;
    InitLmm.StartBrk    =   MMS.StartBrk;
    InitLmm.EndBrk      =   CURRENT->AddrLimit;
    InitLmm.start_bss   =   (unsigned long)(&_bss);
    InitLmm.end_bss     =   (unsigned long)(&_ebss);
    InitLmm.StartStack  =   _stack_start;

    SetTss( (unsigned int*)&InitTss[cpu_id], InitThreads[cpu_id]->rsp0, InitTss[cpu_id].rsp1, InitTss[cpu_id].rsp2, 
            InitTss[cpu_id].ist1, InitTss[cpu_id].ist2, InitTss[cpu_id].ist3,
            InitTss[cpu_id].ist4, InitTss[cpu_id].ist5, InitTss[cpu_id].ist6,
            InitTss[cpu_id].ist7);
    
    InitTss[cpu_id].rsp0 = InitThreads[cpu_id]->rsp0;

    ListInit(&InitTaskUnions[cpu_id]->task.list);

    wrmsr(MSR_IA32_SYSENTER_CS, KERNEL_CS);
    wrmsr(MSR_IA32_SYSENTER_ESP, CURRENT->thread->rsp0);
    wrmsr(MSR_IA32_SYSENTER_EIP, (unsigned long)Syscall);

    // Second Process Should Be User State
    KernelThread(init, 10, TATTR(CLONG_FS) | TATTR(CLONG_FS) | TATTR(CLONG_SIGNAL));

    InitTaskUnions[cpu_id]->task.state = TASK_RUNING;

    // p = ContainerOf(ListNext(&task_scheduler.task_queue.list), struct TaskStruct, list);

    // SWITCH_TO(CURRENT, p);
}