#ifndef MEMORY_H
#define MEMORY_H

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


// Set a Small Mem Gap
#define MEM_GAP_ALIGN(addr)  (((unsigned long)(addr) + (sizeof(long) << 5)) & (~(sizeof(long) - 1)))

#define PATTR(attr)     (1UL << (attr))


enum PageAttribute{
    PG_PTable_Maped = 0 ,
    PG_Kernel_Init      , 
    PG_Active           ,
    PG_Kernel           ,
    PG_Referenced       ,
    PG_K_Share_To_U
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

struct GlobalMemDescriptor{
    struct E820 descriptor[MAX_GMD_LEN];
    unsigned int GMDLength;

    unsigned long*  BitsMap;
    unsigned long   BitsMapSize;    // Total bits in BitsMap
    unsigned long   BitsMapLength;  // Bytes Of BitsMap

    struct Page*    PagesGroup;
    unsigned long   PagesSize;
    unsigned long   PagesLength;

    struct Zone*    ZonesGroup;
    unsigned long   ZonesSize;
    unsigned long   ZonesLength;


    // Kernel Level
    unsigned long   StartCode;
    unsigned long   EndCode;
    unsigned long   EndData;
    unsigned long   EndBrk;

    // MemPageManagerStruct End
    unsigned long   EndStruct;
};

struct Zone{
    struct Page *   PagesGroup;
    unsigned long   PagesSize;

    unsigned long   ZoneStartAddr;
    unsigned long   ZoneEndAddr;
    unsigned long   ZoneLength;
    unsigned long   Attribute;

    struct GlobalMemDescriptor * GMD;

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

void InitMemory();
void PageInit(struct Page *p, unsigned long flag);
struct Page *AllocPage(int ZoneSelector, int number, unsigned long PageAttr);

#endif