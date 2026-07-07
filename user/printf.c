#include "stdio.h"

#include "stdlib.h"

#include "string.h"

#define MAX_BUFFER_LEN 1024

int printf(const char * restrict format, ...){

    char buf[MAX_BUFFER_LEN];

    int length;
    va_list ap;
    va_start(ap, format);

    length = vsprintf(buf, format, ap);

    va_end(ap);

    putstring(buf);

    return length;
}

int sprintf(char * restrict s, const char * restrict format, ...){

    int length;
    va_list ap;
    va_start(ap, format);

    length = vsprintf(s, format, ap);

    va_end(ap);

    return length;
}

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


int vsprintf(char * restrict s, const char * restrict format, va_list ap){
        
    char    CurrentChar  =  *(format++);
    int     BufferIndex  =  0;
    char    TempBuffer[40];

    int     Upper = 0;
    unsigned long ArgNum = 0;
    int     IndexIncre = 0;
    char    *ArgString = NULL;
    void    *ArgPtr = NULL;

    while(CurrentChar   != '\0'){

        if(CurrentChar  == '%'){

            CurrentChar = *(format++);

            if(CurrentChar == '\0') break;



            switch (CurrentChar){
            case '%':
                s[BufferIndex++] = '%';
                break;

            case 'x':
            case 'X':
                Upper = CurrentChar < 'a' ? 1 : 0; 
                
                ArgNum  = Upper ? va_arg(ap, unsigned long): va_arg(ap, unsigned int); // break the Protocol Just For Easy

                IndexIncre = NumToString(TempBuffer, ArgNum, 16, Upper);

                if(IndexIncre + BufferIndex >= MAX_BUFFER_LEN){
                    CurrentChar = 0;
                    break;
                }

                strcpy(TempBuffer, s + BufferIndex);
                BufferIndex += IndexIncre;
                break;

            case 'd':
            case 'D':
                ArgNum = CurrentChar == 'D' ? va_arg(ap, long) : va_arg(ap, int);

                IndexIncre = NumToString(TempBuffer, ArgNum, 10, 0);

                if(IndexIncre + BufferIndex >= MAX_BUFFER_LEN){
                    CurrentChar = 0;
                    break;
                }

                strcpy(TempBuffer, s + BufferIndex);
                BufferIndex += IndexIncre;
                break;

            case 's':
                ArgString = va_arg(ap, char*);
                IndexIncre = strlen(ArgString);

                if(IndexIncre + BufferIndex >= MAX_BUFFER_LEN){
                    CurrentChar = 0;
                    break;
                }

                strcpy(ArgString, s + BufferIndex);
                BufferIndex += IndexIncre;
                break;

            case 'c':
                s[BufferIndex++] = va_arg(ap, int);
                break;
            
            case 'p':              
                ArgPtr = va_arg(ap, void*);

                IndexIncre = NumToString(TempBuffer, (long)ArgPtr, 16, 0);

                if(IndexIncre + BufferIndex >= MAX_BUFFER_LEN){
                    CurrentChar = 0;
                    break;
                }

                strcpy(TempBuffer, s + BufferIndex);
                BufferIndex += IndexIncre;
                break;

            default:
                s[BufferIndex++] = '?';
                break;
            }
            
            CurrentChar = *(format++);

        }else{

            s[BufferIndex++] = CurrentChar;
            CurrentChar = *(format++);

        }
    }

    s[BufferIndex] = '\0';
    return BufferIndex;
}