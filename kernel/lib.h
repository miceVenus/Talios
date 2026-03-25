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
#define NULL 0UL
#endif


#define STR(x) #x

#define CONCAT(x1, x2) x1 ## x2

#define sti()  __asm__ volatile("sti":::"memory")

#define cli()  __asm__ volatile("cli":::"memory")

#define nop()  __asm__ volatile("nop":::"memory")

#define hlt()  __asm__ volatile("hlt":::"memory")

#define mfence() __asm__ volatile("mfence":::"memory")

#define bochs_bp() __asm__ volatile("xchg %bx, %bx")


#define GetCr3() ({ \
    unsigned long temp;\
    __asm__ volatile("movq %%cr3, %0":"=r"(temp)::"memory");\
    temp;})

#define SetCr3(num) __asm__ volatile("movq %0, %%cr3"::"r"(num):"memory")

#define OpenA20()   (OUT8b(0x64, 0xd1);OUT8b(0x60, 0xDF);)
#define SysRestart()(OUT8b(0x64, 0xfe);)

#define FlushTLB() ({   \
    unsigned long temp; \
    temp = GetCr3();    \
    SetCr3(temp);})


#define DoDiv(num, base) ({\
        int __res; \
        __asm__ volatile("divq %%rcx":"=a"(num), "=d"(__res):"0"(num), "1"(0), "c"(base)); \
        __res; })

#define GetBits(num, pos, count) ((num) >> (pos) & ((1UL << (count)) - 1))

#define BCD_TO_BIN(value) (GetBits(value, 4, 4) * 10 + GetBits(value, 0, 4))

#define is_num(char) (((char) <= '9' && (char) >= '0') ? 1 : 0)
#define is_char(char) (((char) <= 'z' && (char) >= 'a') ||((char) <= 'Z' && (char) >= 'A') ? 1 : 0)

// ONLY IN GNU C !!!!!!
#define SwitchMem(Ptr1, Ptr2) ({\
        typeof(*(Ptr1)) Temp; \
        Temp = *Ptr1; \
        *Ptr1 = *Ptr2; \
        *Ptr2 = Temp; })

#define max(value1, value2) ((value1) >= (value2) ? (value1) : (value2))
#define min(value1, value2) ((value1) >= (value2) ? (value2) : (value1))

#define ContainerOf(ptr, type, member)   ({                             \
    typeof(ptr) p = (ptr);                                              \
    (type *)((unsigned long)p - (unsigned long)&(((type *)0)->member));  \
})

struct List{
    struct List *prev;
    struct List *next;
};

static inline void wrmsr(unsigned long addr, unsigned long content){
    __asm__ volatile("wrmsr "::"c"(addr), "d"(content >> 32), "a"(content & 0xffffffff));
}

static inline unsigned long rdmsr(unsigned long addr){
    unsigned int res1;
    unsigned long res2;
    
    __asm__ volatile("rdmsr ":"=d"(res2), "=a"(res1):"c"(addr));

    return (res2 << 32) + res1;
}

// must be num in ax and port in dx
static inline void OUT8b(unsigned int port, unsigned char byte){
    __asm__ volatile("outb %1, %%dx" : : "d"(port), "a"(byte): "memory");
}

static inline unsigned char IN8b(unsigned int port){
    unsigned char num;
    __asm__ volatile("inb %%dx, %0" :"=a"(num): "d"(port): "memory");
    return num;
}

static inline void OUT32b(unsigned int port, unsigned int num){
    __asm__ volatile("outl %1, %%dx" : : "d"(port), "a"(num): "memory");
}


static inline unsigned int IN32b(unsigned int port){
    unsigned int num;
    __asm__ volatile("inl %%dx, %0" :"=a"(num): "d"(port): "memory");
    return num;
}

static inline void port_insw(void *buffer, unsigned int port, unsigned long num){
    __asm__ volatile("rep insw":"+D"(buffer), "+c"(num):"d"(port):"memory");
}

static inline void port_outsw(void *buffer, unsigned int port, unsigned long num){
    __asm__ volatile("rep outsw":"+D"(buffer), "+c"(num):"d"(port):"memory");
}

inline void ListInit(struct List *list){
    list->prev = list;
    list->next = list;
}

inline void ListForeAdd(struct List *new, struct List *list){

    new->next = list;
    new->prev = list->prev;
    list->prev->next = new;
    list->prev = new;

}

inline void ListBackAdd(struct List *list, struct List *new){
    
    new->prev = list;
    new->next = list->next;
    list->next->prev = new;
    list->next = new;
}

inline struct List* ListNext(struct List *list){
    return list->next;
}

inline struct List* ListPrev(struct List *list){
    return list->prev;
}

inline int ListIsEmpty(struct List *list){
    if(list -> next == list && list -> prev == list)
        return 1;
    return 0;
}

inline int ListDelete(struct List *list){
    list->prev->next = list->next;
    list->next->prev = list->prev;
    return 1;
}


inline unsigned long get_rflags()
{
	unsigned long tmp = 0;
	__asm__ __volatile__	("pushfq	\n\t"
				 "movq	(%%rsp), %0	\n\t"
				 "popfq	\n\t"
				:"=r"(tmp)::"memory");
	return tmp;
}


void    memset(void *Src, char num, size_t n);
void    memcopy(void *Src, void *Dst, size_t n);

int     StringLen(char *Str);
char*   StringReverse(char *Str);
void    StringCopy(char *Src, char *Dst);
void    StringNCopy(char *Src, char *Dst, size_t n);
int     NumToString(char *Buffer, long Num, unsigned int base, int Upper);
void    upper_case(char *str);
void    lower_case(char *str);
int     strcmp(char *str1, char *str2);

#endif
