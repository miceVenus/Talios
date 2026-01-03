#ifndef CPU_H
#define CPU_H

static inline void CPUID(  unsigned int mop, unsigned int sop, unsigned int* eax, 
                    unsigned int* ebx, unsigned int* ecx, unsigned int* edx){

    __asm__ volatile("cpuid     \n\t"                           
                    :"=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) 
                    :"0"(mop),"2"(sop));              
}

void CpuInit();

#endif