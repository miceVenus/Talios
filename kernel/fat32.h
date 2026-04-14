#ifndef FAT32_H
#define FAT32_H


#define ATTR_READ_ONLY  (1 << 0)
#define ATTR_HIDDEN     (1 << 1)
#define ATTR_SYSTEM     (1 << 2)
#define ATTR_VOLUME_ID  (1 << 3)
#define ATTR_DIRECTORY  (1 << 4)
#define ATTR_ARCHIVE    (1 << 5)
#define ATTR_LONG_NAME  0x0f

#include "vfs.h"
#include "disk.h"

typedef struct FAT32_FSInfo{
    unsigned char lead_signature[4];    // "RRaA"
    unsigned char reserved[480];        
    unsigned char struct_signature[4];  // "rrAa"
    unsigned int free_count;            // bad value : 0xffffffff not automatically
    unsigned int next_free;
    unsigned char reserved1[12];
    unsigned short FSI_trailsignature;  // 0xaa55

}__attribute__((packed))FAT32_FSInfo;

typedef struct FAT32_Directory{
    unsigned char dir_name[11];
    unsigned char dir_attr;
    unsigned char dir_ntres;            // NT reserved filed
    unsigned char dir_crt_time_tenth;   // create time high precision extension
    unsigned short dir_crt_time;
    unsigned short dir_crt_Date;
    unsigned short dir_last_acc_Date;
    unsigned short dir_fst_clus_hi;
    unsigned short dir_wrt_time;
    unsigned short dir_wrt_date;
    unsigned short dir_fst_clus_lo;
    unsigned int   dir_file_size;

}__attribute__((packed))FAT32_Directory;



typedef struct FAT32_LongDirectory{

    unsigned char ldir_ord;         // long directory index num if bit 6 set 1 this is last in logically
    unsigned short ldir_name1[5];
    unsigned char ldir_attr;        //fixed 0x0f for compatibility in some old system
    unsigned char ldir_type;
    unsigned char ldir_chksum;
    unsigned short ldir_name2[6];
    unsigned short ldir_fst_clus_lo;
    unsigned short ldir_name3[2];
}__attribute__((packed))FAT32_LongDirectory;

typedef struct FAT32_sb_info{

    unsigned int    start_sec;
    unsigned int    total_sectors;

    unsigned long sec_count;
    unsigned short byte_per_sec;
    unsigned long fat_num;
    unsigned long fat1_start_sector;
    unsigned long fst_data_sector;
    unsigned long byte_per_clus;
    unsigned long sec_per_fat;
    unsigned long sec_per_clus;

    unsigned long fs_info_flat;
    unsigned long bootsector_bk_flat;

    FAT32_FSInfo * fs_info;
}FAT32_sb_info;


typedef struct FAT32_inode_info{

    unsigned long   first_cluster;
    unsigned long   dentry_location;

    unsigned long   dentry_position;

    unsigned short  create_date;
    unsigned short  create_time;
    unsigned short  write_time;
    unsigned short  write_date;

}FAT32_inode_info;

void DISK1_FAT32_FS_INIT();
dir_entry * path_walk(char *path, unsigned long flags);
super_block * fat32_read_superblock(disk_partition_table_entry *dpte, void *buf);

// dentry op
int fat32_create(index_node * inode, dir_entry * dir, int mode);
int fat32_mkdir(index_node * inode, dir_entry * dir, int mode);
int fat32_rmdir(index_node * inode, dir_entry * dir);
int fat32_rename(index_node * old_inode, dir_entry * old_dentry, index_node * new_inode, dir_entry * new_dentry);
int fat32_getattr(dir_entry * dir, unsigned long * attr);
int fat32_setattr(dir_entry * dir, unsigned long * attr);


// inode op
dir_entry * fat32_lookup(index_node* parent_inode, dir_entry * dir);
int fat32_compare(dir_entry * pdentry, char * source_filename, char * dest_filename);
int fat32_hash(dir_entry * dentry, char *filename);
int fat32_release(dir_entry * dentry);
int fat32_iput(dir_entry * dentry, index_node * inode);

// file op
int fat32_open(index_node * inode, file * filp);
int fat32_read(index_node * inode, file * filp);
int fat32_write(file * filp, char * buf, unsigned long count, long * position);
int fat32_close(file * filp, char * buf, unsigned long count, long * position);
int fat32_lseek(file * filp, long offset, long origin);
int fat32_ioctl(index_node * inode, file * filp, unsigned long cmd, unsigned long arg);

// super block op
void fat32_write_superblock(super_block * lsb);
void fat32_put_superblock(super_block * lsb);
void fat32_write_inode(index_node * inode);

#endif