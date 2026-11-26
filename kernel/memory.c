#include "memory.h"
#include "printk.h"
#include "lib.h"
#include "task.h"
#include "errno.h"
#include "vma.h"
#include "mmap.h"

void ListInit(struct List *list);

void ListForeAdd(struct List *new, struct List *list);
void ListBackAdd(struct List *List, struct List *new);
int ListDelete(struct List *list);

struct List* ListNext(struct List *list);

int ListIsEmpty(struct List* list);
struct List* list_search(const struct List *head, const struct List *tar);



struct GlobalMemManager MMS;

extern char _text;
extern char _etext;
extern char _edata;
extern char _end;
extern char _erodata;

unsigned long ZoneDmaIndex;
unsigned long ZoneNormalIndex;
unsigned long ZoneUnmapedIndex;

static void reserve_phys_range(unsigned long start, unsigned long end,
                               unsigned long attr);

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
    if(!size || size > PAGE_2M_SIZE) return NULL;
    struct Slab* slab = (struct Slab*)kmalloc(sizeof(struct Slab), 0);

    if(!slab) return NULL;

    memset(slab, 0, sizeof(struct Slab));

    ListInit(&(slab->list));
    slab->ColorCount = slab->FreeCount = PAGE_2M_SIZE / size;
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
    if(!SlabSize || SlabSize > PAGE_2M_SIZE) return NULL;
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
    struct Slab *head = SC->CachePool;
    while(!ListIsEmpty(&head->list)){
        struct Slab *slab = ContainerOf(head->list.next, struct Slab, list);
        ListDelete(&slab->list);
        kfree(slab->ColorMap);
        FreePage(slab->page, 1);
        kfree(slab);
    }
    kfree(head->ColorMap);
    FreePage(head->page, 1);
    kfree(head);
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
            if((unsigned long)(Vaddress - slab->Vaddress) % SC->size || index >= slab->ColorCount ||
               !(slab->ColorMap[index >> 6] & (1UL << (index % 64)))) return 0;
            slab->ColorMap[index >> 6] &= ~(1UL << (index % 64));
            slab->FreeCount++;
            slab->UsingCount--;

            SC->TotalFree++;
            SC->TotalUse--;

            void *Vaddr = (void *)((unsigned long)slab->Vaddress + (index * SC->size));
            if(SC->Destructor != NULL) SC->Destructor(Vaddr, arg);

            if(slab != SC->CachePool && !slab->UsingCount && SC->TotalFree >= slab->ColorCount * 3 / 2){
                SC->TotalFree -= slab->ColorCount;
                ListDelete(&slab->list);
                kfree(slab->ColorMap);
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

    /* TempAddr is already a physical address. */
    reserve_phys_range(TempAddr, VIRT_TO_PHY(MMS.EndStruct),
                       PATTR(PG_KERNEL) | PATTR(PG_KERNEL_INIT) |
                       PATTR(PG_PTABLE_MAPPED));

    // ColorPrintfk(   BLUE, BLACK, "MMS.BitsMap:%X, ZoneStruct->FreeCount:%X, ZoneStruct->UsingCount:%X \n", 
    //                 MMS.BitsMap[0], MMS.ZonesGroup->PageFreeCount, MMS.ZonesGroup->PageUsingCount);

    for(int i = 0; i < KMALLOC_SLAB_SIZE; i++){
        unsigned long virtual   = PAGE_2M_ALIGN_UP(MMS.EndStruct + PAGE_2M_SIZE * i);
        unsigned long Physical  = VIRT_TO_PHY(virtual);
        struct Page *page = (struct Page*)(MMS.PagesGroup + PAGE_4K_INDEX(Physical));
        reserve_phys_range(Physical, Physical + PAGE_2M_SIZE,
                           PATTR(PG_PTABLE_MAPPED) | PATTR(PG_KERNEL_INIT) |
                           PATTR(PG_KERNEL));
        KmallocSlabSet[i].CachePool->page = page;
        KmallocSlabSet[i].CachePool->Vaddress = (void *)virtual;
    }

    // ColorPrintfk(   BLUE, BLACK, "MMS.BitsMap:%X, ZoneStruct->FreeCount:%X, ZoneStruct->UsingCount:%X \n", 
    //                 MMS.BitsMap[0], MMS.ZonesGroup->PageFreeCount, MMS.ZonesGroup->PageUsingCount);
    
    return 1;
}

static void reserve_phys_range(unsigned long start, unsigned long end,
                               unsigned long attr){
    unsigned long first = PAGE_4K_INDEX(PAGE_4K_ALIGN_DOWN(start));
    unsigned long last  = PAGE_4K_INDEX(PAGE_4K_ALIGN_UP(end));

    if(last > MMS.PagesCount) last = MMS.PagesCount;
    for(unsigned long pfn = first; pfn < last; pfn++){
        struct Page *page = MMS.PagesGroup + pfn;
        if(page->ZoneStruct != NULL && !page->reserved){
            page->reserved = 1;
            page->ZoneStruct->PageFreeCount--;
            page->ZoneStruct->PageUsingCount++;
            page->Attribute |= attr;
            if(page->RefCount == 0){
                page->RefCount = 1;
                page->ZoneStruct->TotalPagesLink++;
            }
        }
    }
}


struct Page *__alloc_pages_slowpath(){
    ColorPrintfk(RED, BLACK, "slow path is not implemented");
    return NULL;
}
struct Page *get_page_from_freelist(struct Zone *zone, unsigned int order, int alloc_flags){

        for(int i = order; i < MAX_ORDER; i++){
            if(zone->free_area[i].nr_free == 0) continue;

            // split buddy
            if(i != order){
                int ob = i;
                while(ob > order){
                    struct List * bigger_block = zone->free_area[ob].free_list.next;
                    struct Page * block_head = ContainerOf(bigger_block, struct Page, buddy_list);
                    
                    ListDelete(zone->free_area[ob].free_list.next);
                    zone->free_area[ob].nr_free--;
                    
                    ListBackAdd(&zone->free_area[ob - 1].free_list, &(block_head + (1 << (ob - 1)))->buddy_list);
                    ListBackAdd(&zone->free_area[ob - 1].free_list, &block_head->buddy_list);

                    zone->free_area[ob - 1].nr_free += 2;

                    ob--;
                }
            }
            struct Page * page =  ContainerOf(zone->free_area[order].free_list.next, struct Page, buddy_list);

            ListDelete(&page->buddy_list);
            zone->free_area[order].nr_free--;

            if(page->ZoneStruct){
                page->ZoneStruct->PageFreeCount  -= (1 << order);
                page->ZoneStruct->PageUsingCount += (1 << order);
                page->ZoneStruct->TotalPagesLink += (1 << order);

                for(int j = 0; j < (1 << order); j++){
                    (page + j)->allocation_order = j ? -1 : (int)order;
                    (page + j)->RefCount++;
                    (page + j)->Attribute |= alloc_flags;
                }
                return page;

            }
        }

        return NULL;
}
 
struct Page *__alloc_pages(struct Zone *zone, unsigned int order, int alloc_flags){

    struct Page *page = get_page_from_freelist(zone, order, alloc_flags);

    if(!page) page = __alloc_pages_slowpath();

    return page;
}

void __free_pages_core(struct Page *page, int order){

    // this page should be aligned with (1 << order) block_head
    struct Zone *zone = page->ZoneStruct;
    unsigned long page_index = PAGE_4K_INDEX(page->PhyAddr);

    while(order < MAX_ORDER - 1){
        unsigned long buddy_index = page_index ^ (1 << order);
        if( buddy_index < PAGE_4K_INDEX(zone->ZoneStartAddr) ||
            buddy_index >= PAGE_4K_INDEX(zone->ZoneEndAddr)){
            break;
        }

        struct Page * buddy_head = MMS.PagesGroup + buddy_index;
        if(!list_search(&zone->free_area[order].free_list, &buddy_head->buddy_list)){
            break;
        }
        ListDelete(&buddy_head->buddy_list);
        zone->free_area[order].nr_free--;

        if(buddy_index < page_index) page_index = buddy_index;
        order++;
    }

    // this is a safe access
    ListBackAdd(&zone->free_area[order].free_list, &(MMS.PagesGroup + page_index)->buddy_list);

    zone->free_area[order].nr_free++;
}

void __free_pages(struct Page *page, int order){

    struct Zone *zone = page->ZoneStruct;

    __free_pages_core(page, order);

    zone->PageFreeCount  += (1 << order);
    zone->PageUsingCount -= (1 << order);
    zone->TotalPagesLink -= (1 << order);

    for(int j = 0; j < (1 << order); j++){
        (page + j)->allocation_order = -1;
        (page + j)->RefCount--;
        (page + j)->Attribute = 0;
    }
}

unsigned long is_this_zone(unsigned long zone_index, int zone_selector){
    switch(zone_selector){
        // lower 16MiB
        case ZONE_DMA_INDEX:
            return zone_index == ZoneDmaIndex;

        // lower 4GiB
        case ZONE_NORMAL_INDEX:
            return zone_index == ZoneNormalIndex;

        // upper 4GiB
        case ZONE_UNMAPED_INDEX:
            return zone_index == ZoneUnmapedIndex;
    }

    return 0;
}

static unsigned int get_order(unsigned int number){
    unsigned int order = 0;

    while(order < MAX_ORDER){
        if(number <= (1 << order)) break;
        order++;
    }

    return order;
}

static SpinLock_T buddy_lock, slab_lock;

struct Page *alloc_pages(int zone_selector, unsigned int number, unsigned long PageAttr){
    unsigned long attr = PageAttr;
    if(zone_selector != ZONE_UNMAPED_INDEX) attr |= PATTR(PG_PTABLE_MAPPED);

    unsigned int order = get_order(number);

    if(!number || order >= MAX_ORDER) return NULL;

    for(int i = 0; i < MMS.ZonesCount; i++){
        struct Zone * zone = MMS.ZonesGroup + i;
        if(!is_this_zone(i, zone_selector)){
            continue;
        }

        unsigned long irq = spin_lock_irqsave(&buddy_lock);
        struct Page * page = __alloc_pages(zone, order, attr);
        spin_unlock_irqrestore(&buddy_lock, irq);

        if(page) return page;
    }

    return NULL;
}

int free_pages(struct Page *page, int number){
    unsigned long order = get_order(number);

    if(!page || number <= 0 || order >= MAX_ORDER) return 0;

    unsigned long irq = spin_lock_irqsave(&buddy_lock);
    if(!page->ZoneStruct || page->reserved || page->RefCount != 1 ||
       page->allocation_order != (int)order){
        spin_unlock_irqrestore(&buddy_lock, irq);
        return 0;
    }
    __free_pages(page, order);
    spin_unlock_irqrestore(&buddy_lock, irq);

    return 1;
}

/* AllocPage keeps its historical meaning: number of contiguous 2 MiB blocks. */
struct Page *AllocPage(int ZoneSelector, int number, unsigned long PageAttr){
    if(number <= 0 || number > (1 << (MAX_ORDER - 1)) / 512) return NULL;
    return alloc_pages(ZoneSelector, number * 512, PageAttr);
}

struct Page *AllocPage4K(int ZoneSelector, int number, unsigned long PageAttr){
    return alloc_pages(ZoneSelector, number, PageAttr);
}

int FreePage(struct Page *page, int number){
    if(number <= 0 || number > (1 << (MAX_ORDER - 1)) / 512) return 0;
    return free_pages(page, number * 512);
}

int FreePage4K(struct Page *page, int number){
    return free_pages(page, number);
}


static unsigned int kfree_locked(void *ptr);

static void *kmalloc_locked(unsigned long size, unsigned long flags){
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
                    slab->FreeCount     = (PAGE_2M_SIZE - StructSize) / ksize;
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
                    struct Slab* slab = (struct Slab*)kmalloc_locked(sizeof(struct Slab), 0);
                    if(!slab){ FreePage(page, 1); return NULL; }
                    unsigned long virtual = (unsigned long)PHY_TO_VIRT(page->PhyAddr);
                    slab->Vaddress = (void *)virtual;
                    slab->ColorCount = PAGE_2M_SIZE / ksize;
                    slab->ColorLength = BITS_MAP_LENGTH(slab->ColorCount);
                    slab->ColorMap = kmalloc_locked(slab->ColorLength * sizeof(unsigned long), 0);
                    if(!slab->ColorMap){ kfree_locked(slab); FreePage(page, 1); return NULL; }
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
static unsigned int kfree_locked(void *ptr){
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
                unsigned long displacement = (unsigned long)ptr - PageAddr;
                unsigned long offset = displacement / SC->size;
                if(displacement % SC->size || offset >= slab->ColorCount ||
                   !(slab->ColorMap[offset >> 6] & (1UL << (offset % 64)))) return 0;
                slab->ColorMap[offset >> 6] &= ~(1UL << (offset % 64));
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
                        FreePage(slab->page, 1);
                        break;
                    }
                    default:{
                        SC->TotalFree -= slab->FreeCount;
                        ListDelete(&slab->list);
                        FreePage(slab->page, 1);
                        kfree_locked(slab->ColorMap);
                        kfree_locked(slab);
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

void *kmalloc(unsigned long size, unsigned long flags){
    unsigned long irq = spin_lock_irqsave(&slab_lock);
    void *ptr = kmalloc_locked(size, flags);
    spin_unlock_irqrestore(&slab_lock, irq);
    return ptr;
}

unsigned int kfree(void *ptr){
    if(!ptr) return 0;
    unsigned long irq = spin_lock_irqsave(&slab_lock);
    unsigned int freed = kfree_locked(ptr);
    spin_unlock_irqrestore(&slab_lock, irq);
    return freed;
}

static unsigned long *table_from_entry(unsigned long entry){
    return PHY_TO_VIRT(entry & PAGE_4K_MASK);
}

/* The allocator owns one reference initially. VM references may also be
 * held by a page-table entry or a resident backing object. */
void page_get(struct Page *page){
    unsigned long irq = spin_lock_irqsave(&buddy_lock);
    page->RefCount++;
    page->ZoneStruct->TotalPagesLink++;
    spin_unlock_irqrestore(&buddy_lock, irq);
}

void page_put(struct Page *page){
    unsigned long irq = spin_lock_irqsave(&buddy_lock);
    if(page->RefCount == 1) __free_pages(page, 0);
    else{
        page->RefCount--;
        page->ZoneStruct->TotalPagesLink--;
    }
    spin_unlock_irqrestore(&buddy_lock, irq);
}

void init_buddy_system(){
    
    for(unsigned int i = 0; i < MMS.ZonesCount; i++){

        struct Zone * z = MMS.ZonesGroup + i;

        for(int j = 0; j < MAX_ORDER; j++){
            z -> free_area[j].nr_free = 0;
            ListInit(&z -> free_area[j].free_list);
        }

        int bp = 0;
        while (bp < z->PagesCount){

            if((z->PagesGroup + bp)->reserved){
                bp++;
                continue;
            }
            
            unsigned long run_start = bp;

            while(bp < z->PagesCount && !(z->PagesGroup + bp)->reserved) {
                bp++;
            }

            unsigned long run_end = bp;

            unsigned long cursor = run_start;

            while(cursor < run_end){

                int order   = MAX_ORDER - 1;
                int remains = run_end - cursor;
                unsigned long abs_page_index = PAGE_4K_INDEX(z -> ZoneStartAddr) + cursor;
                
                while(order > 0){
                    unsigned long block_pages = 1UL << order;
                    if((1 << order) <= remains && ((abs_page_index & (block_pages - 1)) == 0)){
                        break;
                    }
                    order--;
                }

                __free_pages_core(z->PagesGroup + cursor, order);

                cursor += (1 << (order));

            }
        }
    }
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
    unsigned long TotalPage = 0;

    // Count Total Page
    for(unsigned int i = 0; i < MMS.GMDLength; i++){

        if(MMS.descriptor[i].type != 1) continue;

        struct E820 *CurrentMD  =  MMS.descriptor + i; 

        StartAddr   = PAGE_4K_ALIGN_UP(CurrentMD->address);
        EndAddr     = PAGE_4K_ALIGN_DOWN((CurrentMD->address + CurrentMD->length));
        if(EndAddr  <= StartAddr) continue;
        TotalPage += (EndAddr - StartAddr) >> PAGE_4K_SHIFT;
    }
    ColorPrintfk(BLUE, BLACK, "Page Num OS Have Is %D Totally\n", TotalPage);

    // Include Memory Gap And ROM
    TotalMemory = 0;
    for(unsigned int i = 0; i < MMS.GMDLength; i++)
        if(MMS.descriptor[i].type == 1){
            unsigned long end = MMS.descriptor[i].address + MMS.descriptor[i].length;
            if(end > TotalMemory) TotalMemory = end;
        }
                    
    // Init BitsMap

    MMS.BitsMap = (unsigned long *)MEM_GAP_ALIGN(MMS.StartBrk);
    MMS.BitsMapCount = TotalMemory >> PAGE_4K_SHIFT;

    MMS.BitsMapLength = ((TotalMemory >> PAGE_4K_SHIFT) + 7) >> 3;

    memset(MMS.BitsMap, 0xff, MMS.BitsMapLength);

    // Init PageGroup
    // our pages struct is stored in an continuous area

    MMS.PagesGroup  = (struct Page*)MEM_GAP_ALIGN(((unsigned long)MMS.BitsMap + MMS.BitsMapLength));
    MMS.PagesCount   = TotalMemory >> PAGE_4K_SHIFT;
    MMS.PagesLength = (TotalMemory >> PAGE_4K_SHIFT) * sizeof(struct Page);
    memset(MMS.PagesGroup, 0x00, MMS.PagesLength);

    // Init ZoneGroup

    MMS.ZonesGroup  = (struct Zone*)MEM_GAP_ALIGN((unsigned long)MMS.PagesGroup + MMS.PagesLength);
    MMS.ZonesCount   = 0; // Assume Here
    MMS.ZonesLength = MMS.GMDLength * sizeof(struct Zone);
    memset(MMS.ZonesGroup, 0x00, MMS.ZonesLength);

    for(unsigned int i = 0; i < MMS.GMDLength; i++){
        if(MMS.descriptor[i].type != 1)
        continue;

        StartAddr   = PAGE_4K_ALIGN_UP(MMS.descriptor[i].address);
        EndAddr     = PAGE_4K_ALIGN_DOWN(MMS.descriptor[i].address + MMS.descriptor[i].length);

        if(StartAddr >= EndAddr)
        continue;


        struct Zone * z = MMS.ZonesGroup + MMS.ZonesCount;

        z -> GMM           = &MMS;

        z -> Attribute     = 0;

        z -> PagesGroup    = MMS.PagesGroup + (StartAddr >> PAGE_4K_SHIFT);
        z -> PageFreeCount = (EndAddr - StartAddr) >> PAGE_4K_SHIFT;
        z -> PagesCount    = z -> PageFreeCount;
        z -> PageUsingCount= 0;
        z -> TotalPagesLink= 0;

        z -> ZoneEndAddr   = EndAddr;
        z -> ZoneStartAddr = StartAddr;
        z -> ZoneLength    = EndAddr - StartAddr;

        MMS.ZonesCount++;

        for(unsigned long CurrentAddr = StartAddr; CurrentAddr < EndAddr; CurrentAddr += PAGE_4K_SIZE){
            struct Page * p = MMS.PagesGroup + (CurrentAddr >> PAGE_4K_SHIFT);
            MMS.BitsMap[BITS_MAP_4KB_INDEX(CurrentAddr)] ^= BITS_MAP_4KB_BIT_PATTERN(CurrentAddr);

            // Init PagesGroup
            p -> age           = 0;
            p -> Attribute     = 0;
            p -> PhyAddr       = CurrentAddr;
            p -> RefCount      = 0;
            p -> ZoneStruct    = z;
            p -> reserved      = 0;
            p->allocation_order = -1;
            ListInit(&p->buddy_list);
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

    // curiosity
    ZoneDmaIndex    = 0;
    ZoneNormalIndex = 1;
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

    reserve_phys_range(0, VIRT_TO_PHY(MMS.EndStruct),
                       PATTR(PG_KERNEL) | PATTR(PG_KERNEL_INIT) |
                       PATTR(PG_PTABLE_MAPPED));


    SlabCacheInit();
    
    init_buddy_system();

    // Flush The Consistency Mapping 

    // unsigned long CR3 = GetCr3();
 
    // ColorPrintfk(BLUE, BLACK, "CR3 : %X ; PML4E : %X ; PDPTE : %X ; \n", CR3, *PHY_TO_VIRT(CR3), \
    //                                                                     *PHY_TO_VIRT(*(PHY_TO_VIRT(CR3)) & (~0xff)));
    // *PHY_TO_VIRT(CR3 & (~0xfffUL)) = 0;

    // FlushTLB();
}

void InitPageTable(){
    unsigned long Cr3 = GetCr3();
    unsigned long *pml4 = PHY_TO_VIRT(Cr3 & PAGE_4K_MASK);

    for(unsigned long zi = 0; zi < MMS.ZonesCount; zi++){
        struct Zone *zone = MMS.ZonesGroup + zi;
        /* Map the complete enclosing huge-page range.  A usable E820 range
         * may start or end in the middle of a 2 MiB block, while the direct
         * map is still needed for every allocatable 4 KiB page in it. */
        unsigned long start = PAGE_2M_ALIGN_DOWN(zone->ZoneStartAddr);
        unsigned long end = PAGE_2M_ALIGN_UP(zone->ZoneEndAddr);

        for(unsigned long physical = start; physical < end;
            physical += PAGE_2M_SIZE){
            unsigned long vaddr = PAGE_OFFSET + physical;
            unsigned long *entry = pml4 + GetBits(vaddr, PAGE_GDT_SHIFT, 9);

            if(!*entry){
                void *page = kmalloc(PAGE_4K_SIZE, 0);
                if(!page) continue;
                memset(page, 0, PAGE_4K_SIZE);
                *entry = VIRT_TO_PHY(page) | PEA_SUPERVISOR_TABLE;
            }
            if(*entry & PEA_PAGE_SIZE) continue;

            unsigned long *pdpt = table_from_entry(*entry);
            entry = pdpt + GetBits(vaddr, PAGE_1G_SHIFT, 9);
            if(!*entry){
                void *page = kmalloc(PAGE_4K_SIZE, 0);
                if(!page) continue;
                memset(page, 0, PAGE_4K_SIZE);
                *entry = VIRT_TO_PHY(page) | PEA_SUPERVISOR_TABLE;
            }
            if(*entry & PEA_PAGE_SIZE) continue;

            unsigned long *pd = table_from_entry(*entry);
            entry = pd + GetBits(vaddr, PAGE_2M_SHIFT, 9);
            *entry = physical | PEA_SUPERVISOR_ENTRY;
        }
    }

    FlushTLB();
}

void InitPage(struct Page *p, unsigned long flag){
    p -> Attribute |= flag;

    if(!p->RefCount || (p->Attribute & PATTR(PG_SHARED))){
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
