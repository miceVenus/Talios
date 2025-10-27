#include "memory.h"
#include "printk.h"
#include "lib.h"

struct GlobalMemDescriptor MMS;

extern char _text;
extern char _etext;
extern char _edata;
extern char _end;
unsigned long ZoneDmaIndex;
unsigned long ZoneNormalIndex;
unsigned long ZoneUnmapedIndex;

void InitMemory(){

    MMS = (struct GlobalMemDescriptor){
        .descriptor = {0},
        .GMDLength  =  0
    };

    MMS.StartCode   = (unsigned long)(&_text);
    MMS.EndCode     = (unsigned long)(&_etext);
    MMS.EndData     = (unsigned long)(&_edata);
    MMS.EndBrk      = (unsigned long)(&_end);
    

    ColorPrintfk(BLUE, BLACK, "TYPE(1. Normal RAM, 2. ROM OR Reserved, 3. ACPI Reclaimable Memory,\
        4. ACPI NVS memory, 5. Area containing bad memory)\n");
    struct E820 *p = (struct E820*)(MEM_STRUCT_ADDR);

    unsigned int  Temp          = 0;
    unsigned long TotalMemory   = 0;
    unsigned long StartAddr     = 0;
    unsigned long EndAddr       = 0;

    for(int i = 0; i < MAX_GMD_LEN; i++){

        ColorPrintfk(YELLOW, BLACK, "ADDR : %X, Length : %X, Type : %x \n", \
        p->address, p->length, p->type);

        if(p->type == 1) TotalMemory += p->length;

        MMS.descriptor[i].address = p->address;
        MMS.descriptor[i].length  = p->length;
        MMS.descriptor[i].type    = p->type;
        MMS.GMDLength++;

        p++;

        if(p->type > 4 || p->type < 1 || p->length == 0) break;

    }
    ColorPrintfk(BLUE, BLACK, "Memory OS Could Use Is %D B Totally\n", TotalMemory);
    unsigned long TotalPage2M = 0;

    // Count Total Page
    for(int i = 0; i < MMS.GMDLength; i++){

        if(MMS.descriptor[i].type != 1) continue;

        struct E820 *CurrentMD  =  MMS.descriptor + i; 

        StartAddr   = PAGE_2M_ALIGN_UP(CurrentMD->address);
        EndAddr     = PAGE_2M_ALIGN_DOWN((CurrentMD->address + CurrentMD->length));
        if(EndAddr  <= StartAddr) continue;
        TotalPage2M += (EndAddr - StartAddr) >> PAGE_2M_SHIFT; 
    }
    ColorPrintfk(BLUE, BLACK, "Page Num OS Have Is %D Totally\n", TotalPage2M);

    // Include Memory Gap And ROM
    TotalMemory =  MMS.descriptor[MMS.GMDLength - 1].address + MMS.descriptor[MMS.GMDLength - 1].length;
                    
    // Init BitsMap

    MMS.BitsMap = (unsigned long *)MEM_GAP_ALIGN(MMS.EndBrk);
    MMS.BitsMapSize = TotalMemory >> PAGE_2M_SHIFT;

    MMS.BitsMapLength = ((TotalMemory >> PAGE_2M_SHIFT) + 7) >> 3;

    memset(MMS.BitsMap, 0xff, MMS.BitsMapLength);

    // Init PageGroup

    MMS.PagesGroup  = (struct Page*)MEM_GAP_ALIGN(((unsigned long)MMS.BitsMap + MMS.BitsMapLength));
    MMS.PagesSize   = TotalMemory >> PAGE_2M_SHIFT;
    MMS.PagesLength = (TotalMemory >> PAGE_2M_SHIFT) * sizeof(struct Page);
    memset(MMS.PagesGroup, 0x00, MMS.PagesLength);

    // Init ZoneGroup

    MMS.ZonesGroup  = (struct Zone*)MEM_GAP_ALIGN((unsigned long)MMS.PagesGroup + MMS.PagesLength);
    MMS.ZonesSize   = 0; // Assume Here
    MMS.ZonesLength = 5 * sizeof(struct Zone);
    memset(MMS.ZonesGroup, 0x00, MMS.ZonesLength);

    for(int i = 0; i < MMS.GMDLength; i++){
        if(MMS.descriptor[i].type != 1)
        continue;

        StartAddr   = PAGE_2M_ALIGN_UP(MMS.descriptor[i].address);
        EndAddr     = PAGE_2M_ALIGN_DOWN(MMS.descriptor[i].address + MMS.descriptor[i].length);

        if(StartAddr >= EndAddr)
        continue;


        struct Zone * z = MMS.ZonesGroup + MMS.ZonesSize;

        z -> GMD           = &MMS;
        z -> Attribute     = 0;

        z -> PagesGroup    = MMS.PagesGroup + (StartAddr >> PAGE_2M_SHIFT);
        z -> PageFreeCount = (StartAddr - EndAddr) >> PAGE_2M_SHIFT;
        z -> PagesLength   = z -> PageFreeCount;
        z -> PageUsingCount= 0;
        z -> TotalPagesLink= 0;

        z -> ZoneEndAddr   = EndAddr;
        z -> ZoneStartAddr = StartAddr;
        z -> ZoneLength    = EndAddr - StartAddr;

        MMS.ZonesSize++;

        for(int j = 0; StartAddr < EndAddr; StartAddr += PAGE_2M_SIZE, j++){
            struct Page * p = MMS.PagesGroup + j;
            MMS.BitsMap[BITS_MAP_INDEX(StartAddr)] ^= BITS_MAP_OFFSET(StartAddr);

            // Init PagesGroup
            p -> age           = 0;
            p -> Attribute     = 0;
            p -> PhyAddr       = StartAddr;
            p -> RefCount      = 0;
            p -> ZoneStruct    = z;
        }
    }
    // !!!!!!!!!!!!!!!!!!!!!

    MMS.ZonesLength = MMS.ZonesSize * sizeof(struct Zone);

    ColorPrintfk(BLUE, BLACK, "BitsMap bitsmap in : %p, bitsmap size : %D b, bitsmap length : %D B\n", 
                                                    MMS.BitsMap, MMS.BitsMapSize, MMS.BitsMapLength);

    ColorPrintfk(BLUE, BLACK, "PageGroup pagegroup in : %p, pagegroup size : %D s, pagegroup length : %D B\n",
                                                    MMS.PagesGroup, MMS.PagesSize, MMS.PagesLength);

    ColorPrintfk(BLUE, BLACK, "ZoneGroup zonegroup in : %p, zonegroup size : %D s, zonegroup length : %D B\n",
                                                    MMS.ZonesGroup, MMS.ZonesSize, MMS.ZonesLength);
    
    // Marked Zone Which Is the Unmapped

    ZoneDmaIndex    = 0;
    ZoneNormalIndex = 0;

    for(int i = 0; i < MMS.ZonesSize; i++){
        struct Zone * z = MMS.ZonesGroup + i;
        if(z -> ZoneStartAddr == 0x100000000){
            ZoneUnmapedIndex = i;
        }
    }

    MMS.EndStruct = (unsigned long)MEM_GAP_ALIGN((MMS.ZonesGroup + MMS.ZonesSize));

    // Init Page Attribute Used In Kernel Init

    Temp =  VIRT_TO_PHY(PAGE_4K_ALIGN_UP(MMS.EndStruct)) >> PAGE_2M_SHIFT;

    for(int i = 0; i < Temp; i++){
        PageInit(MMS.PagesGroup + i, PATTR(PG_Active) | PATTR(PG_Kernel) | PATTR(PG_Kernel_Init) | PATTR(PG_PTable_Maped));
    }

    // Flush The Consistency Mapping 

    unsigned long CR3 = GetCr3();

    ColorPrintfk(BLUE, BLACK, "CR3 : %X ; PML4E : %X ; PDPTE : %X ; \n", CR3, *PHY_TO_VIRT(CR3), \
                                                                        *PHY_TO_VIRT(*(PHY_TO_VIRT(CR3)) & (~0xff)));
    *PHY_TO_VIRT(CR3 & (~0xff)) = 0;

    FlushTLB();
}

void PageInit(struct Page *p, unsigned long flag){
    if(!p -> Attribute){
        MMS.BitsMap[BITS_MAP_INDEX(p->PhyAddr)] |= BITS_MAP_OFFSET(p->PhyAddr);
        p -> RefCount++;
        p -> Attribute = flag;
        p -> ZoneStruct -> PageFreeCount--;
        p -> ZoneStruct -> PageUsingCount++;
        p -> ZoneStruct -> TotalPagesLink++;

    }
    else if ((p->Attribute & PATTR(PG_K_Share_To_U)) || (p->Attribute & PATTR(PG_Referenced)) ||
              (flag & PATTR(PG_K_Share_To_U)) || (flag & PG_Referenced))
    {
        p -> Attribute |= flag;
        p -> RefCount++;
        p -> ZoneStruct->TotalPagesLink++;
    }
    else
    {
        MMS.BitsMap[BITS_MAP_INDEX(p->PhyAddr)] |= BITS_MAP_OFFSET(p->PhyAddr);
        p -> Attribute |= flag;
    }
}
