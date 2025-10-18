#include "memory.h"
#include "printk.h"



void InitMemory(){
    ColorPrintfk(BLUE, BLACK, "TYPE(1. Normal RAM, 2. ROM OR Reserved, 3. ACPI Reclaimable Memory,\
        4. ACPI NVS memory, 5. Area containing bad memory)\n");
    struct MemoryE820Formate *p = (struct MemoryE820Formate*)(MEM_STRUCT_ADDR);
    unsigned long TotalMemory = 0;
    for(int i = 0; i < 32; i++){
        ColorPrintfk(YELLOW, BLACK, "ADDR : %x, %x, Length : %x, %x, Type : %x \n", \
        p->addrH, p->addrL, p->lengthH, p->lengthL, p->type);

        if(p->type == 1) TotalMemory += (p->lengthH << 32) + p->lengthL;

        p++;

        if(p->type > 4) {
            break;
        };
    }
    ColorPrintfk(BLUE, BLACK, "Memory OS Could Use Is %D B Totally\n", TotalMemory);
}