#include "lib.h"

/*
    @brief Transfer A Num To A String Simply
    @param Buffer Because that we Can't use malloc for now
    @param Num Target Num
    @param base Max 36;
    @param Upper 
    @return Buffer Len;

*/
int NumToString(char *Buffer, long Num, unsigned int base, int Upper){
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

/*

*/
void memset(void *Src, char num, size_t n){
    char *src = (char *)Src;
    char *dst = src + n;
    while(src < dst) *(src++) = num;
}

/*

*/
void memcopy(void *Src, void *Dst, size_t n){

    char *src = (char *)Src;
    char *dst = (char *)Dst;

    short int IsBackcopy = 0;
    uintptr_t SrcPtr = (uintptr_t)src;
    uintptr_t DstPtr = (uintptr_t)dst;

    IsBackcopy = (DstPtr > SrcPtr && DstPtr < SrcPtr + n);

    if(IsBackcopy){
        dst = dst + n - 1;
        src = src + n - 1;
        while (n--) *(dst--) = *(src--);
    }else{
        while (n--) *(dst++) = *(src++);
    }
}


/*
    @brief Reverse a string simply;
    @param Str Target String
    @return Reversed String
*/
char *StringReverse(char *Str){
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

/*

*/
void StringCopy(char *Src, char *Dst){
    char *Ptr = Src;
    while(*Ptr != '\0') *(Dst++) = *(Ptr++);
    *Dst = '\0';
}

/*

*/
void StringNCopy(char *Src, char *Dst, size_t n){
    char *Ptr = Src;
    size_t Len = 0;
    while(*Ptr != '\0' && Len < n){
        *(Dst++) = *(Ptr++);
        Len++;
    }
    Len < n ? *Dst = '\0' : 0;
}

/*

*/
int StringLen(char *Str){
    char *Ptr = Str;
    while(*Ptr != '\0') Ptr++;
    return Ptr - Str;
}