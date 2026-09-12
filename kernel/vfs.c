#include "vfs.h"
#include "memory.h"
#include "printk.h"

super_block * fsb;

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

        if(end == start) return parent;
        
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

        ListInit(&c_dir->child_node);
        ListInit(&c_dir->subdirs_list);
        c_dir->parent = parent;
        ListBackAdd(&parent->subdirs_list, &c_dir->child_node);
        parent = c_dir;
    }

    return c_dir;
}
