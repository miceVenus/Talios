#include "memory.h"
#include "printk.h"
#include "lib.h"

struct GlobalMemDescriptor MemoryManagerStruct;

extern char _text;
extern char _etext;
extern char _edata;
extern char _end;

void InitMemory(){

    MemoryManagerStruct = (struct GlobalMemDescriptor){
        .descriptor = {0},
        .GMDLength  =  0
    };

    MemoryManagerStruct.StartCode   = (unsigned long)(&_text);
    MemoryManagerStruct.EndCode     = (unsigned long)(&_etext);
    MemoryManagerStruct.EndData     = (unsigned long)(&_edata);
    MemoryManagerStruct.EndBrk      = (unsigned long)(&_end);
    

    ColorPrintfk(BLUE, BLACK, "TYPE(1. Normal RAM, 2. ROM OR Reserved, 3. ACPI Reclaimable Memory,\
        4. ACPI NVS memory, 5. Area containing bad memory)\n");
    struct E820 *p = (struct E820*)(MEM_STRUCT_ADDR);

    unsigned long TotalMemory   = 0;
    unsigned long StartAddr     = 0;
    unsigned long EndAddr       = 0;

    for(int i = 0; i < MAX_GMD_LEN; i++){

        ColorPrintfk(YELLOW, BLACK, "ADDR : %X, Length : %X, Type : %x \n", \
        p->address, p->length, p->type);

        if(p->type == 1) TotalMemory += p->length;

        MemoryManagerStruct.descriptor[i].address = p->address;
        MemoryManagerStruct.descriptor[i].length  = p->length;
        MemoryManagerStruct.descriptor[i].type    = p->type;
        MemoryManagerStruct.GMDLength++;

        p++;

        if(p->type > 4 || p->type < 1 || p->length == 0) break;

    }
    ColorPrintfk(BLUE, BLACK, "Memory OS Could Use Is %D B Totally\n", TotalMemory);
    unsigned long TotalPage2M = 0;

    // Count Total Page
    for(int i = 0; i < MemoryManagerStruct.GMDLength; i++){

        if(MemoryManagerStruct.descriptor[i].type != 1) continue;

        struct E820 *CurrentMD  =  MemoryManagerStruct.descriptor + i; 

        StartAddr   = PAGE_2M_ALIGN(CurrentMD->address);
        EndAddr     = (CurrentMD->address + CurrentMD->length) & PAGE_2M_MASK;
        if(EndAddr  <= StartAddr) continue;
        TotalPage2M += (EndAddr - StartAddr) >> PAGE_2M_SHIFT; 
    }
    ColorPrintfk(BLUE, BLACK, "Page Num OS Have Is %D Totally\n", TotalPage2M);


    // Init BitsMap
    MemoryManagerStruct.BitsMap = (unsigned long *)PAGE_4K_ALIGN(MemoryManagerStruct.EndBrk);
    MemoryManagerStruct.BitsMapSize = TotalPage2M;

    MemoryManagerStruct.BitsMapLength = (TotalPage2M + 7) >> 3;

    memset(MemoryManagerStruct.BitsMap, 0x00, MemoryManagerStruct.BitsMapLength);
}