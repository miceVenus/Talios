#include "task.h"
#include "gate.h"
#include "printk.h"
#include "memory.h"
#include "mmap.h"
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

struct LocalMemManager InitLmm = {0};
/* FXSAVE64 layout: default x87 control word, empty tag word, default MXCSR. */
static const unsigned char initial_fxstate[512] __attribute__((aligned(16))) = {
    [0] = 0x7f, [1] = 0x03, [24] = 0x80, [25] = 0x1f
};
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
            "   sti                 \n\t"
            "   callq   *%rbx       \n\t"
            "   movq    %rax,   %rdi       \n\t"
            "   callq   DoExit      \n\t");

static void exit_files(TaskStruct *task){
    for(int i = 0; i < MAX_HANDLE_PER_TASK; i++){
        if(task->handle_array[i]){
            file_put(task->handle_array[i]);
            task->handle_array[i] = NULL;
        }
    }
}

void __Switch_To(struct TaskStruct *prev, struct TaskStruct *next){
    long cpu_id = smp_cpu_id();

    InitTss[cpu_id].rsp0 = next->thread->rsp0;
    wrmsr(MSR_IA32_SYSENTER_ESP, next->thread->rsp0);

    __asm__ volatile("movq %%fs,    %0" :"=r"(prev->thread->fs));
    __asm__ volatile("movq %%gs,    %0" :"=r"(prev->thread->gs));

    __asm__ volatile("movq %0,      %%fs":: "r"(next->thread->fs));
    __asm__ volatile("movq %0,      %%gs":: "r"(next->thread->gs));

    __asm__ volatile("fxsave64 %0" : "=m"(prev->fxstate));
    __asm__ volatile("fxrstor64 %0" :: "m"(next->fxstate));
    sched_finish_switch(prev);
}

void UserLevelFunc(){

    while(1){

    };
}

void make_keyboard_file(){
    char string[] = "/KEYBOARD.DEV";

    dir_entry * dentry = path_walk(string, 0);

    if(!dentry) return;
    if(dentry->dir_node->attribute & FS_ATTR_DEVICE) return;
    dentry->dir_node->attribute |= FS_ATTR_DEVICE;

    ((FAT32_inode_info *)dentry->dir_node->private_index_info)->first_cluster |= 0xf0000000;
    dentry->dir_node->sb->sb_ops->write_inode(dentry->dir_node);
}

unsigned long init(unsigned long arg){
    ColorPrintfk(BLUE, BLACK, "Init Process Is Runing .args %D\n", arg);

    DISK1_FAT32_FS_INIT();
    make_keyboard_file();

    /* exec may sleep or migrate. The saved scheduler continuation belongs
     * to switch_context; keep the first user register frame on our stack. */
    struct PtRegs regs = {0};
    long rc = do_execve(&regs, "/init.bin");
    if(rc < 0){
        ColorPrintfk(RED, BLACK, "failed to start /init.bin (%D)\n", rc);
        return rc;
    }
    __asm__ volatile("movw %w1, %%fs\n\tmovw %w1, %%gs\n\t"
                     "movq %0, %%rsp\n\tjmp ret_system_call"
                     :: "r"(&regs), "r"((unsigned short)USER_DS) : "memory");
    __builtin_unreachable();
}

static file *open_exec_file(char *path){
    dir_entry * dentry = path_walk(path, 0);

    if(dentry == NULL) {
        ColorPrintfk(BLUE, BLACK, "Can`t Find file %s", path);
        return (file *)(long)-ENOENT;
    }

    if(dentry->dir_node->attribute == FS_ATTR_DIR){
        ColorPrintfk(BLUE, BLACK, "This File Is A Dir%s", path);
        return (file *)(long)-EISDIR;
    }


    ColorPrintfk(BLUE, BLACK, "entry name : %s, entry size : %d\n", dentry->name, dentry->dir_node->file_size);

    if(dentry->dir_node->attribute != FS_ATTR_FILE) return (file *)(long)-EACCES;
    file * filp     = (file *) kmalloc(sizeof(file), 0);
    if(filp == NULL) return (file *)(long)-ENOMEM;
    memset(filp, 0, sizeof(file));
    filp->refs      = 1;
    filp->dentry    = dentry;
    filp->f_ops     = dentry->dir_node->f_ops;
    filp->mode      = 0;
    filp->position  = 0;

    return filp;
}

static void complete_vfork(TaskStruct *task){
    if(task->vfork_parent){
        wakeup_process(task->vfork_parent);
        task->vfork_parent = NULL;
    }
}

unsigned long do_execve(struct PtRegs* regs, char *name){

    const unsigned long base = 0x800000, stack = 0xa00000;
    file *filp = open_exec_file(name);
    if((unsigned long)filp >= (unsigned long)-4095) return (long)filp;
    unsigned long size = filp->dentry->dir_node->file_size;
    if(!size || size > stack - base - PAGE_4K_SIZE){
        file_put(filp);
        return -ENOEXEC;
    }

    struct LocalMemManager *mm = mm_create();
    if(!mm){ file_put(filp); return -ENOMEM; }
    unsigned long end = base + PAGE_4K_ALIGN_UP(size);
    /* The flat image combines instructions and initialized data. The rest
     * of its historical window supplies demand-zero BSS and the user stack. */
    long rc = vm_map(mm, filp, base, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                     MAP_FIXED | MAP_PRIVATE, 0);
    file_put(filp);
    if(rc >= 0)
        rc = vm_map(mm, NULL, end, stack - end, PROT_READ | PROT_WRITE,
                    MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, 0);
    if(rc < 0){ mm_put(mm); return rc; }

    mm->StartCode = base;
    mm->EndCode = base + size;
    mm->StartStack = stack;
    mm->StartBrk = mm->EndBrk = 0xc00000;
    struct LocalMemManager *old = CURRENT->lmm;
    unsigned long irq = irq_save();
    CURRENT->lmm = mm;
    CURRENT->flags &= ~(PF_VFORK | PF_KTHREAD);
    CURRENT->AddrLimit = TASK_SIZE;
    vm_switch_root(CURRENT->cpu_id, (unsigned long)mm->pgd);
    __asm__ volatile("fxrstor64 %0" :: "m"(initial_fxstate));
    complete_vfork(CURRENT);
    irq_restore(irq);
    mm_put(old);

    regs->rdx = regs->r10 = base;
    regs->rcx = regs->r11 = stack;
    regs->rax = 0;
    regs->es = regs->ds = regs->ss = USER_DS;
    regs->cs = USER_CS;
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

    return do_fork(&regs, flag | CLONE_VM, 0, 0);
}

static void copy_thread(TaskStruct *task, unsigned long stack_start, struct PtRegs *regs){
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

}

unsigned long do_fork(struct PtRegs *regs, unsigned long flags, unsigned long stack_start, unsigned long stack_size){
    (void)stack_size;
    TaskStruct *parent = CURRENT;
    TaskStruct *child = kmalloc(STACK_SIZE, 0);
    if(!child) return -ENOMEM;
    memset(child, 0, sizeof(*child));
    spin_lock_init(&child->lock);
    child->flags = parent->flags;
    child->lmm = parent->lmm;
    child->AddrLimit = parent->AddrLimit;
    memcopy(parent->handle_array, child->handle_array, sizeof(child->handle_array));
    ListInit(&child->list);
    if(flags & CLONE_VM){
        if(child->lmm != &InitLmm){
            vm_lock();
            child->lmm->users++;
            vm_unlock();
        }
        child->flags |= PF_VFORK;
    }else{
        child->lmm = mm_fork(parent->lmm);
        if(!child->lmm){ kfree(child); return -ENOMEM; }
        child->flags &= ~PF_VFORK;
    }
    for(int i = 0; i < MAX_HANDLE_PER_TASK; i++)
        if(child->handle_array[i]) file_get(child->handle_array[i]);
    child->parent = parent;
    child->vfork_parent = (flags & CLONE_VFORK) ? parent : NULL;
    child->priority = 2;
    child->state = TASK_UNINTERRUPTABLE;
    child->preempt_count = 0;
    child->flags &= ~NEED_SCHEDULE;
    __asm__ volatile("fxsave64 %0" : "=m"(child->fxstate));
    copy_thread(child, stack_start, regs);
    return sched_publish(child);
}

void DoExit(unsigned long code){
    (void)code;
    TaskStruct *task = CURRENT;
    struct LocalMemManager *old = task->lmm;
    unsigned long irq = irq_save();
    task->lmm = &InitLmm;
    vm_switch_root(CURRENT->cpu_id, (unsigned long)InitLmm.pgd);
    complete_vfork(task);
    irq_restore(irq);
    mm_put(old);
    exit_files(task);

    sched_exit();
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
    if(InitLmm.vma_list.next == NULL)
        lmm_init(&InitLmm);

    InitTss[cpu_id].rsp0 = InitThread.rsp0;

    ListInit(&InitTaskUnions[cpu_id]->task.list);

    CURRENT->state = TASK_RUNNING;
    task_cpu_init();
}

void task_cpu_init(void){
    unsigned long cr0, cr4;
    __asm__ volatile("mov %%cr0,%0" : "=r"(cr0));
    cr0 = (cr0 & ~((1UL << 2) | (1UL << 3))) | (1UL << 1);
    __asm__ volatile("mov %0,%%cr0" :: "r"(cr0) : "memory");
    __asm__ volatile("mov %%cr4,%0" : "=r"(cr4));
    __asm__ volatile("mov %0,%%cr4" :: "r"(cr4 | (3UL << 9)) : "memory");
    __asm__ volatile("fxrstor64 %0" :: "m"(initial_fxstate));

    wrmsr(MSR_IA32_SYSENTER_CS, KERNEL_CS);
    wrmsr(MSR_IA32_SYSENTER_ESP, CURRENT->thread->rsp0);
    wrmsr(MSR_IA32_SYSENTER_EIP, (unsigned long)Syscall);
}
