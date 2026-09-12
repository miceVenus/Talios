#ifndef MMAP_H
#define MMAP_H

#include "vfs.h"
#include "vma.h"

#define PROT_NONE   0x00UL
#define PROT_READ   0x01UL
#define PROT_WRITE  0x02UL
#define PROT_EXEC   0x04UL

#define MAP_SHARED      0x0001UL
#define MAP_PRIVATE     0x0002UL
#define MAP_FIXED       0x0010UL
#define MAP_ANONYMOUS   0x0020UL

unsigned long __get_unmapped_area(
            file *file,
            unsigned long addr,
            unsigned long len,
            unsigned long pgoff,
            unsigned long flags,
            vm_flags_t vm_flags);

unsigned long do_mmap(
            file *file, 
            unsigned long addr,
            unsigned long len, 
            unsigned long prot,
			unsigned long flags, 
            vm_flags_t vm_flags,
            unsigned long pgoff);

int do_munmap(unsigned long addr, unsigned long len);


#endif
