#include "fat32.h"
#include "disk.h"
#include "lib.h"
#include "printk.h"
#include "memory.h"
#include "vfs.h"

#define CLUS_TO_LBA(FDS, CLUS, SPC) ((FDS) + (((CLUS) - 2) * (SPC)))

super_block_operations FAT32_sb_ops;
dir_entry_operations FAT32_dir_ops;
file_operations FAT32_f_ops;
index_node_operations FAT32_inode_ops;

disk_partition_table dpt;
disk_boot_sector dbs;
FAT32_FSInfo fs_info;

unsigned long fat1_start_sector;
unsigned long fat2_start_sector;
unsigned long fst_data_sector;
unsigned long byte_per_clus;

void ListInit(struct List *list);

void DISK1_FAT32_FS_INIT(){
    unsigned char buf[512];

    memset(buf, 0, 512);
    ide_transfer(ATA_READ_CMD, 0, 1, buf);
    dpt = *(disk_partition_table*)buf;
    ColorPrintfk(BLUE, BLACK, "dpt bs_trailsignature: %x\n", dpt.bs_trailsignature);

    memset(buf, 0, 512);
    ide_transfer(ATA_READ_CMD, dpt.DPTE[0].start_lba, 1, buf);
    dbs = *(disk_boot_sector*)buf;
    ColorPrintfk(BLUE, BLACK, "dbs BS_FileSysType: %d\n", dbs.BPB_SecsPerClus);

    memset(buf, 0, 512);
    ide_transfer(ATA_READ_CMD, dpt.DPTE[0].start_lba + dbs.BPB_FSInfo, 1, buf);
    fs_info = *(FAT32_FSInfo*)buf;
    ColorPrintfk(BLUE, BLACK, "fs_info free_count: %d\n", fs_info.free_count);

    fat1_start_sector   = dpt.DPTE[0].start_lba + dbs.BPB_RsvdSecCnt;
    fat2_start_sector   = fat1_start_sector + dbs.BPB_FATsz32;
    fst_data_sector     = fat1_start_sector + (dbs.BPB_FATsz32 * dbs.BPB_NumFATs);
    byte_per_clus       = dbs.BPB_BytesPerSec * dbs.BPB_SecsPerClus;

    FAT32_Directory entry;
    path_walk(&entry, "/SJKLDJLK/shdadhajskh/SAD/LLLKSNNMM.txt", 0);
    ColorPrintfk(BLUE, BLACK, "entry.dir_name: %s\n", entry.dir_name);
}


super_block * fat32_read_superblock(disk_partition_table_entry *dpte, void *buf){

    super_block * sb = (super_block *)kmalloc(sizeof(super_block), 0);
    sb->sb_ops = &FAT32_sb_ops;

    FAT32_sb_info * private_sb_info = (FAT32_sb_info*)kmalloc(sizeof(FAT32_FSInfo), 0);
    disk_boot_sector *fbs    = (disk_boot_sector*)buf;

    private_sb_info->byte_per_clus          = fbs->BPB_SecsPerClus * fbs->BPB_BytesPerSec;
    private_sb_info->byte_per_sec           = fbs->BPB_BytesPerSec;
    private_sb_info->fat1_start_sector      = dpte->start_lba + fbs->BPB_RsvdSecCnt;
    private_sb_info->fat_num                = fbs->BPB_NumFATs;
    private_sb_info->fst_data_sector        = dpte->start_lba + fbs->BPB_RsvdSecCnt + (fbs->BPB_NumFATs * fbs->BPB_FATsz32);
    private_sb_info->sec_count              = fbs->BPB_TotSec32;
    private_sb_info->sec_per_fat            = fbs->BPB_FATsz32;
    private_sb_info->start_sec              = dpte->start_lba;
    private_sb_info->total_sectors          = dpte->total_sectors;
    private_sb_info->fs_info_flat           = fbs->BPB_FSInfo;
    private_sb_info->bootsector_bk_flat     = fbs->BPB_BKBootSec;

    

    FAT32_FSInfo * fs_info = (FAT32_FSInfo*)kmalloc(sizeof(FAT32_FSInfo), 0);
    ide_transfer(ATA_READ_CMD, dpte->start_lba + fbs->BPB_FSInfo, 1, fs_info);
    private_sb_info->fs_info                = fs_info;

    sb->private_sb_info = (void *)private_sb_info;

    dir_entry * root = (dir_entry *)kmalloc(sizeof(dir_entry), 0);
    root->parent        = NULL;
    root->name          = (char*)kmalloc(2, 0);
    root->name[0]       = '/';
    root->name[1]       = '\0';
    root->name_len      = 1;
    root->dir_ops       = &FAT32_dir_ops;
    ListInit(&root->child_node);
    ListInit(&root->subdirs_list);

    index_node *inode   = (index_node*)kmalloc(sizeof(index_node), 0);

    inode->attribute    = FS_ATTR_DIR;
    inode->blocks       = 0;
    inode->f_ops        = &FAT32_f_ops;
    inode->file_size    = 0;
    inode->inode_ops    = &FAT32_inode_ops;

    FAT32_inode_info * finode = (FAT32_inode_info*)kmalloc(sizeof(FAT32_inode_info), 0);

    finode->create_date = 0;
    finode->create_time = 0;
    finode->write_date  = 0;
    finode->write_time  = 0;
    finode->dentry_location = 0;
    finode->dentry_position = 0;


    inode->private_index_info = finode;
    inode->sb           = sb;

    root->dir_node      = inode;


    sb->root = root;

}

struct file_system_type FAT32_filesystem = {
    .name = "FAT32",
    .next = NULL,
    .fs_flags = 0,
    .read_super_block = fat32_read_superblock,
};

unsigned int DISK1_FAT32_read(unsigned int index){
    unsigned int sec_index     = (index) / (dbs.BPB_BytesPerSec >> 2);
    unsigned int sec_offset    = (index) % (dbs.BPB_BytesPerSec >> 2);
    unsigned int *fat_slace = (unsigned int*)kmalloc(dbs.BPB_BytesPerSec, 0);
    ide_transfer(ATA_READ_CMD, fat1_start_sector + sec_index, 1, (unsigned char *)fat_slace);

    if(fat_slace[sec_offset] < 0xffffff7) {
        kfree(fat_slace);
        return fat_slace[sec_offset];
    }else{
        kfree(fat_slace);
        return 0;
    }
}

void DISK1_FAT32_write(unsigned int index, unsigned int content){
    unsigned int sec_index     = (index) / (dbs.BPB_BytesPerSec >> 2);
    unsigned int sec_offset    = (index) % (dbs.BPB_BytesPerSec >> 2);
    unsigned int *fat_slace = (unsigned int*)kmalloc(dbs.BPB_BytesPerSec, 0);
    ide_transfer(ATA_READ_CMD, fat1_start_sector + sec_index, 1, (unsigned char *)fat_slace);
    fat_slace[sec_offset]   =  content;
    ide_transfer(ATA_WRITE_CMD, fat1_start_sector + sec_index, 1, (unsigned char *)fat_slace);
    ide_transfer(ATA_WRITE_CMD, fat2_start_sector + sec_index, 1, (unsigned char *)fat_slace);

    kfree(fat_slace);
}


void fsde(char *buf, char *name){
    int i = 0, j = 0;
    for(i = 0; i < 11; i++){
        if(name[i] == ' ') continue;
        buf[j++] = name[i];
    }
    buf[j] = '\0';
    upper_case(buf);
}

FAT32_Directory *lookup(char *disk_buf, char *name){

    char lname_buf[256] = {0};
    char sname_buf[12] = {0};
    memset(lname_buf, 0, 256);

    FAT32_LongDirectory *lentry;
    FAT32_Directory *entry = (FAT32_Directory*)disk_buf;

    for(int i = 0; i < byte_per_clus / sizeof(FAT32_Directory); i++){
        if((entry + i)->dir_attr == ATTR_LONG_NAME){
            lentry = (FAT32_LongDirectory *)(entry + i);

            for(int k = 0; k < 5; k++)
            *(lname_buf + ((GetBits(lentry->ldir_ord, 0, 6) - 1) * 13) + k) = (char)(lentry->ldir_name1[k] & 0xff);
            for(int k = 0; k < 6; k++)
            *(lname_buf + ((GetBits(lentry->ldir_ord, 0, 6) - 1) * 13) + k + 5) = (char)(lentry->ldir_name2[k] & 0xff);
            for(int k = 0; k < 2; k++)
            *(lname_buf + ((GetBits(lentry->ldir_ord, 0, 6) - 1) * 13) + k + 11) = (char)(lentry->ldir_name3[k] & 0xff);
            if(GetBits(lentry->ldir_ord, 6, 1))
            lname_buf[((GetBits(lentry->ldir_ord, 0, 6) - 1) * 13) + 13] = '\0';

        }else{
            if(lname_buf[0]){
                if(!strcmp(name, lname_buf)) return (entry + i);
            }else{
                fsde(sname_buf, (entry+i)->dir_name);
                if(!strcmp(name, sname_buf)) return (entry + i);
            }
            memset(lname_buf, 0, 256);
        }
    }
    return NULL; 
}

int path_walk(FAT32_Directory* res, char *path, unsigned long flags){
    char *start = path;
    char *end  = path;
    char *disk_buf = (char *)kmalloc(byte_per_clus, 0);
    unsigned int *fat = (unsigned int *)kmalloc(dbs.BPB_FATsz32 * dbs.BPB_BytesPerSec, 0);
    ide_transfer(ATA_READ_CMD, fat1_start_sector, dbs.BPB_FATsz32, (unsigned char*)fat);

    char name[256] = {0};
    unsigned long clus = dbs.BPB_RootClus;
    unsigned long lba = CLUS_TO_LBA(fst_data_sector, clus, dbs.BPB_SecsPerClus);

    while(*end){
        start = end;
        while(*start == '/') start++;
        end = start;
        while(*end != '/' && *end != '\0') end++;
        memcopy(start, name, (end - start));
        name[end - start] = '\0';

        ide_transfer(ATA_READ_CMD, lba, dbs.BPB_SecsPerClus, (unsigned char*)disk_buf);
        FAT32_Directory* entry = lookup(disk_buf, name);
        if(entry){
            if(entry->dir_attr & ATTR_DIRECTORY){
                clus    = (entry->dir_fst_clus_hi << 16) + entry->dir_fst_clus_lo;
                lba     = CLUS_TO_LBA(fst_data_sector, clus, dbs.BPB_SecsPerClus);
            }else{
                memcopy(entry, res, sizeof(FAT32_Directory));
                kfree(fat);
                kfree(disk_buf);
                return 1;
            }
        }else{
            clus    = fat[clus];
            if(clus >= 0xfffff7) break;
            lba     = CLUS_TO_LBA(fst_data_sector, clus, dbs.BPB_SecsPerClus);
        }
    }

    kfree(fat);
    kfree(disk_buf);
    return NULL;
}