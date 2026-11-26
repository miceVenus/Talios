#include "mmap.h"
#include "task.h"
#include "errno.h"
#include "cpu.h"
#include "smp.h"
#include "semaphore.h"

#define PTE_ADDRESS 0x000ffffffffff000UL
#define PF_PRESENT  1UL
#define PF_WRITE    2UL
#define PF_RESERVED 8UL
#define PF_FETCH   16UL

extern struct GlobalMemManager MMS;
extern struct LocalMemManager InitLmm;
static semaphore_t vm_mutex;
static unsigned long nx_flag;

unsigned long vm_cpu_init(void){
    unsigned long nx = 0;
    unsigned int a, b, c, d;
    CPUID(0x80000001, 0, &a, &b, &c, &d);
    if(d & (1U << 20)){
        wrmsr(0xc0000080, rdmsr(0xc0000080) | (1UL << 11));
        nx = PEA_NO_EXEC;
    }
    unsigned long cr0;
    __asm__ volatile("mov %%cr0,%0" : "=r"(cr0));
    __asm__ volatile("mov %0,%%cr0" :: "r"(cr0 | (1UL << 16)) : "memory");
    return nx;
}

void vm_init(void){
    nx_flag = vm_cpu_init();
    semaphore_init(&vm_mutex, 1);
}

void vm_lock(void){ semaphore_acquire(&vm_mutex); }
void vm_unlock(void){ semaphore_release(&vm_mutex); }

static struct Page *pte_page(unsigned long pte){
    return &MMS.PagesGroup[(pte & PTE_ADDRESS) >> PAGE_4K_SHIFT];
}

static unsigned long *lookup_pte(unsigned long pgd, unsigned long addr, int create){
    unsigned long *table = PHY_TO_VIRT(pgd);
    for(int shift = PAGE_GDT_SHIFT; shift > PAGE_4K_SHIFT; shift -= 9){
        unsigned long *entry = &table[(addr >> shift) & 511];
        if(!*entry && create){
            void *child = kmalloc(PAGE_4K_SIZE, 0);
            if(!child) return NULL;
            memset(child, 0, PAGE_4K_SIZE);
            *entry = VIRT_TO_PHY(child) | PEA_USER_TABLE;
        }
        if(!(*entry & PEA_PRESENT) || (*entry & PEA_PAGE_SIZE)) return NULL;
        table = PHY_TO_VIRT(*entry & PTE_ADDRESS);
    }
    return &table[(addr >> PAGE_4K_SHIFT) & 511];
}

int MapPage4K(unsigned long pgd, unsigned long addr, struct Page *page, unsigned long flags){
    if(!pgd || !page || addr >= USER_LIMIT || (addr & (PAGE_4K_SIZE - 1))) return -EINVAL;
    unsigned long *pte = lookup_pte(pgd, addr, 1);
    if(!pte) return -ENOMEM;
    if(*pte & PEA_PRESENT) return -EEXIST;
    *pte = page->PhyAddr | flags;
    return 0;
}

static void invalidate(struct LocalMemManager *mm, unsigned long addr){
    smp_invalidate((unsigned long)mm->pgd, addr);
}

void vm_writeprotect(struct LocalMemManager *mm, unsigned long addr, struct Page *page){
    unsigned long *pte = lookup_pte((unsigned long)mm->pgd, addr, 0);
    if(pte && (*pte & PEA_READ_WRITE) && (*pte & PTE_ADDRESS) == page->PhyAddr){
        *pte &= ~PEA_READ_WRITE;
        invalidate(mm, addr);
    }
}

/* Walk existing subtrees only. An untouched, terabyte-sized reservation
 * costs no page-table allocations or per-page work during fork or unmap. */
static int walk_resident(struct LocalMemManager *mm, unsigned long *table, int shift,
                         unsigned long start, unsigned long end, struct LocalMemManager *child){
    unsigned long span = 1UL << shift;
    for(unsigned long addr = start; addr < end; ){
        unsigned long limit = min(end, (addr & ~(span - 1)) + span);
        unsigned long *entry = &table[(addr >> shift) & 511];
        if(*entry & PEA_PRESENT){
            if(shift > PAGE_4K_SHIFT){
                if(*entry & PEA_PAGE_SIZE) return -EINVAL;
                unsigned long *next = PHY_TO_VIRT(*entry & PTE_ADDRESS);
                int rc = walk_resident(mm, next, shift - 9, addr, limit, child);
                if(rc) return rc;
                if(!child){
                    int i = 0;
                    while(i < 512 && !next[i]) i++;
                    if(i == 512){
                        *entry = 0;
                        invalidate(mm, addr);
                        kfree(next);
                    }
                }
            }else if(child){
                vm_area_struct *vma = find_vma(mm, addr);
                if(!vma || addr < vma->start) return -EFAULT;
                unsigned long flags = *entry & ~PTE_ADDRESS;

                if((vma->mmap_flags & MAP_PRIVATE) && (flags & PEA_READ_WRITE))
                    flags = (flags & ~PEA_READ_WRITE) | PEA_COW;
                    
                struct Page *page = pte_page(*entry);
                int rc = MapPage4K((unsigned long)child->pgd, addr, page, flags);
                if(rc) return rc;
                page_get(page);
                if(flags & PEA_COW){
                    *entry = page->PhyAddr | flags;
                    invalidate(mm, addr);
                }
            }else{
                struct Page *page = pte_page(*entry);
                *entry = 0;
                invalidate(mm, addr);
                page_put(page);
            }
        }
        addr = limit;
    }
    return 0;
}

void vm_unmap_pages(struct LocalMemManager *mm, unsigned long start, unsigned long end){
    walk_resident(mm, PHY_TO_VIRT(mm->pgd), PAGE_GDT_SHIFT, start, end, NULL);
}

struct LocalMemManager *mm_create(void){
    struct LocalMemManager *mm = kmalloc(sizeof(*mm), 0);
    if(!mm) return NULL;
    unsigned long *root = kmalloc(PAGE_4K_SIZE, 0);
    if(!root){ kfree(mm); return NULL; }
    memset(mm, 0, sizeof(*mm));
    lmm_init(mm);
    mm->users = 1;
    mm->pgd = (pml4t_t *)VIRT_TO_PHY(root);
    memset(root, 0, PAGE_4K_SIZE / 2);
    unsigned long *master = PHY_TO_VIRT(InitLmm.pgd);
    for(int i = 256; i < 512; i++) root[i] = master[i] & ~PEA_IS_USER;
    return mm;
}

struct LocalMemManager *mm_fork(struct LocalMemManager *src){
    struct LocalMemManager *dst = mm_create();
    if(!dst) return NULL;
    vm_lock();
    pml4t_t *root = dst->pgd;
    *dst = *src;
    lmm_init(dst);
    dst->mmap_base = src->mmap_base;
    dst->users = 1;
    dst->pgd = root;
    int rc = vma_clone_all(src, dst);
    if(!rc)
        rc = walk_resident(src, PHY_TO_VIRT(src->pgd), PAGE_GDT_SHIFT, 0, USER_LIMIT, dst);
    vm_unlock();
    if(rc){ mm_put(dst); return NULL; }
    return dst;
}

void mm_put(struct LocalMemManager *mm){
    if(!mm || mm == &InitLmm) return;
    vm_lock();
    if(--mm->users == 0){
        vm_unmap_pages(mm, 0, USER_LIMIT);
        vma_destroy_all(mm);
        kfree(PHY_TO_VIRT(mm->pgd));
        kfree(mm);
    }
    vm_unlock();
}

/* On success the PTE owns a reference to its frame. The backing object,
 * when present, holds a separate reference even after all PTEs disappear. */
static int cow_write(struct LocalMemManager *mm, unsigned long addr, unsigned long *pte){
    struct Page *old = pte_page(*pte);
    struct Page *page = old;
    if(old->RefCount > 1){
        page = AllocPage4K(ZONE_NORMAL_INDEX, 1, 0);
        if(!page) return -ENOMEM;
        memcopy(PHY_TO_VIRT(old->PhyAddr), PHY_TO_VIRT(page->PhyAddr), PAGE_4K_SIZE);
    }
    *pte = page->PhyAddr | (*pte & ~(PTE_ADDRESS | PEA_COW)) | PEA_READ_WRITE;
    invalidate(mm, addr);
    if(page != old) page_put(old);
    return 0;
}

static int resolve(struct LocalMemManager *mm, unsigned long addr, unsigned long access){
    vm_area_struct *vma = find_vma(mm, addr);
    /* x86 user pages that can be written or executed are also readable. */
    unsigned long allowed = access == PROT_READ ? PROT_READ | PROT_WRITE | PROT_EXEC : access;
    if(!vma || addr < vma->start || !(vma->prot & allowed)) return -EFAULT;
    
    int write = access == PROT_WRITE;

    unsigned long *pte = lookup_pte((unsigned long)mm->pgd, addr, 1);
    if(!pte) return -ENOMEM;
    unsigned long flags = PEA_PRESENT | PEA_IS_USER;
    if(!(vma->prot & PROT_EXEC)) flags |= nx_flag;
    struct Page *page;
    if(*pte & PEA_PRESENT){
        if(!write || (*pte & PEA_READ_WRITE)) return 0;
        if(*pte & PEA_COW) return cow_write(mm, addr, pte);
        if(!(vma->mmap_flags & MAP_SHARED)) return -EACCES;
    }

    if(vma->object){
        int rc = vm_backing_page(vma, addr, write, &page);
        if(rc) return rc;
        if((vma->mmap_flags & MAP_PRIVATE) && (vma->prot & PROT_WRITE)){
            flags |= PEA_COW;
        }else if(write){
            flags |= PEA_READ_WRITE;
        }
    }else{
        page = AllocPage4K(ZONE_NORMAL_INDEX, 1, 0);
        if(!page) return -ENOMEM;
        memset(PHY_TO_VIRT(page->PhyAddr), 0, PAGE_4K_SIZE);
        if(vma->prot & PROT_WRITE) flags |= PEA_READ_WRITE;
    }

    if(*pte & PEA_PRESENT) page_put(pte_page(*pte));
    *pte = page->PhyAddr | flags;
    invalidate(mm, addr);
    if(write && (flags & PEA_COW)) return cow_write(mm, addr, pte);
    return 0;
}

int handle_page_fault(struct LocalMemManager *mm, unsigned long addr, unsigned long error){
    if(addr >= USER_LIMIT || !mm || !mm->pgd || (error & PF_RESERVED) ||
       ((error & PF_PRESENT) && !(error & PF_WRITE))) return -EFAULT;
    unsigned long access = (error & PF_WRITE) ? PROT_WRITE :
                           (error & PF_FETCH) ? PROT_EXEC : PROT_READ;
    vm_lock();
    int rc = resolve(mm, PAGE_4K_ALIGN_DOWN(addr), access);
    vm_unlock();
    return rc;
}

int vm_copy_user_locked(void *user, void *kernel, unsigned long len, int write){
    unsigned long addr = (unsigned long)user;
    if(!verify_area(user, len)) return -EFAULT;
    while(len){
        unsigned long part = min(len, PAGE_4K_SIZE - (addr & (PAGE_4K_SIZE - 1)));
        int rc = resolve(CURRENT->lmm, PAGE_4K_ALIGN_DOWN(addr), write ? PROT_WRITE : PROT_READ);
        if(rc) return rc;
        if(kernel){
            if(write) memcopy(kernel, (void *)addr, part);
            else memcopy((void *)addr, kernel, part);
            kernel = (char *)kernel + part;
        }
        addr += part;
        len -= part;
    }
    return 0;
}

