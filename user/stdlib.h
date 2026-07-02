#ifndef STDLIB_H
#define STDLIB_H


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


void memset(void *Src, char num, unsigned long n);

void memcopy(void *Src, void *Dst, unsigned long n);

#endif