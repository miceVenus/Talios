#ifndef MEMORY_H
#define MEMORY_H

#include "lib.h"

#define MAX_GMD_LEN     32
#define MEM_STRUCT_ADDR 0xffff800000007e00

// ONLY MAP FIRST 10MB FROM PAGE_OFFSET TO PHYSIC 0x0
#define PTRS_PER_PAGE   512
#define PAGE_OFFSET     ((unsigned long)0xffff800000000000)
#define PAGE_GDT_SHIFT  39
#define PAGE_1G_SHIFT   30
#define PAGE_2M_SHIFT   21
#define PAGE_4K_SHIFT   12

#define PAGE_2M_SIZE    (1UL << PAGE_2M_SHIFT)
#define PAGE_4K_SIZE    (1UL << PAGE_4K_SHIFT)

#define PAGE_2M_MASK    (~ (PAGE_2M_SIZE - 1))
#define PAGE_4K_MASK    (~ (PAGE_4K_SIZE - 1))

#define PAGE_2M_ALIGN_DOWN(addr) ((unsigned long)(addr) & PAGE_2M_MASK)
#define PAGE_4K_ALIGN_DOWN(addr) ((unsigned long)(addr) & PAGE_4K_MASK)
#define PAGE_2M_ALIGN_UP(addr) (((unsigned long)(addr) + PAGE_2M_SIZE - 1) & PAGE_2M_MASK)
#define PAGE_4K_ALIGN_UP(addr) (((unsigned long)(addr) + PAGE_4K_SIZE - 1) & PAGE_4K_MASK)

#define VIRT_TO_PHY(addr)   ((unsigned long)(addr) - PAGE_OFFSET)
#define PHY_TO_VIRT(addr)   ((unsigned long *)((unsigned long)(addr) + PAGE_OFFSET))

#define PAGE_2M_INDEX(Paddr)             ((Paddr) >> PAGE_2M_SHIFT)
#define BITS_MAP_INDEX(Paddr)            (PAGE_2M_INDEX(Paddr) >> 6)
#define BITS_MAP_BIT_OFFSET(Paddr)       (PAGE_2M_INDEX(Paddr) % BITS_PER_LONG)
#define BITS_MAP_BIT_PATTERN(Paddr)      (1UL << (BITS_PER_LONG - 1 - BITS_MAP_BIT_OFFSET(Paddr)))

#define BITS_PER_LONG     ((sizeof(long) << 3))

#define BITS_MAP_LENGTH(count) (((count) + (sizeof(long) << 3) - 1) >> 6)


// Set a Small Mem Gap
#define MEM_GAP_ALIGN(addr)  (((unsigned long)(addr) + (sizeof(long) << 3)) & (~(sizeof(long) - 1)))

#define ALIGN_WITH_LONG(num)    (((num) + sizeof(long) - 1) & (~(sizeof(long) - 1)))

#define PATTR(attr)     (1UL << (attr))

#define KMALLOC_SLAB_SIZE 16

#define SetPML4E(PML4, PML4E, flags)    (*(PML4) = ((PML4E) | (flags)))
#define SetPDPTE(PDPT, PDPTE, flags)    (*(PDPT) = ((PDPTE) | (flags)))
#define SetPDE(PD, PDE, flags)          (*(PD) = ((PDE) | (flags)))




enum PageAttribute{
    // mapped = 1 or unmapped = 0
    PG_PTABLE_MAPPED = 0 ,

    // Is Used In Init Code?
    PG_KERNEL_INIT      , 

    PG_ACTIVE           ,

    // Is Used In Kernel?
    PG_KERNEL           ,

    PG_REFERENCED       ,

    // Is Shared Or Only Page?
    PG_K_SHARE_To_U     ,

    // Is Used In Device?
    PG_DEVICE           ,

    // SHARED PAGE
    PG_SHARED
};

enum PageTableEntryAttribute{
    // 0 1    2   3   4   5  6  7       8  9 ~ 11
    // P R/W  U/S PWT PCD A  D  PAT/PS  G  AVL

    PEA_PRESENT             = (1UL << 0),
    PEA_READ_WRITE          = (1UL << 1),
    PEA_IS_USER             = (1UL << 2),
    PEA_WRITE_THROUGH       = (1UL << 3),
    PEA_CACHE_DISABLE       = (1UL << 4),
    PEA_PAGE_SIZE           = (1UL << 7),

    PEA_USER_TABLE          = PEA_PRESENT | PEA_READ_WRITE | PEA_IS_USER,
    PEA_USER_ENTRY          = PEA_USER_TABLE | PEA_PAGE_SIZE, 
    PEA_SUPERVISOR_TABLE    = PEA_PRESENT | PEA_READ_WRITE,
    PEA_SUPERVISOR_ENTRY    = PEA_SUPERVISOR_TABLE | PEA_PAGE_SIZE
};

enum ZONE_INDEX{
    ZONE_NORMAL_INDEX = 0   ,
    ZONE_UNMAPED_INDEX      ,
    ZONE_DMA_INDEX          ,
};

struct E820{
    unsigned long address;
    unsigned long length;
    unsigned int  type;
}__attribute__((packed));

struct GlobalMemManager{
    struct E820 descriptor[MAX_GMD_LEN];
    unsigned int GMDLength;

    unsigned long*  BitsMap;
    unsigned long   BitsMapCount;    // Total bits in BitsMap
    unsigned long   BitsMapLength;  // Bytes Of BitsMap

    struct Page*    PagesGroup;
    unsigned long   PagesCount;
    unsigned long   PagesLength;

    struct Zone*    ZonesGroup;
    unsigned long   ZonesCount;
    unsigned long   ZonesLength;


    // Kernel Level
    unsigned long   StartCode;
    unsigned long   EndCode;
    unsigned long   EndData;
    unsigned long   EndRoData;
    unsigned long   StartBrk;

    // MemPageManagerStruct End
    unsigned long   EndStruct;
};

struct Zone{
    struct Page *   PagesGroup;
    unsigned long   PagesCount;

    unsigned long   ZoneStartAddr;
    unsigned long   ZoneEndAddr;
    unsigned long   ZoneLength;
    unsigned long   Attribute;

    struct GlobalMemManager * GMM;

    unsigned long   PageUsingCount;
    unsigned long   PageFreeCount;

    unsigned long   TotalPagesLink;
};

struct Page{
    struct Zone *   ZoneStruct;
    unsigned long   PhyAddr;
    unsigned long   Attribute;
    unsigned long   RefCount;
    unsigned long   age;
};

struct Slab{
    struct List list;
    struct Page *page;
    void *Vaddress;

    unsigned long* ColorMap;
    unsigned long UsingCount;
    unsigned long FreeCount;

    unsigned long ColorLength;
    unsigned long ColorCount;
};

struct SlabCache{
    unsigned long size;
    unsigned long TotalFree;
    unsigned long TotalUse;
    void * (*Constructor)(void* Vaddress, unsigned long arg);
    void * (*Destructor)(void* Vaddress, unsigned long arg);
    struct Slab *CachePool;
    struct Slab *CacheDmaPool;
};


void InitMemory();
void* kmalloc(unsigned long size, unsigned long flags);
unsigned int kfree(void *ptr);
void InitPage(struct Page *p, unsigned long flag);
void CleanPage(struct Page *p);
unsigned long SlabCacheInit();
unsigned long GetPageAttr(struct Page* p);
unsigned long SetPageAttr(struct Page* p, unsigned long flags);
int FreePage(struct Page* page, int number);
struct Page *AllocPage(int ZoneSelector, int number, unsigned long PageAttr);
int FreeSlab(struct SlabCache *SC, void *Vaddress, unsigned long arg);
int DeleteSlabCache(struct SlabCache *SC);
struct SlabCache* CreateSlabCache(  unsigned long SlabSize, void *(*Constructor)(void *Vaddr, unsigned long arg), 
                                    void *(*Destructor)(void *Vaddr, unsigned long arg), unsigned long arg);
struct Slab* CreatSlab(unsigned long size, int ZoneSelector);
void InitPageTable();


#endif