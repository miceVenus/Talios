#ifndef MEMORY_H
#define MEMORY_H


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
#define PAGE_4K_SIZE    (~ (PAGE_4K_SIZE - 1))

#define PAGE_2M_ALIGN(addr) (((unsigned long)addr + PAGE_2M_SIZE - 1) & PAGE_2M_MASK)
#define PAGE_4K_ALIGN(addr) (((unsigned long)addr + PAGE_4K_SIZE - 1) & PAGE_4K_MASK)

#define VIRT_TO_PHY(addr)   ((unsigned long)addr - PAGE_OFFSET)
#define PHY_TO_VIRT(addr)   ((unsigned long *)((unsigned long)addr + PAGE_OFFSET))

struct MemoryE820Formate{
    unsigned int addrL;
    unsigned int addrH;
    unsigned int lengthL;
    unsigned int lengthH;
    unsigned int type;
};

void InitMemory();

#endif