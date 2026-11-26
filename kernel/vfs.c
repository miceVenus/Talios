#include "vfs.h"
#include "memory.h"
#include "printk.h"

super_block * fsb;
static SpinLock_T dentry_lock;

file_system_type filesystem = {.name = "filesystem", 0};

super_block * mount_fs(char * name, disk_partition_table_entry * dpte, void * buf){

    for(file_system_type *p = &filesystem; p; p = p->next){
        if(!strcmp(name, p->name)){
            return p->read_super_block(dpte, buf);
        }
    }
    ColorPrintfk(RED, BLACK, "unmatched filesystem : %s \n", name);
    return 0;
}

int register_filesystem(file_system_type * fs){
    for(file_system_type * p = &filesystem; p; p = p->next){
        if(!strcmp(p->name, fs->name))
            return 0;
    }

    fs->next = filesystem.next;
    filesystem.next = fs;

    return 1;
}


dir_entry* path_walk(char *path, unsigned long flags){
    char *start = path;
    char *end  = path;
    dir_entry * parent = fsb->root;
    dir_entry * c_dir   = NULL;

    char name[256] = {0};
    // unsigned long lba = CLUS_TO_LBA(sb_info->fst_data_sector, clus, sb_info->sec_per_clus);

    while(*end){
        start = end;
        while(*start == '/') start++;
        end = start;
        while(*end != '/' && *end != '\0') end++;

        if(parent->dir_node->attribute != FS_ATTR_DIR) return NULL;
        if(end == start) return parent;
        if(end - start == 1 && start[0] == '.') continue;
        if(end - start == 2 && start[0] == '.' && start[1] == '.'){
            if(parent->parent) parent = parent->parent;
            continue;
        }
        
        if(end - start >= (long)sizeof(name)) return NULL;
        memcopy(start, name, (end - start));
        name[end - start] = '\0';
        c_dir = (dir_entry*)kmalloc(sizeof(dir_entry), 0);
        if(c_dir == NULL) return NULL;
        c_dir->name_len = end - start;
        c_dir->name = (char*)kmalloc(c_dir->name_len + 1, 0);
        if(c_dir->name == NULL){
            kfree(c_dir);
            return NULL;
        }
        memcopy(start, c_dir->name, c_dir->name_len);
        c_dir->name[c_dir->name_len] = '\0';

        if(parent->dir_node->inode_ops->lookup(parent->dir_node, c_dir) == NULL){
            kfree(c_dir->name);
            kfree(c_dir);
            return NULL;
        }

        /* FAT short and long names can resolve to the same directory slot.
         * Share that inode (and hence its mmap cache) across independent opens. */
        unsigned long irq = spin_lock_irqsave(&dentry_lock);
        dir_entry *known = NULL;
        for(struct List *p = parent->subdirs_list.next; p != &parent->subdirs_list; p = p->next){
            dir_entry *d = ContainerOf(p, dir_entry, child_node);
            if(d->dir_node->ino == c_dir->dir_node->ino){ known = d; break; }
        }
        if(known){
            spin_unlock_irqrestore(&dentry_lock, irq);
            kfree(c_dir->dir_node->private_index_info);
            kfree(c_dir->dir_node);
            kfree(c_dir->name);
            kfree(c_dir);
            parent = c_dir = known;
            continue;
        }
        ListInit(&c_dir->child_node);
        ListInit(&c_dir->subdirs_list);
        c_dir->parent = parent;
        ListBackAdd(&parent->subdirs_list, &c_dir->child_node);
        parent = c_dir;
        spin_unlock_irqrestore(&dentry_lock, irq);
    }

    return *path ? parent : NULL;
}


void file_get(file *filp){
    __atomic_fetch_add(&filp->refs, 1, __ATOMIC_RELAXED);
}

int file_put(file *filp){
    unsigned long refs = __atomic_sub_fetch(&filp->refs, 1, __ATOMIC_ACQ_REL);
    if(refs) return 0;
    int rc = 0;
    if(filp->f_ops && filp->f_ops->close) rc = filp->f_ops->close(filp->dentry->dir_node, filp);
    kfree(filp);
    return rc < 0 ? rc : 0;
}
