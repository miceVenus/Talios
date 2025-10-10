#ifndef LIB_H
#define LIB_H

typedef unsigned long uintptr_t;

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

static inline int ToDeciString(char *Buffer, int Decimal){
    return 1;
}

static inline int ToHexString(char *Buffer, int Decimal, int Upper){
    return 1;
}

static inline int ToPtrString(char *Buffer, void *Ptr){
    return 1;
}

#endif
