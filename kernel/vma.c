#include "mmap.h"
#include "task.h"
#include "errno.h"

void lmm_init(struct LocalMemManager *mm){
    if(mm == NULL) return;

    ListInit(&mm->vma_list);
    mm->mmap_base = DEFAULT_MMAP_BASE;
}


void vma_destroy_all(struct LocalMemManager *mm){
    struct List *pos;
    struct List *next;

    if(mm == NULL || mm->vma_list.next == NULL ||
       mm->vma_list.prev == NULL)
        return;

    pos = mm->vma_list.next;
    while(pos != &mm->vma_list){
        vm_area_struct *vma = ContainerOf(pos, vm_area_struct, list);
        next = pos->next;

        ListDelete(&vma->list);
        vm_object_detach(vma);
        kfree(vma);
        pos = next;
    }
}


int vma_clone_all(struct LocalMemManager *src,
                  struct LocalMemManager *dst){
    struct List *pos;

    if(src == NULL || dst == NULL || src == dst ||
       src->vma_list.next == NULL ||
       dst->vma_list.next == NULL)
        return -EINVAL;

    for(pos = src->vma_list.next;
        pos != &src->vma_list;
        pos = pos->next){
        vm_area_struct *src_vma = ContainerOf(pos, vm_area_struct, list);
        vm_area_struct *dst_vma =
            (vm_area_struct *)kmalloc(sizeof(vm_area_struct), 0);

        if(dst_vma == NULL){
            vma_destroy_all(dst);
            return -ENOMEM;
        }

        memcopy(src_vma, dst_vma, sizeof(vm_area_struct));
        ListInit(&dst_vma->list);
        dst_vma->vm_mm = dst;
        vm_object_attach(dst_vma);
        ListForeAdd(&dst_vma->list, &dst->vma_list);
    }

    return 0;
}


vm_area_struct *find_vma(struct LocalMemManager *mm,
                         unsigned long addr){
    struct List *head;
    struct List *ptr;

    if(mm == NULL || mm->vma_list.next == NULL)
        return NULL;

    head = &mm->vma_list;
    ptr  = head->next;

    while(ptr != head){
        vm_area_struct * vma = ContainerOf(ptr, vm_area_struct, list);
        /* Linux find_vma() returns the first VMA whose end is after addr.
         * The caller must still check addr >= vma->start when it needs an
         * actual containment test. */
        if(addr < vma->end) return vma;
        ptr = ptr->next;
    }

    return NULL;

}


int insert_vma(struct LocalMemManager *mm,
               vm_area_struct *vma){
    struct List *head;
    struct List *ptr;

    if(mm == NULL || vma == NULL ||
       mm->vma_list.next == NULL ||
       vma->start >= vma->end ||
       vma->start < PAGE_4K_SIZE || vma->end > USER_LIMIT ||
       (vma->start & (PAGE_4K_SIZE - 1)) != 0 ||
       (vma->end & (PAGE_4K_SIZE - 1)) != 0)
        return -EINVAL;

    head = &mm->vma_list;
    ptr  = head->next;
    vma->vm_mm = mm;

    while(ptr != head){
        vm_area_struct *p_vma = ContainerOf(ptr, vm_area_struct, list);

        if(vma->end <= p_vma->start)
            break;

        if(vma->start < p_vma->end)
            return -EEXIST;

        ptr = ptr->next;
    }

    ListForeAdd(&vma->list, ptr);

    return 1;
}

int range_is_free(struct LocalMemManager *mm,
                  unsigned long start, unsigned long len){
    unsigned long end;

    if(mm == NULL || mm->vma_list.next == NULL ||
       len == 0 || start < PAGE_4K_SIZE ||
       start >= USER_LIMIT || len > USER_LIMIT - start){
        return 0;
    }

    end = start + len;

    struct List *ptr = mm->vma_list.next;

    while(ptr != &mm->vma_list){
        vm_area_struct *vma = ContainerOf(ptr, vm_area_struct, list);

        if(end <= vma->start)
            return 1;

        if(start < vma->end && end > vma->start)
            return 0;

        ptr = ptr->next;
    }

    return 1;
}

static unsigned long find_gap(struct LocalMemManager *mm, unsigned long cur, unsigned long len){
    struct List *head = &mm->vma_list;
    for(struct List *pos = head->next; pos != head; pos = pos->next){
        vm_area_struct *v = ContainerOf(pos, vm_area_struct, list);
        if(v->end <= cur) continue;
        if(v->start >= cur && len <= v->start - cur) return cur;
        cur = v->end;
    }
    return len <= USER_LIMIT - cur ? cur : (unsigned long)-ENOMEM;
}

unsigned long find_unmapped_area(struct LocalMemManager *mm, unsigned long len){
    if(!mm || !mm->vma_list.next || !len || (len & (PAGE_4K_SIZE - 1)) ||
       len > USER_LIMIT - PAGE_4K_SIZE) return -ENOMEM;
    unsigned long addr = find_gap(mm, mm->mmap_base, len);
    if((long)addr < 0) addr = find_gap(mm, PAGE_4K_SIZE, len);
    return addr;
}
