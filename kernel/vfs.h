#ifndef VFS_H
#define VFS_H


#include "lib.h"
#include "fat32.h"

#define FS_ATTR_DIR 0

typedef struct index_node index_node;
typedef struct super_block super_block;
typedef struct dir_entry dir_entry;
typedef struct file file;
typedef struct super_block_operations super_block_operations;

typedef struct file_operations file_operations;

typedef struct index_node_operations index_node_operations;

typedef struct dir_entry_operations dir_entry_operations;
typedef struct file_system_type file_system_type;


typedef struct file_system_type{
    char * name;
    int fs_flags;
    super_block * (*read_super_block)(disk_partition_table_entry * dpte, void * buf);
    file_system_type * next;

}file_system_type;

typedef struct super_block{
    dir_entry * root;
    super_block_operations * sb_ops;
    void * private_sb_info;

}super_block;


typedef struct index_node{
    unsigned long file_size;
    unsigned long blocks;
    unsigned long attribute;

    super_block * sb;
    file_operations * f_ops;
    index_node_operations * inode_ops;
    void * private_index_info;
    
}index_node;

typedef struct dir_entry{
    
    char *name;
    int name_len;
    struct List child_node;
    struct List subdirs_list;
    index_node * dir_node;
    dir_entry * parent;
    dir_entry_operations * dir_ops;

}dir_entry;

typedef struct file{
    long position;
    unsigned long mode;
    dir_entry * dentry;
    file_operations * f_ops;
    void * private_data;

}file;

typedef struct super_block_operations{
    void (*write_sb)(super_block * sb);
    void (*read)(super_block * sb);
    void (*write_inode)(index_node * inode);
}super_block_operations;

typedef struct index_node_operations{

    int (*create)(index_node * inode, dir_entry * entry, int mode);
    dir_entry* (*lookup)(index_node * inode, dir_entry * entry);
    int (*mkdir)(index_node * inode, dir_entry * entrym, int mode);
    int (*rmdir)(index_node * inode, dir_entry * dentry);
    int (*rename)(index_node * old_node, dir_entry * old_entry, index_node * new_node, dir_entry * new_entry);
    int (*getattr)(dir_entry * dentry);
    int (*setattr)(dir_entry * dentry, unsigned long * attr);

}index_node_operations;

typedef struct dir_entry_operations{
    int (*compare)(dir_entry * pdentry, char * source_filename, char * dest_filename);
    int (*hash)(dir_entry * dentry, char *filename);
    int (*release)(dir_entry * dentry);
    int (*iput)(dir_entry * dentry, index_node * inode);

}dir_entry_operations;


typedef struct file_operations{
    int (*open)(index_node * inode, file * filp);
    int (*read)(index_node * inode, file * filp);
    int (*write)(file * filp, char * buf, unsigned long count, long * position);
    int (*close)(file * filp, char * buf, unsigned long count, long * position);
    int (*lseek)(file * filp, long offset, long origin);
    int (*ioctl)(index_node * inode, file * filp, unsigned long cmd, unsigned long arg);

}file_operations;

#endif