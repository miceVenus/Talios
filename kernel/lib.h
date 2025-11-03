#ifndef LIB_H
#define LIB_H


#ifndef uintptr_t
typedef unsigned long uintptr_t;
#endif

#ifndef size_t
typedef unsigned long size_t;
#endif

#ifndef uintptr_t
typedef unsigned char uint8_t;
#endif

#ifndef NULL
#define NULL (void *)0
#endif


#define STR(x) #x

#define CONCAT(x1, x2) x1 ## x2

#define sti()  __asm__ volatile("sti":::"memory")

#define cli()  __asm__ volatile("cli":::"memory")

#define GetCr3() ({ \
    unsigned long temp;\
    __asm__ volatile("movq %%cr3, %0":"=r"(temp)::"memory");\
    temp;})

#define SetCr3(num) __asm__ volatile("movq %0, %%cr3"::"r"(num):"memory")

#define FlushTLB() ({   \
    unsigned long temp; \
    temp = GetCr3();    \
    SetCr3(temp);})


#define DoDiv(num, base) ({\
        int __res; \
        __asm__ volatile("divq %%rcx":"=a"(num), "=d"(__res):"0"(num), "1"(0), "c"(base)); \
        __res; })


// ONLY IN GNU C !!!!!!
#define SwitchMem(Ptr1, Ptr2) ({\
        typeof(*(Ptr1)) Temp; \
        Temp = *Ptr1; \
        *Ptr1 = *Ptr2; \
        *Ptr2 = Temp; })

// must be num in ax and port in dx
static inline void OUT8b(unsigned char port, unsigned char byte){
    __asm__ volatile("outb %1, %%dx" : : "d"(port), "a"(byte): "memory");
}


static inline unsigned char IN8b(unsigned char port){
    unsigned char num;
    __asm__ volatile("inb %%dx, %0" :"=a"(num): "d"(port): "memory");
    return num;
}

static inline void OUT32b(unsigned char port, unsigned int num){
    __asm__ volatile("outl %1, %%dx" : : "d"(port), "a"(num): "memory");
}


static inline unsigned int IN32b(unsigned char port){
    unsigned int num;
    __asm__ volatile("inl %%dx, %0" :"=a"(num): "d"(port): "memory");
    return num;
}

void    memset(void *Src, char num, size_t n);
void    memcopy(char *Src, char *Dst, size_t n);

int     StringLen(char *Str);
char*   StringReverse(char *Str);
void    StringCopy(char *Src, char *Dst);
void    StringNCopy(char *Src, char *Dst, size_t n);
int     NumToString(char *Buffer, long Num, unsigned int base, int Upper);

#endif
