#include "lib.h"
#include "mmap.h"

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
    __asm__ volatile("rep stosb" : "+D"(Src), "+c"(n) : "a"((unsigned char)num) : "memory");
}

/*

*/
void memcopy(void *Src, void *Dst, size_t n){

    unsigned char *src = Src, *dst = Dst;
    if((uintptr_t)dst > (uintptr_t)src && (uintptr_t)dst - (uintptr_t)src < n){
        while(n){ n--; dst[n] = src[n]; }
    }else{
        __asm__ volatile("rep movsb" : "+S"(src), "+D"(dst), "+c"(n) :: "memory");
    }
}


/*
    @brief Reverse a string simply;
    @param Str Target String
    @return Reversed String
*/
char *StringReverse(char *Str){
    for(int l = 0, r = StringLen(Str) - 1; l < r; l++, r--)
        SwitchMem(&Str[l], &Str[r]);

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

int strcmp(char *str1, char *str2){
    while(*str1 && *str2 && (*str1 == *str2)) {str1++; str2++;}
    return *str1 - *str2;
}

void upper_case(char *str){
    while(*str){
        if(is_char(*str) && *str >= 'a') *str = *str + 'A' - 'a';
        str++;
    }
}

void lower_case(char *str){
    while(*str){
        if(is_char(*str) && *str <= 'Z') *str = *str - 'A' + 'a';
        str++;
    }
}

/* Return the number of bytes not copied, as expected by uaccess callers. */
long copy_from_user(const void *from, void *to, unsigned long size){
    vm_lock();
    int rc = vm_copy_user_locked((void *)from, to, size, 0);
    vm_unlock();
    return rc ? (long)size : 0;
}

long copy_to_user(void *from, void *to, unsigned long size){
    vm_lock();
    int rc = vm_copy_user_locked(to, from, size, 1);
    vm_unlock();
    return rc ? (long)size : 0;
}

long strncpy_from_user(const char *src, char *dst, unsigned long size){
    long rc = copy_from_user(src, dst, size);
    if(!rc) dst[size] = 0;
    return rc;
}

long strnlen_user(const char *src, unsigned long size){
    unsigned long total = 0;
    vm_lock();
    while(total < size){
        const char *p = src + total;
        unsigned long part = min(size - total, PAGE_4K_SIZE - ((unsigned long)p & (PAGE_4K_SIZE - 1)));
        if(vm_copy_user_locked((void *)p, NULL, part, 0)){ vm_unlock(); return -1; }
        for(unsigned long n = 0; n < part; n++, total++){
            if(!p[n]){ vm_unlock(); return total; }
        }
    }
    vm_unlock();
    return total;
}
