#include "memory.h"
#include "printk.h"
#include "lib.h"
#include "task.h"
#include "errno.h"

void ListInit(struct List *list);

void ListForeAdd(struct List *new, struct List *list);
void ListBackAdd(struct List *List, struct List *new);
int ListDelete(struct List *list);

struct List* ListNext(struct List *list);

int ListIsEmpty(struct List* list);

struct GlobalMemManager MMS;

extern char _text;
extern char _etext;
extern char _edata;
extern char _end;
extern char _erodata;

unsigned long ZoneDmaIndex;
unsigned long ZoneNormalIndex;
unsigned long ZoneUnmapedIndex;

struct SlabCache KmallocSlabSet[KMALLOC_SLAB_SIZE] = {
  {0x20, 0, 0, NULL, NULL, NULL, NULL},
  {0x40, 0, 0, NULL, NULL, NULL, NULL},
  {0x80, 0, 0, NULL, NULL, NULL, NULL},
  {0x100, 0, 0, NULL, NULL, NULL, NULL},

  {0x200, 0, 0, NULL, NULL, NULL, NULL},
  {0x400, 0, 0, NULL, NULL, NULL, NULL},
  {0x800, 0, 0, NULL, NULL, NULL, NULL},
  {0x1000, 0, 0, NULL, NULL, NULL, NULL},

  {0x2000, 0, 0, NULL, NULL, NULL, NULL},
  {0x4000, 0, 0, NULL, NULL, NULL, NULL},
  {0x8000, 0, 0, NULL, NULL, NULL, NULL},
  {0x10000, 0, 0, NULL, NULL, NULL, NULL},

  {0x20000, 0, 0, NULL, NULL, NULL, NULL},
  {0x40000, 0, 0, NULL, NULL, NULL, NULL},
  {0x80000, 0, 0, NULL, NULL, NULL, NULL},
  {0x100000, 0, 0, NULL, NULL, NULL, NULL},
};


struct Slab* CreatSlab(unsigned long size, int ZoneSelector){
    struct Slab* slab = (struct Slab*)kmalloc(sizeof(struct Slab), 0);

    if(!slab) return NULL;

    memset(slab, 0, sizeof(struct Slab));

    ListInit(&(slab->list));
    slab->FreeCount     = PAGE_2M_SIZE / size;
    slab->ColorLength   = BITS_MAP_LENGTH(slab->FreeCount);
    slab->ColorMap      = (unsigned long*)kmalloc(sizeof(unsigned long) * slab->ColorLength, 0);

    if(!(slab->ColorMap)) {kfree(slab); return NULL;}

    memset(slab->ColorMap, 0, sizeof(unsigned long) * slab->ColorLength);

    slab->page          = AllocPage(ZoneSelector, 1, PATTR(PG_KERNEL));

    if(!(slab->page)) {kfree(slab -> ColorMap); kfree(slab); return NULL;}

    slab->Vaddress      = PHY_TO_VIRT(slab->page->PhyAddr);

    return slab;
}

struct SlabCache* CreateSlabCache(  unsigned long SlabSize, void *(*Constructor)(void *Vaddr, unsigned long arg), 
                                    void *(*Destructor)(void *Vaddr, unsigned long arg), unsigned long arg){
    struct SlabCache *SC= (struct SlabCache *)kmalloc(sizeof(struct SlabCache), 0);
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
        ListDelete(&slab->list);
        kfree(slab->ColorMap);
        CleanPage(slab->page);
        FreePage(slab->page, 1);
        tmp_slab = slab;
        slab = ContainerOf(ListNext(&slab->list), struct Slab, list);
        kfree(tmp_slab);
    }
    ListDelete(&slab->list);
    kfree(slab->ColorMap);
    CleanPage(slab->page);
    FreePage(slab->page, 1);
    kfree(slab);
    kfree(SC);
    return 1;
}

void * AllocSlab(struct SlabCache *SC, unsigned long arg){

    struct Slab *slab;
    if(SC->TotalFree == 0){
        slab = CreatSlab(SC->size, ZONE_NORMAL_INDEX);
        if(slab == NULL) {ColorPrintfk(RED, BLACK, "CreatSlab Fail In AllocSlab\n"); return NULL;}
        ListBackAdd(&SC->CachePool->list, &slab->list);
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
                if((*(slab->ColorMap + (i >> 6)) & (1UL << i % 64)) == 0){
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

    ColorPrintfk(RED, BLACK, "There Is No Slap Could Be Alloc\n");
    return NULL;
}

int FreeSlab(struct SlabCache *SC, void *Vaddress, unsigned long arg){

    struct Slab *slab;

    slab = SC->CachePool;
    unsigned long index = 0;

    do{
        if(slab->Vaddress <= Vaddress && ((unsigned long)(Vaddress - slab->Vaddress) < PAGE_2M_SIZE)){
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
                ListDelete(&slab->list);
                kfree(slab->ColorMap);
                CleanPage(slab->page);
                FreePage(slab->page, 1);
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

unsigned long SlabCacheInit(){
    unsigned long TempAddr = VIRT_TO_PHY(MMS.EndStruct);
    unsigned long StartAddr = MEM_GAP_ALIGN(MMS.EndStruct);
    
    for(int i = 0; i < KMALLOC_SLAB_SIZE; i++){
        struct Slab *CachePool  = (struct Slab*)StartAddr;
        CachePool->ColorCount   = PAGE_2M_SIZE / KmallocSlabSet[i].size;
        CachePool->UsingCount   = 0;
        CachePool->FreeCount    = CachePool->ColorCount;
        ListInit(&CachePool->list);
        CachePool->ColorLength  = BITS_MAP_LENGTH(CachePool->ColorCount);
        CachePool->ColorMap     = (unsigned long*)(StartAddr + sizeof(struct Slab));
        memset(CachePool->ColorMap, 0, CachePool->ColorLength * sizeof(unsigned long));

        MMS.EndStruct = (unsigned long)CachePool->ColorMap + (CachePool->ColorLength * sizeof(unsigned long));
        StartAddr = MEM_GAP_ALIGN(MMS.EndStruct);

        KmallocSlabSet[i].CachePool = CachePool;
        KmallocSlabSet[i].TotalFree = CachePool->ColorCount;
    }

    // ColorPrintfk(   BLUE, BLACK, "MMS.BitsMap:%X, ZoneStruct->FreeCount:%X, ZoneStruct->UsingCount:%X \n", 
    //                 MMS.BitsMap[0], MMS.ZonesGroup->PageFreeCount, MMS.ZonesGroup->PageUsingCount);

    for(unsigned long i = PAGE_2M_INDEX(TempAddr); i < PAGE_2M_INDEX(VIRT_TO_PHY(MMS.EndStruct)); i++){
        struct Page* TmpPage = (struct Page*)(MMS.PagesGroup + i);
        TmpPage->ZoneStruct->PageFreeCount--;
        TmpPage->ZoneStruct->PageUsingCount++;
        MMS.BitsMap[BITS_MAP_INDEX(TmpPage->PhyAddr)] |= BITS_MAP_BIT_PATTERN(TmpPage->PhyAddr);
        InitPage(TmpPage, PATTR(PG_KERNEL) | PATTR(PG_KERNEL_INIT) | PATTR(PG_PTABLE_MAPPED));
    }

    // ColorPrintfk(   BLUE, BLACK, "MMS.BitsMap:%X, ZoneStruct->FreeCount:%X, ZoneStruct->UsingCount:%X \n", 
    //                 MMS.BitsMap[0], MMS.ZonesGroup->PageFreeCount, MMS.ZonesGroup->PageUsingCount);

    for(int i = 0; i < KMALLOC_SLAB_SIZE; i++){
        unsigned long virtual   = PAGE_2M_ALIGN_UP(MMS.EndStruct + PAGE_2M_SIZE * i);
        unsigned long Physical  = VIRT_TO_PHY(virtual);
        struct Page *page = (struct Page*)(MMS.PagesGroup + PAGE_2M_INDEX(Physical));
        MMS.BitsMap[BITS_MAP_INDEX(page->PhyAddr)] |= BITS_MAP_BIT_PATTERN(page->PhyAddr);
        InitPage(page, PATTR(PG_PTABLE_MAPPED) | PATTR(PG_KERNEL_INIT)| PATTR(PG_KERNEL));
        page->ZoneStruct->PageUsingCount++;
        page->ZoneStruct->PageFreeCount--;
        KmallocSlabSet[i].CachePool->page = page;
        KmallocSlabSet[i].CachePool->Vaddress = (void *)virtual;
    }

    // ColorPrintfk(   BLUE, BLACK, "MMS.BitsMap:%X, ZoneStruct->FreeCount:%X, ZoneStruct->UsingCount:%X \n", 
    //                 MMS.BitsMap[0], MMS.ZonesGroup->PageFreeCount, MMS.ZonesGroup->PageUsingCount);
    
    return 1;
}
/// @brief ugly but useful
/// @param ZoneSelector An Enum Type Defined In memory.h To Control The Type Of Memory
/// @param number   0 < Number < 64
/// @param PageAttr An Enum Type Defined In memory.h To Describe Page 
/// @return Alloced Memory Start
struct Page* AllocPage(int ZoneSelector, int number, unsigned long PageAttr){
    
    if(number <= 0 || number > 64) return NULL;

    struct Zone *z;
    struct Page *p;
    unsigned long value;
    unsigned long ZoneStart, ZoneEnd;
    unsigned long pattern = number == 64 ? 0xffffffffffffffff : ~((1UL << (BITS_PER_LONG - number)) - 1);
    unsigned long PatternBak = pattern;
    unsigned long DefaultAttr;
    unsigned long i, j, k, l, tmp;

    switch (ZoneSelector){
        case ZONE_DMA_INDEX:
            ZoneStart   = 0;
            ZoneEnd     = ZoneDmaIndex; 
            DefaultAttr = PATTR(PG_PTABLE_MAPPED); 
            goto LABEL_HANDLE;

        case ZONE_NORMAL_INDEX:
            ZoneStart   = ZoneDmaIndex;
            ZoneEnd     = ZoneNormalIndex;
            DefaultAttr = PATTR(PG_PTABLE_MAPPED);
            goto LABEL_HANDLE;

        case ZONE_UNMAPED_INDEX:
            ZoneStart   = ZoneUnmapedIndex;
            ZoneEnd     = MMS.ZonesCount - 1;
            DefaultAttr = 0;
            goto LABEL_HANDLE;
        
        default:
            ColorPrintfk(YELLOW, BLACK, "Unknown ZoneSelector\n");
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
                        for(l = 0; l < (unsigned)number; l++){
                            struct Page *TmpPage = (p + k + l);
                            MMS.BitsMap[BITS_MAP_INDEX(TmpPage->PhyAddr)] |= BITS_MAP_BIT_PATTERN(TmpPage->PhyAddr);
                            TmpPage->ZoneStruct->PageFreeCount--;
                            TmpPage->ZoneStruct->PageUsingCount++;;
                            InitPage(TmpPage, PageAttr | DefaultAttr);
                        }
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

int FreePage(struct Page* page, int number){
    
    if(number <= 0 || number > 64) return 0;

    unsigned long index = PAGE_2M_INDEX(VIRT_TO_PHY(MMS.EndStruct));
    if(page <= MMS.PagesGroup + index) {ColorPrintfk(RED, BLACK, "Can`t Free Fixed Page"); return 0;}

    if(page == NULL) {ColorPrintfk(RED, BLACK, "page == NULL In FreePage()\n"); return 0;}

    struct Page *tmp = page;
    for(int i = 0; i < number; i++){
        tmp = page + i;
        MMS.BitsMap[BITS_MAP_INDEX(tmp->PhyAddr)] ^= BITS_MAP_BIT_PATTERN(tmp->PhyAddr);

        tmp->ZoneStruct->PageFreeCount++;
        tmp->ZoneStruct->PageUsingCount--;
        tmp->Attribute = 0;
    }

    ColorPrintfk(   BLUE, BLACK, "MMS.BitsMap:%X, ZoneStruct->FreeCount:%X, ZoneStruct->UsingCount:%X \n", 
                    MMS.BitsMap[0], MMS.ZonesGroup->PageFreeCount, MMS.ZonesGroup->PageUsingCount);
    return 1;
}


void* kmalloc(unsigned long size, unsigned long flags){
    if(size > 0x100000){ ColorPrintfk(RED, BLACK, "Size > 0x100000 No Slap For It\n"); return NULL;}

    for(int i = 0; i < KMALLOC_SLAB_SIZE; i++){

        if(KmallocSlabSet[i].size < size) continue;

        if(KmallocSlabSet[i].TotalFree == 0){
            struct Page* page = AllocPage(ZONE_NORMAL_INDEX, 1, 0);
            struct SlabCache* SC = &KmallocSlabSet[i];
            if(page == NULL){ColorPrintfk(RED, BLACK, "Alloc Page failed in kmalloc\n"); return NULL;}

            unsigned long ksize = KmallocSlabSet[i].size;
            switch (ksize){
                case 0x20:
                case 0x40:
                case 0x80:
                case 0x100:
                case 0x200:{
                    unsigned long virtual = (unsigned long)PHY_TO_VIRT(page->PhyAddr);
                    unsigned int StructSize = sizeof(struct Slab) + (PAGE_2M_SIZE / ksize / sizeof(unsigned long));
                    struct Slab* slab = (struct Slab*)(virtual + PAGE_2M_SIZE - StructSize);
                    slab->UsingCount    = 0;
                    slab->FreeCount     = (PAGE_2M_SIZE - StructSize + ksize - 1) / ksize;
                    slab->ColorMap      = (unsigned long *)((unsigned long)slab + sizeof(struct Slab));
                    slab->ColorCount    = slab->FreeCount;
                    slab->ColorLength   = BITS_MAP_LENGTH(slab->ColorCount);
                    memset(slab->ColorMap, 0xff, slab->ColorLength * sizeof(long));
                    for(unsigned long i = 0; i < slab->ColorCount; i++){
                        *(slab->ColorMap + (i >> 6)) ^= (1UL << (i % 64)); 
                    }
                    ListInit(&slab->list);
                    slab->page = page;
                    slab->Vaddress = (void *)virtual;
                    ListBackAdd(&KmallocSlabSet[i].CachePool->list, &slab->list);
                    SC->TotalFree       += slab->FreeCount;
                    
                    break;
                }
                case 0x400:
                case 0x800:
                case 0x1000:
                case 0x2000:
                case 0x4000:
                case 0x8000:
                case 0x10000:
                case 0x20000:
                case 0x40000:
                case 0x80000:
                case 0x100000:{
                    struct Slab* slab = (struct Slab*)kmalloc(sizeof(struct Slab), 0);
                    unsigned long virtual = (unsigned long)PHY_TO_VIRT(page->PhyAddr);
                    slab->Vaddress = (void *)virtual;
                    slab->ColorCount = PAGE_2M_SIZE / ksize;
                    slab->ColorLength = BITS_MAP_LENGTH(slab->ColorCount);
                    slab->ColorMap = (unsigned long*)(virtual);
                    slab->FreeCount = slab->ColorCount;
                    memset(slab->ColorMap, 0, slab->ColorLength * sizeof(unsigned long));
                    ListInit(&slab->list);
                    slab->page = page;
                    slab->UsingCount = 0;
                    slab->Vaddress = (void *)virtual;
                    ListBackAdd(&KmallocSlabSet[i].CachePool->list, &slab->list);
                    SC->TotalFree       += slab->FreeCount;
                    break;
                }
                
                default:
                    FreePage(page, 1);
                    ColorPrintfk(RED, BLACK, "In kmalloc illegal ksize\n");
                    return NULL;
            }
        }
        return AllocSlab(&KmallocSlabSet[i], 0);
    }
    ColorPrintfk(RED, BLACK, "Can`t Alloc Memory In kmalloc()\n");
    return 0;
}
unsigned int kfree(void *ptr){
    unsigned long PageAddr = PAGE_2M_ALIGN_DOWN((unsigned long)ptr);
    for(int i = 0; i < KMALLOC_SLAB_SIZE; i++){
        struct SlabCache* SC = &KmallocSlabSet[i];
        if(SC->TotalUse == 0) continue;

        struct Slab *slab = SC->CachePool;
        do{
            if(slab->UsingCount == 0){
                slab = ContainerOf(ListNext(&slab->list), struct Slab, list);
                continue;
            };

            if((unsigned long)slab->Vaddress == PageAddr){
                unsigned long offset = ((unsigned long)ptr - PageAddr) / SC->size;
                slab->ColorMap[offset >> 6] ^= (1UL << offset % 64);
                slab->UsingCount--;
                slab->FreeCount++;
                SC->TotalFree++;
                SC->TotalUse--;

                if((slab->UsingCount != 0) || (SC->TotalFree < ((slab->FreeCount) * 3 / 2))) return 1;

                if(slab == SC->CachePool) return 1;

                unsigned long ksize = SC->size;

                switch (ksize){
                    case 0x20:
                    case 0x40:
                    case 0x80:
                    case 0x100:
                    case 0x200:{
                        SC->TotalUse -= slab->UsingCount;
                        SC->TotalFree -= slab->FreeCount;
                        ListDelete(&slab->list);
                        CleanPage(slab->page);
                        FreePage(slab->page, 1);
                        break;
                    }
                    default:{
                        SC->TotalFree -= slab->FreeCount;
                        ListDelete(&slab->list);
                        CleanPage(slab->page);
                        FreePage(slab->page, 1);
                        kfree(slab);
                        break;
                    }
                }
                return 1;
            }
            slab = ContainerOf(ListNext(&slab->list), struct Slab, list);
        }while(SC->CachePool != slab);
    }
    ColorPrintfk(RED, BLACK, "Can`t Free Memory In kfree\n");
    return 0;
}

unsigned long do_brk(unsigned long start, unsigned long size){

    unsigned long * tmp     = NULL;
    unsigned long * virtual = NULL;
    struct Page *p          = NULL;
    unsigned long i                   = 0;

    for(i = start; i < start + size; i += PAGE_2M_SIZE){
        tmp = PHY_TO_VIRT((unsigned long)CURRENT->lmm->pgd & (~0xfffUL) + GetBits(i, PAGE_GDT_SHIFT, 9));
        if(!(*tmp)){
            virtual = kmalloc(PAGE_4K_SIZE, 0);
            memset(virtual, 0, PAGE_4K_SIZE);
            SetPML4E(tmp, VIRT_TO_PHY(virtual), PEA_USER_TABLE);
        }

        tmp = PHY_TO_VIRT((unsigned long)tmp & (~0xfffUL) + GetBits(i, PAGE_1G_SHIFT, 9));
        if(!(*tmp)){
            virtual = kmalloc(PAGE_4K_SIZE, 0);
            memset(virtual, 0, PAGE_4K_SIZE);
            SetPDPTE(tmp, VIRT_TO_PHY(virtual), PEA_USER_TABLE);
        } 
        tmp = PHY_TO_VIRT((unsigned long)tmp & (~0xfffUL) + GetBits(i, PAGE_2M_SHIFT, 9));
        if(!(*tmp)){
            p = AllocPage(ZONE_NORMAL_INDEX, 1, PG_PTABLE_MAPPED);
            if(p == NULL) return -ENOMEM;
            SetPDE(tmp, p->PhyAddr, PEA_USER_ENTRY);
        } 
    }

    CURRENT->lmm->EndBrk = i;
    FlushTLB();
    return i;
}


void InitMemory(){

    MMS = (struct GlobalMemManager){
        .descriptor = {0},
        .GMDLength  =  0
    };

    MMS.StartCode   = (unsigned long)(&_text);
    MMS.EndCode     = (unsigned long)(&_etext);
    MMS.EndData     = (unsigned long)(&_edata);
    MMS.EndRoData   = (unsigned long)(&_erodata);
    MMS.StartBrk      = (unsigned long)(&_end);
    

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

    MMS.BitsMap = (unsigned long *)MEM_GAP_ALIGN(MMS.StartBrk);
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
        // 4GB For Kernel In Zone Normal
        if(z -> ZoneStartAddr == 0x100000000 && !ZoneUnmapedIndex){
            ZoneUnmapedIndex = i;
        }
    }

    MMS.EndStruct = (unsigned long)MEM_GAP_ALIGN((MMS.ZonesGroup + MMS.ZonesCount));

    // Init Page Attribute Used In Kernel Init

    Temp =  VIRT_TO_PHY(MMS.EndStruct) >> PAGE_2M_SHIFT;

    for(unsigned int i = 0; i < Temp; i++){
        struct Page *TmpPage = (struct Page*)(MMS.PagesGroup + i);
        TmpPage->ZoneStruct->PageFreeCount--;
        TmpPage->ZoneStruct->PageUsingCount++;
        MMS.BitsMap[BITS_MAP_INDEX(TmpPage->PhyAddr)] |= BITS_MAP_BIT_PATTERN(TmpPage->PhyAddr);
        InitPage(TmpPage, PATTR(PG_KERNEL) | PATTR(PG_KERNEL_INIT) | PATTR(PG_PTABLE_MAPPED));
    }

    // Flush The Consistency Mapping 

    // unsigned long CR3 = GetCr3();
 
    // ColorPrintfk(BLUE, BLACK, "CR3 : %X ; PML4E : %X ; PDPTE : %X ; \n", CR3, *PHY_TO_VIRT(CR3), \
    //                                                                     *PHY_TO_VIRT(*(PHY_TO_VIRT(CR3)) & (~0xff)));
    // *PHY_TO_VIRT(CR3 & (~0xfffUL)) = 0;

    // FlushTLB();
}

void InitPageTable(){
    unsigned long Cr3 = GetCr3();
    unsigned long* tmp;
    for(int i = 0; i < MMS.ZonesCount; i++){
        if(ZoneUnmapedIndex == (ZoneUnmapedIndex & i)) break;

        struct Zone* zone = MMS.ZonesGroup + i;

        for(unsigned long j = 0; j < zone->PagesCount; j++){
            struct Page* page       = zone->PagesGroup + i;
            tmp =   (unsigned long *)((unsigned long)PHY_TO_VIRT(Cr3 & (~0xfff))) + 
                    GetBits((unsigned long)PHY_TO_VIRT(page->PhyAddr), PAGE_GDT_SHIFT, 9);

            if(*tmp == 0){
                void *virtual = kmalloc(PAGE_4K_SIZE, 0);
                SetPML4E(tmp, VIRT_TO_PHY(virtual), PEA_USER_TABLE);
            }
            tmp =   (unsigned long*)((unsigned long)PHY_TO_VIRT(*tmp & (~0xfff))) + 
                    GetBits((unsigned long)PHY_TO_VIRT(page->PhyAddr), PAGE_1G_SHIFT, 9);
            
            if(*tmp == 0){
                void *virtual = kmalloc(PAGE_4K_SIZE, 0);
                SetPDPTE(tmp, VIRT_TO_PHY(virtual), PEA_USER_TABLE);
            }

            tmp =   (unsigned long*)((unsigned long)PHY_TO_VIRT(*tmp & (~0xfff))) + 
                    GetBits((unsigned long)PHY_TO_VIRT(page->PhyAddr), PAGE_2M_SHIFT, 9);
            SetPDE(tmp, page->PhyAddr, PEA_USER_ENTRY);
        }
    }

    FlushTLB();
}

void InitPage(struct Page *p, unsigned long flag){
    p -> Attribute |= flag;

    if(!p->RefCount || (p->Attribute & PG_SHARED)){
        p -> RefCount++;
        p -> ZoneStruct->TotalPagesLink++;
    }
}

void CleanPage(struct Page *p){
    p->RefCount--;
    p->ZoneStruct->TotalPagesLink--;

    if(!p->RefCount){
        p->Attribute &= PATTR(PG_PTABLE_MAPPED);
    }
}

unsigned long GetPageAttr(struct Page* p){
    if(p == NULL){
        ColorPrintfk(RED, BLACK, "p is NULL In GetPageAttr()\n");
        return 0;
    }else{
        return p->Attribute;
    }
}

unsigned long SetPageAttr(struct Page* p, unsigned long flags){
    if(p == NULL){
        ColorPrintfk(RED, BLACK, "p is NULL In SetPageAttr()\n");
        return 0;
    }else{
        p->Attribute = flags;
        return 1;
    }
}