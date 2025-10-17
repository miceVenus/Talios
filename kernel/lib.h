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

#define sti()  __asm__ volatile("sti":::"memory")
#define cli()  __asm__ volatile("cli":::"memory")


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

#define STR(x) #x

#define CONCAT(x1, x2) x1 ## x2

static inline void StringCopy(char *Src, char *Dst){
    char *Ptr = Src;
    while(*Ptr != '\0') *(Dst++) = *(Ptr++);
    *Dst = '\0';
}

static inline void StringNCopy(char *Src, char *Dst, size_t n){
    char *Ptr = Src;
    size_t Len = 0;
    while(*Ptr != '\0' && Len < n){
        *(Dst++) = *(Ptr++);
        Len++;
    }
    Len < n ? *Dst = '\0' : 0;
}

static inline int StringLen(char *Str){
    char *Ptr = Str;
    while(*Ptr != '\0') Ptr++;
    return Ptr - Str;
}

/// @brief Reverse a string simply;
/// @param Str Target String
/// @return Reversed String
static char *StringReverse(char *Str){
    int Length = StringLen(Str);
    char *TailPtr = Length - 1 + Str;
    char *HeadPtr = Str;
    while(TailPtr != HeadPtr){
        SwitchMem(HeadPtr, TailPtr);
        HeadPtr++;
        TailPtr--;
    }

    return Str;
}

inline void MemCopy(char *Src, char *Dst, size_t n){

    short int IsBackcopy = 0;
    uintptr_t SrcPtr = (uintptr_t)Src;
    uintptr_t DstPtr = (uintptr_t)Dst;

    IsBackcopy = (SrcPtr + n > DstPtr);

    if(IsBackcopy){
        Dst = Dst + n;
        Src = Src + n;
        while (n--) *(Dst--) = *(Src--);
    }else{
        while (n--) *(Dst++) = *(Src++);
    }
}

inline void MemSet(char *Src, size_t n, char num){
    char *Dst = Src + n;
    while(Src < Dst) *(Src++) = num;
}

/// @brief Transfer A Num To A String Simply
/// @param Buffer Because that we Can't use malloc for now
/// @param Num Target Num
/// @param base Max 36;
/// @param Upper 
/// @return Buffer Len;
static int NumToString(char *Buffer, long Num, unsigned int base, int Upper){
    int Res;
    unsigned char IsMinus = Num < 0 && (base == 10) ? 1 : 0;
    char *Ptr = Buffer;
    char Character;
    
    if(!Num)
        *(Ptr++) =  '0';
        
    while(Num){
        Res = DoDiv(Num, base);
        if(Res > 9) Character = Upper ? Res - 10 + 'A': Res - 10 + 'a';
        else        Character = Res + '0';
        *(Ptr++) =  Character;
    }

    if(IsMinus)     *(Ptr++) =  '-';
    if(base == 16){
        *(Ptr++) =  'x';
        *(Ptr++) =  '0';
    }

    char *TailPtr = Ptr - 1;
    char *HeadPtr = Buffer;
    while(HeadPtr < TailPtr) {
        SwitchMem(HeadPtr, TailPtr);
        HeadPtr++;
        TailPtr--;
    }

    *(Ptr++) =  '\0';

    return Ptr - Buffer - 1;
}

#endif
