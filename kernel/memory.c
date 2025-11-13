#include "memory.h"
#include "printk.h"
#include "lib.h"
#include "task.h"

void ListInit(struct List *list);

void ListForeAdd(struct List *new, struct List *list);
void ListBackAdd(struct List *List, struct List *new);

struct List* ListNext(struct List *list);

int ListIsEmpty(struct List* list);

struct GlobalMemManager MMS;

extern char _text;
extern char _etext;
extern char _edata;
extern char _end;

unsigned long ZoneDmaIndex;
unsigned long ZoneNormalIndex;
unsigned long ZoneUnmapedIndex;

struct Slab* CreatSlab(unsigned long size, int ZoneSelector){
    struct Slab* slab = (struct Slab*)kmalloc(sizeof(struct Slab));

    if(!slab) return NULL;

    memset(slab, 0, sizeof(struct Slab));

    ListInit(&(slab->list));
    slab->FreeCount     = PAGE_2M_SIZE / size;
    slab->ColorLength   = ((slab->FreeCount + (1UL << 6) - 1) >> 6);
    slab->ColorMap      = (unsigned long*)kmalloc(sizeof(unsigned long) * slab->ColorLength);

    if(!(slab->ColorMap)) {free(slab); return NULL;}

    memset(slab->ColorMap, 0, sizeof(unsigned long) * slab->ColorLength);

    slab->page          = AllocPage(ZoneSelector, 1, PATTR(PG_Kernel));

    if(!(slab->page)) {kfree(slab -> ColorMap); kfree(slab); return NULL;}

    slab->Vaddress      = PHY_TO_VIRT(slab->page->PhyAddr);

    return slab;
}

struct SlabCache* CreateSlabCache(  unsigned long SlabSize, void *(*Constructor)(void *Vaddr, unsigned long arg), 
                                    void *(*Destructor)(void *Vaddr, unsigned long arg), unsigned long arg){
    struct SlabCache *SC= (struct SlabCache *)kmalloc(sizeof(struct SlabCache));
    if(SC == NULL)  return NULL;

    memset(SC, 0, sizeof(struct SlabCache));

    unsigned long size = ALIGN_WITH_LONG(SlabSize);

    SC -> Constructor   =   Constructor;
    SC -> Destructor    =   Destructor;
    SC -> TotalUse      =   0;

    SC -> CachePool     =   CreatSlab(size, ZONE_NORMAL_INDEX);
    if(!(SC -> CachePool)) {kfree(SC); return NULL;}

    SC -> TotalFree     +=  SC->CachePool->FreeCount;
    SC -> size          =   size;

    SC -> CacheDmaPool  = NULL;

    return SC;
}

int DeleteSlabCache(struct SlabCache *SC){
    if(SC->TotalUse) return 0;
    struct Slab* slab       = SC->CachePool;
    struct Slab* tmp_slab   = NULL;
    while(!ListIsEmpty(&slab->list)){
        kfree(slab->ColorMap);
        PageClean(slab->page);
        PageFree(slab->page, 1);
        tmp_slab = slab;
        slab = ContainerOf(ListNext(&slab->list), struct Slab, list);
        kfree(tmp_slab);
    }
    kfree(slab->ColorMap);
    PageClean(slab->page);
    PageFree(slab->page, 1);
    kfree(slab);
    kfree(SC);
    return 1;
}

void * AllocSlab(struct SlabCache *SC, unsigned long arg){

    struct Slab *slab;
    if(SC->TotalFree == 0){
        slab = CreatSlab(SC->size, ZONE_NORMAL_INDEX);
        if(slab == NULL) {ColorPrintfk(RED, BLACK, "CreatSlab Fail In AllocSlab"); return NULL;}
        ListForeAdd(&slab->list, &SC->CachePool->list);
        SC->TotalFree += slab->FreeCount;
    }

    slab = SC->CachePool;

    do{
        if(slab->FreeCount == 0){
            slab = ContainerOf(ListNext(&slab->list), struct Slab, list);
            continue;
        }else{
            for(unsigned long i = 0; i < slab->ColorCount; i++){
                if(*(slab->ColorMap + (i >> 6)) == 0xffffffffffffffff){
                    i += 63;
                    continue;
                };
                if(*(slab->ColorMap + (i >> 6)) & (1UL << i % 64) == 0){
                    *(slab->ColorMap + (i >> 6)) |= (1UL << (i % 64));
                    slab->FreeCount--;
                    slab->UsingCount++;

                    SC->TotalFree--;
                    SC->TotalUse++;

                    void *Vaddr = (void *)((unsigned long)slab->Vaddress + (i * SC->size));
                    if(SC->Constructor != NULL) return SC->Constructor(Vaddr, arg);
                    else return Vaddr;
                }
            }
        }
        slab = ContainerOf(ListNext(&slab->list), struct Slab, list);
    }while(slab != SC->CachePool);

    ColorPrintfk(RED, BLACK, "There Is No Slap Could Be Alloc");
    return NULL;
}

unsigned long FreeSlab(struct SlabCache *SC, void *Vaddress, unsigned long arg){

    struct Slab *slab;

    slab = SC->CachePool;
    unsigned long index = 0;

    do{
        if(slab->Vaddress <= Vaddress && Vaddress - slab->Vaddress < PAGE_2M_SIZE){
            index = (unsigned long)(Vaddress - slab->Vaddress) / SC->size;
            *(slab->ColorMap + (index >> 6)) ^= (1UL << index % 64);
            slab->FreeCount++;
            slab->UsingCount--;

            SC->TotalFree++;
            SC->TotalUse--;

            void *Vaddr = (void *)((unsigned long)slab->Vaddress + (index * SC->size));
            if(SC->Destructor != NULL) SC->Destructor(Vaddr, arg);

            if(slab->UsingCount == 0 && SC->TotalFree >= slab->ColorCount * 1.5){
                SC->TotalFree -= slab->ColorCount;
                kfree(slab->ColorMap);
                PageClean(slab->page);
                PageFree(slab->page, 1);
                kfree(slab);
            }
            return 1;
        }else{
            slab = ContainerOf(ListNext(&slab->list), struct Slab, list);
        }
    }while(slab != SC->CachePool);

    ColorPrintfk(RED, BLACK, "Error In FreeSlab() Address Is Illegal\n");
    return 0;
}
/// @brief ugly but useful
/// @param ZoneSelector An Enum Type Defined In memory.h To Control The Type Of Memory
/// @param number   0 < Number <= 64
/// @param PageAttr An Enum Type Defined In memory.h To Describe Page 
/// @return Alloced Memory Start
struct Page* AllocPage(int ZoneSelector, int number, unsigned long PageAttr){
    
    if(number <= 0 || number > 64) return NULL;

    struct Zone *z;
    struct Page *p;
    unsigned long value;
    unsigned long ZoneStart, ZoneEnd;
    unsigned long pattern = number == 64 ? 0xffffffffffffffffUL : ~((1UL << (BITS_PER_LONG - number)) - 1);
    unsigned long PatternBak = pattern;
    unsigned long i, j, k, l, tmp;

    switch (ZoneSelector){
        case ZONE_DMA_INDEX:
            ZoneStart   = 0;
            ZoneEnd     = ZoneDmaIndex; 
            goto LABEL_HANDLE;

        case ZONE_NORMAL_INDEX:
            ZoneStart   = ZoneDmaIndex;
            ZoneEnd     = ZoneNormalIndex;
            goto LABEL_HANDLE;

        case ZONE_UNMAPED_INDEX:
            ZoneStart   = ZoneUnmapedIndex;
            ZoneEnd     = MMS.ZonesCount - 1;
            goto LABEL_HANDLE;
        
        default:
            ColorPrintfk(YELLOW, BLACK, "Unknown ZoneSelector");
            return NULL;

    LABEL_HANDLE:
        // apparentlly Bug exist
        for(i = ZoneStart; i <= ZoneEnd; i++){
            z = MMS.ZonesGroup + i;

            if(z -> PageFreeCount < (unsigned)number){
                ColorPrintfk(YELLOW, BLACK, "Can't Alloc Mem In Zone Index %D Which Type Is %d\n", i, ZoneSelector);
                continue;
            }

            tmp = BITS_PER_LONG - BITS_MAP_BIT_OFFSET(z -> ZoneStartAddr);

            for(j = PAGE_2M_INDEX(z->ZoneStartAddr); j < PAGE_2M_INDEX(z->ZoneEndAddr); j += j % BITS_PER_LONG ? BITS_PER_LONG : tmp){

                // ColorPrintfk(YELLOW, BLACK, "%d, %p, %p\n", j, p -> PhyAddr, p);
                p = MMS.PagesGroup + j;
                unsigned long index = BITS_MAP_INDEX(p -> PhyAddr);
                value = MMS.BitsMap[index];

                // HERE To Handle The UnAligned
                if(index < (MMS.BitsMapLength >> 3)){
                    value <<= BITS_PER_LONG - tmp;
                    value +=  (MMS.BitsMap[index + 1] & (~((1UL << tmp) - 1))) >> tmp;
                }

                for(k = 0; k <= BITS_PER_LONG - number; k++){
                    if(!(value & pattern)){
                        for(l = 0; l < (unsigned)number; l++) PageInit(p + k + l, PageAttr);
                        return p + k;
                    }
                    pattern >>= 1;
                }

                pattern = PatternBak;
            }
        }
    }
    ColorPrintfk(YELLOW, BLACK, "Can't Alloc Mem In Zone Type Is %d May Be There Is No More Space \n", ZoneSelector);
    return NULL;
}



void InitMemory(){

    MMS = (struct GlobalMemManager){
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
    for(unsigned int i = 0; i < MMS.GMDLength; i++){

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
    MMS.BitsMapCount = TotalMemory >> PAGE_2M_SHIFT;

    MMS.BitsMapLength = ((TotalMemory >> PAGE_2M_SHIFT) + 7) >> 3;

    memset(MMS.BitsMap, 0xff, MMS.BitsMapLength);

    // Init PageGroup

    MMS.PagesGroup  = (struct Page*)MEM_GAP_ALIGN(((unsigned long)MMS.BitsMap + MMS.BitsMapLength));
    MMS.PagesCount   = TotalMemory >> PAGE_2M_SHIFT;
    MMS.PagesLength = (TotalMemory >> PAGE_2M_SHIFT) * sizeof(struct Page);
    memset(MMS.PagesGroup, 0x00, MMS.PagesLength);

    // Init ZoneGroup

    MMS.ZonesGroup  = (struct Zone*)MEM_GAP_ALIGN((unsigned long)MMS.PagesGroup + MMS.PagesLength);
    MMS.ZonesCount   = 0; // Assume Here
    MMS.ZonesLength = 5 * sizeof(struct Zone);
    memset(MMS.ZonesGroup, 0x00, MMS.ZonesLength);

    for(unsigned int i = 0; i < MMS.GMDLength; i++){
        if(MMS.descriptor[i].type != 1)
        continue;

        StartAddr   = PAGE_2M_ALIGN_UP(MMS.descriptor[i].address);
        EndAddr     = PAGE_2M_ALIGN_DOWN(MMS.descriptor[i].address + MMS.descriptor[i].length);

        if(StartAddr >= EndAddr)
        continue;


        struct Zone * z = MMS.ZonesGroup + MMS.ZonesCount;

        z -> GMM           = &MMS;

        z -> Attribute     = 0;

        z -> PagesGroup    = MMS.PagesGroup + (StartAddr >> PAGE_2M_SHIFT);
        z -> PageFreeCount = (EndAddr - StartAddr) >> PAGE_2M_SHIFT;
        z -> PagesCount    = z -> PageFreeCount;
        z -> PageUsingCount= 0;
        z -> TotalPagesLink= 0;

        z -> ZoneEndAddr   = EndAddr;
        z -> ZoneStartAddr = StartAddr;
        z -> ZoneLength    = EndAddr - StartAddr;

        MMS.ZonesCount++;

        for(unsigned long CurrentAddr = StartAddr; CurrentAddr < EndAddr; CurrentAddr += PAGE_2M_SIZE){
            struct Page * p = MMS.PagesGroup + (CurrentAddr >> PAGE_2M_SHIFT);
            MMS.BitsMap[BITS_MAP_INDEX(CurrentAddr)] ^= BITS_MAP_BIT_PATTERN(CurrentAddr);

            // Init PagesGroup
            p -> age           = 0;
            p -> Attribute     = 0;
            p -> PhyAddr       = CurrentAddr;
            p -> RefCount      = 0;
            p -> ZoneStruct    = z;
        }
    }
    // !!!!!!!!!!!!!!!!!!!!!

    MMS.ZonesLength = MMS.ZonesCount * sizeof(struct Zone);

    ColorPrintfk(BLUE, BLACK, "BitsMap bitsmap in : %p, bitsmap size : %D b, bitsmap length : %D B\n", 
                                                    MMS.BitsMap, MMS.BitsMapCount, MMS.BitsMapLength);

    ColorPrintfk(BLUE, BLACK, "PageGroup pagegroup in : %p, pagegroup size : %D s, pagegroup length : %D B\n",
                                                    MMS.PagesGroup, MMS.PagesCount, MMS.PagesLength);

    ColorPrintfk(BLUE, BLACK, "ZoneGroup zonegroup in : %p, zonegroup size : %D s, zonegroup length : %D B\n",
                                                    MMS.ZonesGroup, MMS.ZonesCount, MMS.ZonesLength);
    
    // Marked Zone Which Is the Unmapped

    ZoneDmaIndex    = 0;
    ZoneNormalIndex = 0;
    ZoneUnmapedIndex= 0;

    for(unsigned long i = 0; i < MMS.ZonesCount; i++){
        struct Zone * z = MMS.ZonesGroup + i;
        // 4GB For Kernel
        if(z -> ZoneStartAddr == 0x100000000){
            ZoneUnmapedIndex = i;
        }
    }

    MMS.EndStruct = (unsigned long)MEM_GAP_ALIGN((MMS.ZonesGroup + MMS.ZonesCount));

    // Init Page Attribute Used In Kernel Init

    Temp =  VIRT_TO_PHY(PAGE_4K_ALIGN_UP(MMS.EndStruct)) >> PAGE_2M_SHIFT;

    for(unsigned int i = 0; i < Temp; i++){
        PageInit(MMS.PagesGroup + i, PATTR(PG_Active) | PATTR(PG_Kernel) | PATTR(PG_Kernel_Init) | PATTR(PG_PTable_Maped));
    }

    // Flush The Consistency Mapping 

    // unsigned long CR3 = GetCr3();

    // ColorPrintfk(BLUE, BLACK, "CR3 : %X ; PML4E : %X ; PDPTE : %X ; \n", CR3, *PHY_TO_VIRT(CR3), \
    //                                                                     *PHY_TO_VIRT(*(PHY_TO_VIRT(CR3)) & (~0xff)));
    // *PHY_TO_VIRT(CR3 & (~0xfffUL)) = 0;

    // FlushTLB();
}

void PageInit(struct Page *p, unsigned long flag){
    if(!p -> Attribute){
        MMS.BitsMap[BITS_MAP_INDEX(p->PhyAddr)] |= BITS_MAP_BIT_PATTERN(p->PhyAddr);
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
        MMS.BitsMap[BITS_MAP_INDEX(p->PhyAddr)] |= BITS_MAP_BIT_PATTERN(p->PhyAddr);
        p -> Attribute |= flag;
    }
}