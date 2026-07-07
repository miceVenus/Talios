#include "string.h"
#include "stdlib.h"

char *strrvs(char *Str){
    int Length = strlen(Str);
    char *TailPtr = Length - 1 + Str;
    char *HeadPtr = Str;
    while(TailPtr != HeadPtr){
        SwitchMem(HeadPtr, TailPtr);
        HeadPtr++;
        TailPtr--;
    }

    return Str;
}


void strncat(char *t_str, char *s_str, unsigned long n){
    strncpy(s_str, t_str + strlen(t_str), n);
}

void strcpy(char *Src, char *Dst){
    char *Ptr = Src;
    while(*Ptr != '\0') *(Dst++) = *(Ptr++);
    *Dst = '\0';
}

void strncpy(char *Src, char *Dst, size_t n){
    char *Ptr = Src;
    size_t Len = 0;
    while(*Ptr != '\0' && Len < n){
        *(Dst++) = *(Ptr++);
        Len++;
    }
    Len < n ? *Dst = '\0' : 0;
}

int strlen(char *Str){
    char *Ptr = Str;
    while(*Ptr != '\0') Ptr++;
    return Ptr - Str;
}

int strcmp(char *str1, char *str2){
    while(*str1 && *str2 && (*str1 == *str2)) {str1++; str2++;}
    return *str1 - *str2;
}