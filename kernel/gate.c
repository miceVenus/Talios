#include "gate.h"

void SetTss(unsigned long rsp0,unsigned long rsp1,unsigned long rsp2,unsigned long ist1,\
            unsigned long ist2,unsigned long ist3,unsigned long ist4,unsigned long ist5,\
            unsigned long ist6,unsigned long ist7){
    
    *((unsigned long*)(TssTable + 1)) = rsp0;
    *((unsigned long*)(TssTable + 3)) = rsp1;
    *((unsigned long*)(TssTable + 5)) = rsp2;
    *((unsigned long*)(TssTable + 9)) = ist1;
    *((unsigned long*)(TssTable + 11)) = ist2;
    *((unsigned long*)(TssTable + 13)) = ist3;
    *((unsigned long*)(TssTable + 15)) = ist4;
    *((unsigned long*)(TssTable + 17)) = ist5;
    *((unsigned long*)(TssTable + 19)) = ist6;
    *((unsigned long*)(TssTable + 21)) = ist7;
}
