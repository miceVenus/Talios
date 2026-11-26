#include "syscall.h"


#define __SYSFUNC_DEF__(name, nr)   \
            __asm__ (                                                       \
                    ".global    "#name"                             \n\t"       \
                    ".type      "#name",    @function               \n\t"   \
                    #name":                                         \n\t"   \
                    "movq   $"#nr",  %rax                           \n\t"   \
                    "jmp    LABEL_SYSCALL                           \n\t"   );

#define __SYSFUNC_DEF_(name, nr) __SYSFUNC_DEF__(name, nr)
#define STR_NR_(n) #n
#define STR_NR(n) STR_NR_(n)
#define SYSFUNC_DEF(name)   __SYSFUNC_DEF_(name, __NR_##name)

SYSFUNC_DEF(putstring)
SYSFUNC_DEF(open)
SYSFUNC_DEF(close)
SYSFUNC_DEF(read)
SYSFUNC_DEF(lseek)
SYSFUNC_DEF(write)
SYSFUNC_DEF(fork)
/* The child shares the user stack until exec/exit. Keep the parent's return
 * address in the saved register frame, away from the child's calls. */
__asm__(".global vfork\n"
        "vfork: popq %r9\n"
        "movq %rsp, %r11\n"
        "leaq 1f(%rip), %r10\n"
        "movq $" STR_NR(__NR_vfork) ", %rax\n"
        "sysenter\n"
        "1: cmpq $-4095, %rax\n"
        "jb 2f\n"
        "movq %rax, errno(%rip)\n"
        "movq $-1, %rax\n"
        "2: jmp *%r9\n");
SYSFUNC_DEF(execve)
SYSFUNC_DEF(brk)
SYSFUNC_DEF(reboot)
SYSFUNC_DEF(chdir)
SYSFUNC_DEF(fchdir)
SYSFUNC_DEF(getdents)
SYSFUNC_DEF(mmap)
SYSFUNC_DEF(munmap)
SYSFUNC_DEF(msync)
__SYSFUNC_DEF_(_exit, __NR_exit)


long errno;

__asm__ (   
    "LABEL_SYSCALL:                                 \n\t"
    "pushq  %r11                                    \n\t"
    "pushq  %r10                                    \n\t"
    "leaq 1f(%rip),   %r10                          \n\t"
    "movq   %rsp,  %r11                             \n\t"
    "sysenter                                       \n\t"
    "1:                                             \n\t"
    "xchgq  %rdx,  %r10                             \n\t"
    "xchgq  %rcx,  %r11                             \n\t"
    "popq   %r10                                    \n\t"
    "popq   %r11                                    \n\t"
    "cmpq   $-0x1000,   %rax                        \n\t"
    "jb     LABEL_SYSCALL_RET                       \n\t"
    "movq   %rax,       errno(%rip)                 \n\t"
    "orq    $-1,        %rax                        \n\t"
    "LABEL_SYSCALL_RET:                             \n\t"
    "retq                                           \n\t");



/*

*/
void memset(void *Src, char num, unsigned long n){
    char *src = (char *)Src;
    char *dst = src + n;
    while(src < dst) *(src++) = num;
}

/*

*/
void memcopy(void *Src, void *Dst, unsigned long n){

    char *src = (char *)Src;
    char *dst = (char *)Dst;

    short int IsBackcopy = 0;
    unsigned long SrcPtr = (unsigned long)src;
    unsigned long DstPtr = (unsigned long)dst;

    IsBackcopy = (DstPtr > SrcPtr && DstPtr < SrcPtr + n);

    if(IsBackcopy){
        dst = dst + n - 1;
        src = src + n - 1;
        while (n--) *(dst--) = *(src--);
    }else{
        while (n--) *(dst++) = *(src++);
    }
}