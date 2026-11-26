#ifndef TALIOS_MMAP_H
#define TALIOS_MMAP_H
#include "../include/mman.h"
#include "vma.h"

struct LocalMemManager *mm_create(void);
struct LocalMemManager *mm_fork(struct LocalMemManager *source);
void mm_put(struct LocalMemManager *mm);
void vm_init(void);
unsigned long vm_cpu_init(void);
void vm_lock(void);
void vm_unlock(void);
void vm_writeprotect(struct LocalMemManager *mm, unsigned long addr, struct Page *page);
void vm_unmap_pages(struct LocalMemManager *mm, unsigned long start, unsigned long end);

unsigned long vm_map(struct LocalMemManager *mm, file *filp, unsigned long addr,
                     unsigned long len, unsigned long prot, unsigned long flags,
                     unsigned long offset);
long vm_unmap(struct LocalMemManager *mm, unsigned long addr, unsigned long len);
long vm_sync(struct LocalMemManager *mm, unsigned long addr, unsigned long len);
unsigned long vm_brk(struct LocalMemManager *mm, unsigned long addr);

/* The VM lock protects backing objects, resident pages and address spaces. */
void vm_object_attach(vm_area_struct *vma);
void vm_object_detach(vm_area_struct *vma);
int vm_copy_user_locked(void *user, void *kernel, unsigned long len, int write);
int vm_backing_page(vm_area_struct *vma, unsigned long addr, int writing,
                    struct Page **page);
int vm_file_io(file *filp, char *buf, unsigned long count, int write);
int vm_file_truncate(file *filp);
#endif
