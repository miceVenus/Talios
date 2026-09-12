#ifndef VMA_H
#define VMA_H

#include "lib.h"
#include "vfs.h"
#include "memory.h"

#define USER_LIMIT 0x0000800000000000UL
#define DEFAULT_MMAP_BASE 0x0000004000000000UL


typedef unsigned long vm_flags_t;


typedef struct vm_area_struct{
    struct List list;

    struct LocalMemManager *vm_mm;

    /* virtual memory area [start, end)*/
    
    unsigned long start;
    unsigned long end;

    unsigned long prot;

    vm_flags_t flags;
    unsigned long mmap_flags;

    unsigned long file_offset;

    file *file;

}vm_area_struct;


void lmm_init(struct LocalMemManager *mm);
void vma_destroy_all(struct LocalMemManager *mm);
int vma_clone_all(struct LocalMemManager *src,
                  struct LocalMemManager *dst);

/* Return the first VMA with vm_end > addr, or NULL. */
vm_area_struct *find_vma(struct LocalMemManager *mm,
                         unsigned long addr);
int insert_vma(struct LocalMemManager *mm,
               vm_area_struct *new_vma);
unsigned long find_unmapped_area(struct LocalMemManager *mm,
                                 unsigned long size);
int range_is_free(struct LocalMemManager *mm,
                  unsigned long start, unsigned long len);


#endif
