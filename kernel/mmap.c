#include "mmap.h"
#include "errno.h"
#include "fcntl.h"
#include "task.h"

#define VM_HEAP 1UL

struct vm_object {
    unsigned long refs;
    file *filp;

    // cached page list
    struct List pages;

    // vam list
    struct List mappings;
};

struct cached_page {
    struct List list;
    unsigned long offset;
    struct Page *page;
    int dirty;
};

static struct vm_object *get_object(file *filp){
    struct vm_object *obj = filp ? filp->dentry->dir_node->mapping : NULL;
    if(!obj){
        obj = kmalloc(sizeof(*obj), 0);
        if(!obj) return NULL;
        obj->refs = 0;
        obj->filp = filp;
        ListInit(&obj->pages);
        ListInit(&obj->mappings);
        if(filp){
            file_get(filp);
            filp->dentry->dir_node->mapping = obj;
        }
    }else if((filp->mode & O_ACCMODE) == O_RDWR && (obj->filp->mode & O_ACCMODE) != O_RDWR){
        file_get(filp);
        file_put(obj->filp);
        obj->filp = filp;
    }
    obj->refs++;
    return obj;
}

static struct cached_page *find_page(struct vm_object *obj, unsigned long offset){
    if(!obj) return NULL;
    for(struct List *p = obj->pages.next; p != &obj->pages; p = p->next){
        struct cached_page *cached = ContainerOf(p, struct cached_page, list);
        if(cached->offset == offset) return cached;
    }
    return NULL;
}

int vm_backing_page(vm_area_struct *vma, unsigned long addr, int writing, struct Page **out){
    struct vm_object *obj = vma->object;
    unsigned long offset = vma->file_offset + addr - vma->start;
    file *filp = obj->filp;
    if(filp && offset >= filp->dentry->dir_node->file_size) return -EFAULT;
    struct cached_page *cached = find_page(obj, offset);
    if(!cached){
        cached = kmalloc(sizeof(*cached), 0);
        if(!cached) return -ENOMEM;
        cached->page = AllocPage4K(ZONE_NORMAL_INDEX, 1, 0);
        if(!cached->page){ kfree(cached); return -ENOMEM; }
        cached->offset = offset;
        cached->dirty = 0;
        char *buf = (char *)PHY_TO_VIRT(cached->page->PhyAddr);
        memset(buf, 0, PAGE_4K_SIZE);
        if(filp){
            unsigned long bytes = min(PAGE_4K_SIZE, filp->dentry->dir_node->file_size - offset);
            long pos = offset;
            int read = filp->f_ops->read(filp, buf, bytes, &pos);
            if(read != (int)bytes){
                page_put(cached->page);
                kfree(cached);
                return read < 0 ? read : -EIO;
            }
        }
        ListForeAdd(&cached->list, &obj->pages);
    }
    if(filp && writing && (vma->mmap_flags & MAP_SHARED)) cached->dirty = 1;
    page_get(cached->page);
    *out = cached->page;
    return 0;
}

static int flush_object(struct vm_object *obj, unsigned long start, unsigned long end){
    if(!obj || !obj->filp) return 0;
    file *filp = obj->filp;
    unsigned long size = filp->dentry->dir_node->file_size;
    for(struct List *p = obj->pages.next; p != &obj->pages; p = p->next){
        struct cached_page *cached = ContainerOf(p, struct cached_page, list);
        if(!cached->dirty || cached->offset < start || cached->offset >= end ||
           cached->offset >= size) continue;
        /* Re-arm first-write faults in every shared alias before I/O sleeps.
         * A subsequent writer waits for the VM lock, then dirties it again. */
        for(struct List *link = obj->mappings.next; link != &obj->mappings; link = link->next){
            vm_area_struct *v = ContainerOf(link, vm_area_struct, object_link);
            if(!(v->mmap_flags & MAP_SHARED) || cached->offset < v->file_offset) continue;
            unsigned long delta = cached->offset - v->file_offset;
            if(delta < v->end - v->start) vm_writeprotect(v->vm_mm, v->start + delta, cached->page);
        }
        unsigned long bytes = min(PAGE_4K_SIZE, size - cached->offset);
        long pos = cached->offset;
        int written = filp->f_ops->write(filp, (char *)PHY_TO_VIRT(cached->page->PhyAddr), bytes, &pos);
        if(written != (int)bytes) return written < 0 ? written : -EIO;
        cached->dirty = 0;
    }
    return 0;
}

static void discard_pages(struct vm_object *obj){
    while(!ListIsEmpty(&obj->pages)){
        struct cached_page *p = ContainerOf(obj->pages.next, struct cached_page, list);
        ListDelete(&p->list);
        page_put(p->page);
        kfree(p);
    }
}

static void put_object(struct vm_object *obj){
    if(!obj || --obj->refs) return;
    /* Keep a failed writeback reachable via the inode for a later retry. */
    if(flush_object(obj, 0, ~0UL)) return;
    discard_pages(obj);
    if(obj->filp){
        obj->filp->dentry->dir_node->mapping = NULL;
        file_put(obj->filp);
    }
    kfree(obj);
}

void vm_object_attach(vm_area_struct *vma){
    if(vma->object){
        vma->object->refs++;
        ListForeAdd(&vma->object_link, &vma->object->mappings);
    }
}

static void trim_object(struct vm_object *obj, unsigned long start, unsigned long end){
    if(!obj) return;
    struct List *p = obj->pages.next;
    while(p != &obj->pages){
        struct cached_page *page = ContainerOf(p, struct cached_page, list);
        p = p->next;
        if(page->offset < start || page->offset >= end) continue;
        struct List *link = obj->mappings.next;
        for(; link != &obj->mappings; link = link->next){
            vm_area_struct *v = ContainerOf(link, vm_area_struct, object_link);
            if(page->offset >= v->file_offset && page->offset - v->file_offset < v->end - v->start)
                break;
        }
        if(link != &obj->mappings) continue;
        if(page->dirty && flush_object(obj, page->offset, page->offset + PAGE_4K_SIZE)) continue;
        ListDelete(&page->list);
        page_put(page->page);
        kfree(page);
    }
}

void vm_object_detach(vm_area_struct *vma){
    if(vma->object){
        ListDelete(&vma->object_link);
        trim_object(vma->object, vma->file_offset, vma->file_offset + (vma->end - vma->start));
        put_object(vma->object);
    }
}

static int valid_range(unsigned long addr, unsigned long len){
    return !(addr & (PAGE_4K_SIZE - 1)) && addr >= PAGE_4K_SIZE &&
           addr < USER_LIMIT && len && len <= USER_LIMIT - addr;
}

static int sync_vma(vm_area_struct *vma, unsigned long start, unsigned long end){
    if(!(vma->mmap_flags & MAP_SHARED)) return 0;
    return flush_object(vma->object, vma->file_offset + (start - vma->start),
                        vma->file_offset + (end - vma->start));
}

/* Reserve split metadata and complete fallible I/O before editing any VMA. */
static int unmap_locked(struct LocalMemManager *mm, unsigned long start, unsigned long end){
    struct List *head = &mm->vma_list;
    struct List *first = head;
    vm_area_struct *right = NULL;
    for(struct List *pos = head->next; pos != head; pos = pos->next){
        vm_area_struct *v = ContainerOf(pos, vm_area_struct, list);
        if(v->end <= start) continue;
        if(v->start >= end) break;
        if(first == head) first = pos;
        int rc = sync_vma(v, max(start, v->start), min(end, v->end));
        if(rc) return rc;
        if(v->start < start && end < v->end){
            right = kmalloc(sizeof(*right), 0);
            if(!right) return -ENOMEM;
            *right = *v;
        }
    }

    vm_unmap_pages(mm, start, end);
    while(first != head){
        vm_area_struct *v = ContainerOf(first, vm_area_struct, list);
        if(v->start >= end) break;
        first = first->next;
        unsigned long from = v->file_offset + (max(start, v->start) - v->start);
        unsigned long to = v->file_offset + (min(end, v->end) - v->start);
        if(v->start < start){
            if(right){
                right->file_offset += end - v->start;
                right->start = end;
                vm_object_attach(right);
                ListBackAdd(&v->list, &right->list);
            }
            v->end = start;
            trim_object(v->object, from, to);
        }else if(end < v->end){
            v->file_offset += end - v->start;
            v->start = end;
            trim_object(v->object, from, to);
        }else{
            ListDelete(&v->list);
            vm_object_detach(v);
            kfree(v);
        }
    }
    return 0;
}

static unsigned long map_locked(struct LocalMemManager *mm, file *filp, unsigned long addr,
                                unsigned long len, unsigned long prot, unsigned long flags,
                                unsigned long offset){
    int type = flags & (MAP_SHARED | MAP_PRIVATE);
    if(!len || len > USER_LIMIT || (offset & (PAGE_4K_SIZE - 1)) ||
       (prot & ~(PROT_READ | PROT_WRITE | PROT_EXEC)) ||
       (flags & ~(MAP_PRIVATE | MAP_SHARED | MAP_FIXED | MAP_ANONYMOUS)) ||
       (type != MAP_PRIVATE && type != MAP_SHARED)) return -EINVAL;
    len = PAGE_4K_ALIGN_UP(len);
    if(offset > (unsigned long)__LONG_MAX__ - len) return -EOVERFLOW;
    if(flags & MAP_ANONYMOUS){ filp = NULL; offset = 0; }
    else{
        if(!filp) return -EBADF;
        if(filp->dentry->dir_node->attribute != FS_ATTR_FILE || !filp->f_ops->read) return -ENODEV;
        unsigned long mode = filp->mode & O_ACCMODE;
        if(mode == O_WONLY || (type == MAP_SHARED && (prot & PROT_WRITE) &&
            (mode != O_RDWR || !filp->f_ops->write))) return -EACCES;
    }

    if(flags & MAP_FIXED){
        if(!valid_range(addr, len)) return -EINVAL;
    }else{
        addr = PAGE_4K_ALIGN_DOWN(addr);
        if(!range_is_free(mm, addr, len)) addr = find_unmapped_area(mm, len);
        if((long)addr < 0) return addr;
    }

    vm_area_struct *v = kmalloc(sizeof(*v), 0);
    if(!v) return -ENOMEM;
    *v = (vm_area_struct){.start = addr, .end = addr + len, .prot = prot,
                          .mmap_flags = flags, .file_offset = offset};
    if(filp || type == MAP_SHARED){
        v->object = get_object(filp);
        if(!v->object){ kfree(v); return -ENOMEM; }
    }
    int rc = 0;
    if(flags & MAP_FIXED) rc = unmap_locked(mm, addr, addr + len);
    if(!rc) rc = insert_vma(mm, v);
    if(rc != 1){
        put_object(v->object);
        kfree(v);
        return rc < 0 ? (unsigned long)rc : (unsigned long)-EINVAL;
    }
    if(v->object) ListForeAdd(&v->object_link, &v->object->mappings);
    return addr;
}

unsigned long vm_map(struct LocalMemManager *mm, file *filp, unsigned long addr,
                     unsigned long len, unsigned long prot, unsigned long flags,
                     unsigned long offset){
    vm_lock();
    unsigned long result = map_locked(mm, filp, addr, len, prot, flags, offset);
    vm_unlock();
    return result;
}

long vm_unmap(struct LocalMemManager *mm, unsigned long addr, unsigned long len){
    if(!valid_range(addr, len)) return -EINVAL;
    vm_lock();
    int rc = unmap_locked(mm, addr, addr + PAGE_4K_ALIGN_UP(len));
    vm_unlock();
    return rc;
}

long vm_sync(struct LocalMemManager *mm, unsigned long addr, unsigned long len){
    if(!valid_range(addr, len)) return -EINVAL;
    unsigned long end = addr + PAGE_4K_ALIGN_UP(len);
    vm_lock();
    int rc = 0;
    while(addr < end){
        vm_area_struct *v = find_vma(mm, addr);
        if(!v || addr < v->start){ rc = -ENOMEM; break; }
        unsigned long next = min(end, v->end);
        rc = sync_vma(v, addr, next);
        if(rc) break;
        addr = next;
    }
    vm_unlock();
    return rc;
}

/* mm for process vma list, addr for new heap end in this brk*/
unsigned long vm_brk(struct LocalMemManager *mm, unsigned long addr){
    vm_lock();
    unsigned long old = mm->EndBrk;
    unsigned long result = old;
    if(!addr) goto out;
    if(addr < mm->StartBrk || addr > USER_LIMIT){ result = -ENOMEM; goto out; }

    unsigned long end = PAGE_4K_ALIGN_UP(addr);
    if(end < old){
        int rc = unmap_locked(mm, end, old);
        if(rc){ result = rc; goto out; }
    }else if(end > old){
        if(!range_is_free(mm, old, end - old)){ result = -ENOMEM; goto out; }
        vm_area_struct *last = find_vma(mm, old - 1);
        if(last && last->end == old && last->flags == VM_HEAP){
            last->end = end;
        }else{
            result = map_locked(mm, NULL, old, end - old, PROT_READ | PROT_WRITE,
                                MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, 0);
            if((long)result < 0) goto out;
            find_vma(mm, old)->flags = VM_HEAP;
        }
    }
    result = mm->EndBrk = end;
out:
    vm_unlock();
    return result;
}

int vm_file_truncate(file *filp){
    index_node *inode = filp->dentry->dir_node;
    if(!inode->inode_ops->truncate) return -ENOSYS;
    vm_lock();
    struct vm_object *obj = inode->mapping;
    int rc = inode->inode_ops->truncate(inode);
    if(obj && !inode->file_size){
        for(struct List *p = obj->mappings.next; p != &obj->mappings; p = p->next){
            vm_area_struct *v = ContainerOf(p, vm_area_struct, object_link);
            vm_unmap_pages(v->vm_mm, v->start, v->end);
        }
        obj->refs++;
        discard_pages(obj);
        put_object(obj);
    }
    vm_unlock();
    return rc;
}

/* Ordinary I/O and mapped I/O use the same contents for a cached file page.
 * The filesystem sees kernel buffers and independent explicit offsets. */
int vm_file_io(file *filp, char *buf, unsigned long count, int write){
    if(!count) return 0;
    if(!verify_area((unsigned char *)buf, count)) return -EFAULT;
    char *scratch = kmalloc(PAGE_4K_SIZE, 0);
    if(!scratch) return -ENOMEM;
    vm_lock();
    index_node *inode = filp->dentry->dir_node;
    unsigned long done = 0;
    if(write && (filp->mode & O_APPEND)) filp->position = inode->file_size;
    int rc = 0;
    count = min(count, (unsigned long)__INT_MAX__);
    while(done < count){
        long pos = filp->position;
        if(pos < 0){ rc = -EINVAL; break; }
        unsigned long offset = (unsigned long)pos & (PAGE_4K_SIZE - 1);
        unsigned long bytes = min(count - done, PAGE_4K_SIZE - offset);
        struct cached_page *p = find_page(inode->mapping, PAGE_4K_ALIGN_DOWN(pos));
        char *cached = p ? (char *)PHY_TO_VIRT(p->page->PhyAddr) + offset : NULL;
        if(write){
            rc = vm_copy_user_locked(buf + done, scratch, bytes, 0);
            if(rc) break;
            rc = filp->f_ops->write(filp, scratch, bytes, &pos);
            if(rc > 0 && cached) memcopy(scratch, cached, rc);
        }else{
            if((unsigned long)pos >= inode->file_size) break;
            bytes = min(bytes, inode->file_size - pos);
            if(cached){ memcopy(cached, scratch, bytes); rc = bytes; pos += bytes; }
            else rc = filp->f_ops->read(filp, scratch, bytes, &pos);
            if(rc > 0){
                int err = vm_copy_user_locked(buf + done, scratch, rc, 1);
                if(err){ rc = err; break; }
            }
        }
        if(rc <= 0) break;
        filp->position = pos;
        done += rc;
        if((unsigned long)rc < bytes) break;
    }
    vm_unlock();
    kfree(scratch);
    return done ? (int)done : rc;
}
