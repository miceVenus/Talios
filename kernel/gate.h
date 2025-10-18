#ifndef GATE_H
#define GATE_H

#define LTR(n) do{\
    unsigned long TR;\
    unsigned long Selector = (n << 3);\
    __asm__ volatile("str %0" : "=r"(TR));\
    if(TR != Selector) __asm__ volatile("ltr %%ax"::"a"(Selector):"memory");\
} while(0)

#define SetIdtGate(GateSelector, flag, Ist, CodeAddr) do {                \
        __asm__ volatile(                                               \
                        "xorq    %%rax,  %%rax                  \n\t"   \
                        "xorq    %%rbx,  %%rbx                  \n\t"   \
                        "movw    %%cx,   %%ax                   \n\t"   \
                        "shrq    $16,    %%rcx                  \n\t"   \
                        "movw    %%cx,   %%bx                   \n\t"   \
                        "shrq    $16,    %%rcx                  \n\t"   \
                        "shlq    $48,    %%rbx                  \n\t"   \
                        "addq    %%rbx,  %%rax                  \n\t"   \
                        "shlq    $40,    %%rsi                  \n\t"   \
                        "addq    %%rsi,  %%rax                  \n\t"   \
                        "shlq    $32,    %%rdx                  \n\t"   \
                        "addq    %%rdx,  %%rax                  \n\t"   \
                        "movq    $(0x8 << 16),      %%rbx       \n\t"   \
                        "addq    %%rbx,  %%rax                  \n\t"   \
                        "movq    %%rax,  (%%rdi)                \n\t"   \
                        "movq    %%rcx,  8(%%rdi)               \n\t"   \
                        : \
                        :   "D"(GateSelector), "S"((unsigned long)(flag)),  \
                            "d"((unsigned long)(Ist)), "c"((unsigned long)(CodeAddr)) \
                        : "rax", "rbx", "memory"); \
} while(0)

extern unsigned int TssTable[26];

struct GateStruct{
    unsigned long low;
    unsigned long high;
};
extern struct GateStruct IdtTable[256];

static inline void SetIntrGate(unsigned int Num, char Ist, void* Addr){
    SetIdtGate(IdtTable + Num, 0x8E, Ist, Addr);  // P = 1 DPL = 0 GateType = 0b1110;
}


static inline void SetSystemGate(unsigned int Num, char Ist, void* Addr){
    SetIdtGate(IdtTable + Num, 0xEF, Ist, Addr);  // P = 1 DPL = 11 GateType = 0b1111;
}


static inline void SetTrapGate(unsigned int Num, char Ist, void* Addr){
    SetIdtGate(IdtTable + Num, 0x8F, Ist, Addr);  // P = 1 DPL = 0 GateType = 0b1111;
}

void SetTss(unsigned long rsp0,unsigned long rsp1,unsigned long rsp2,unsigned long ist1,\
            unsigned long ist2,unsigned long ist3,unsigned long ist4,unsigned long ist5,\
            unsigned long ist6,unsigned long ist7);

#endif