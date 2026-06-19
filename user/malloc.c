static unsigned long brk_start  = 0;
static unsigned long brk_end    = 0;
static unsigned long brk_used   = 0;
#define SIZE_ALIGN  (sizeof(long) << 3)
#define PAGE_SIZE   (1 << 21)

#define PAGE_2M_ALIGN_DOWN(addr) ((unsigned long)(addr) & PAGE_2M_MASK)
#define PAGE_4K_ALIGN_DOWN(addr) ((unsigned long)(addr) & PAGE_4K_MASK)
#define PAGE_2M_ALIGN_UP(addr) (((unsigned long)(addr) + PAGE_2M_SIZE - 1) & PAGE_2M_MASK)
#define PAGE_4K_ALIGN_UP(addr) (((unsigned long)(addr) + PAGE_4K_SIZE - 1) & PAGE_4K_MASK)

#include "stdio.h"

void *malloc(unsigned long size){
    unsigned long addr = 0;
    if(!brk_start) brk_start = brk_used = brk(brk_start);
    if(brk_end <= brk_used + size + SIZE_ALIGN)
        brk_end = brk(brk_end + ((size + SIZE_ALIGN + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1)));

    addr = brk_used;
    brk_used += size + SIZE_ALIGN;

    return (void*)addr;

}

void free(void *addr){

}