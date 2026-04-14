#include "vfs.h"
#include "printk.h"


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