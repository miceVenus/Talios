#include "mmap.h"
#include "task.h"
#include "errno.h"

unsigned long __get_unmapped_area(
        struct file *file, unsigned long addr, 
        unsigned long len, unsigned long pgoff, 
        unsigned long flags, vm_flags_t vm_flags){

    struct LocalMemManager *lmm = CURRENT->lmm;

    (void)file;
    (void)pgoff;
    (void)vm_flags;

    if(lmm == NULL || len == 0 ||
       (len & (PAGE_4K_SIZE - 1)) != 0)
        return (unsigned long)-EINVAL;

    if(flags & MAP_FIXED){
        if(addr & (PAGE_4K_SIZE - 1)) return (unsigned long)-EINVAL;
        if(addr < PAGE_4K_SIZE || addr >= USER_LIMIT ||
           len > (USER_LIMIT - addr))
            return (unsigned long)-ENOMEM;

        return addr;
    }

    if(addr != 0){
        unsigned long hint = PAGE_4K_ALIGN_DOWN(addr);
        if(range_is_free(lmm, hint, len)) return hint;
    }

    return find_unmapped_area(lmm, len);
}


unsigned long do_mmap(
            file *file, 
            unsigned long addr,
			unsigned long len, 
            unsigned long prot,
			unsigned long flags, 
            vm_flags_t vm_flags,
			unsigned long pgoff){
    
    struct LocalMemManager *lmm = CURRENT->lmm;
    vm_area_struct *vma;
    int ret;

    if(lmm == NULL || !len)
        return (unsigned long)-EINVAL;

    if(prot & ~(PROT_READ | PROT_WRITE | PROT_EXEC))
        return (unsigned long)-EINVAL;

    if(flags & ~(MAP_SHARED | MAP_PRIVATE |
                 MAP_FIXED | MAP_ANONYMOUS))
        return (unsigned long)-EINVAL;

    if((flags & MAP_SHARED) && (flags & MAP_PRIVATE))
        return (unsigned long)-EINVAL;

    if(!(flags & MAP_SHARED) && !(flags & MAP_PRIVATE))
        return (unsigned long)-EINVAL;

    if((flags & MAP_ANONYMOUS) && file != NULL)
        return (unsigned long)-EINVAL;

    if(!(flags & MAP_ANONYMOUS) && file == NULL)
        return (unsigned long)-EBADF;

    if(!(flags & MAP_ANONYMOUS))
        return (unsigned long)-EOPNOTSUPP;

    /* PAGE_ALIGN must not wrap around. */
    if(len > USER_LIMIT - (PAGE_4K_SIZE - 1))
        return (unsigned long)-ENOMEM;

    len = PAGE_4K_ALIGN_UP(len);
    if(len == 0 || len > USER_LIMIT)
        return (unsigned long)-ENOMEM;

    if((len >> PAGE_4K_SHIFT) > (~0UL - pgoff))
        return (unsigned long)-EOVERFLOW;

    if(pgoff > (~0UL >> PAGE_4K_SHIFT))
        return (unsigned long)-EOVERFLOW;

    addr = __get_unmapped_area(file, addr, len, pgoff, flags, vm_flags);
    if ((long)addr < 0)
        return addr;

    /* This first stage does not replace existing mappings. */
    if((flags & MAP_FIXED) && !range_is_free(lmm, addr, len))
        return (unsigned long)-EEXIST;

    vma = kmalloc(sizeof(vm_area_struct), 0);
    if(vma == NULL)
        return (unsigned long)-ENOMEM;

    memset(vma, 0, sizeof(vm_area_struct));
    ListInit(&vma->list);
    vma->vm_mm = lmm;
    vma->start = addr;
    vma->end = addr + len;
    vma->prot = prot;
    vma->flags = vm_flags;
    vma->mmap_flags = flags;
    vma->file = file;
    vma->file_offset = pgoff << PAGE_4K_SHIFT;

    ret = insert_vma(lmm, vma);
    if(ret != 1){
        kfree(vma);
        return (ret < 0) ? (unsigned long)ret :
                           (unsigned long)-EEXIST;
    }

    return addr;
}

static void unmap_anonymous_pages(struct LocalMemManager *lmm,
                                  unsigned long start,
                                  unsigned long end){
    unsigned long address;

    for(address = start; address < end; address += PAGE_4K_SIZE){
        struct Page *page = UnmapPage4K((unsigned long)lmm->pgd,
                                        address);
        if(page != NULL)
            FreePage4K(page, 1);
    }
}

int do_munmap(unsigned long addr, unsigned long len){
    struct LocalMemManager *lmm = CURRENT->lmm;
    struct List *head;
    struct List *pos;
    unsigned long end;
    int unmapped = 0;

    if(lmm == NULL || lmm->pgd == NULL || len == 0 ||
       (addr & (PAGE_4K_SIZE - 1)) != 0)
        return -EINVAL;

    if(len > USER_LIMIT - (PAGE_4K_SIZE - 1))
        return -EINVAL;
    len = PAGE_4K_ALIGN_UP(len);

    if(addr < PAGE_4K_SIZE || addr >= USER_LIMIT ||
       len == 0 || len > USER_LIMIT - addr)
        return -EINVAL;

    end = addr + len;

    head = &lmm->vma_list;
    if(head->next == NULL || head->prev == NULL)
        return -EINVAL;

    pos = head->next;
    while(pos != head){
        vm_area_struct *vma = ContainerOf(pos, vm_area_struct, list);
        struct List *next = pos->next;
        unsigned long unmap_start;
        unsigned long unmap_end;
        vm_area_struct *right = NULL;

        if(vma->end <= addr){
            pos = next;
            continue;
        }
        if(vma->start >= end)
            break;

        /* File-backed mappings are not supported by this stage. */
        if(!(vma->mmap_flags & MAP_ANONYMOUS) || vma->file != NULL)
            return -EOPNOTSUPP;

        unmap_start = addr > vma->start ? addr : vma->start;
        unmap_end = end < vma->end ? end : vma->end;

        if(unmap_start > vma->start && unmap_end < vma->end){
            right = (vm_area_struct *)kmalloc(sizeof(vm_area_struct), 0);
            if(right == NULL)
                return -ENOMEM;

            memcopy(vma, right, sizeof(vm_area_struct));
            ListInit(&right->list);
            right->vm_mm = lmm;
            right->start = unmap_end;
            right->file_offset += unmap_end - vma->start;
        }

        unmap_anonymous_pages(lmm, unmap_start, unmap_end);

        if(unmap_start == vma->start && unmap_end == vma->end){
            ListDelete(&vma->list);
            kfree(vma);
        }else if(unmap_start == vma->start){
            vma->start = unmap_end;
        }else if(unmap_end == vma->end){
            vma->end = unmap_start;
        }else{
            vma->end = unmap_start;
            ListBackAdd(&vma->list, &right->list);
        }

        unmapped = 1;
        pos = next;
    }

    if(unmapped)
        FlushTLB();

    return unmapped ? 0 : -EINVAL;
}
