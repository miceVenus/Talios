#include "fat32.h"
#include "disk.h"
#include "lib.h"
#include "printk.h"
#include "memory.h"
#include "errno.h"

#define CLUS_TO_LBA(FDS, CLUS, SPC) ((FDS) + (((CLUS) - 2) * (SPC)))

super_block_operations FAT32_sb_ops = {
    .put_sb = fat32_put_superblock,
    .write_sb = fat32_write_superblock,
    .write_inode = fat32_write_inode,
};
dir_entry_operations FAT32_dir_ops = {
    .compare = fat32_compare,
    .hash    = fat32_hash,
    .iput    = fat32_iput,
    .release = fat32_release,
};
file_operations FAT32_f_ops = {
    .close  = fat32_close,
    .ioctl  = fat32_ioctl,
    .lseek  = fat32_lseek,
    .open   = fat32_open,
    .read   = fat32_read,
    .write  = fat32_write,
};
index_node_operations FAT32_inode_ops = {
    .create     = &fat32_create,
    .lookup     = &fat32_lookup,
    .mkdir      = &fat32_mkdir,
    .rmdir      = &fat32_rmdir,
    .setattr    = &fat32_setattr,
    .getattr    = &fat32_getattr,
    .rename     = &fat32_rename,
};

// disk_partition_table dpt;
// disk_boot_sector dbs;
// FAT32_FSInfo fs_info;

// unsigned long fat1_start_sector;
// unsigned long fat2_start_sector;
// unsigned long fst_data_sector;
// unsigned long byte_per_clus;

extern super_block *fsb;

void ListInit(struct List *list);


struct file_system_type FAT32_filesystem = {
    .name = "FAT32",
    .next = NULL,
    .fs_flags = 0,
    .read_super_block = fat32_read_superblock,
};


void fsde(char *buf, char *name){
    int i = 0, j = 0;
    for(i = 0; i < 11; i++){
        if(name[i] == ' ') continue;
        buf[j++] = name[i];
    }
    buf[j] = '\0';
    upper_case(buf);
}

unsigned int fat32_table_read(FAT32_sb_info *fsb, unsigned int index){
    unsigned int sec_index     = (index) / (fsb->byte_per_sec >> 2);
    unsigned int sec_offset    = (index) % (fsb->byte_per_sec >> 2);
    unsigned int *fat_slace = (unsigned int*)kmalloc(fsb->byte_per_sec, 0);
    ide_transfer(ATA_READ_CMD, fsb->fat1_start_sector + sec_index, 1, (unsigned char *)fat_slace);

    if(fat_slace[sec_offset] < 0xffffff7) {
        kfree(fat_slace);
        return fat_slace[sec_offset];
    }else{
        kfree(fat_slace);
        return 0;
    }
}

void fat32_table_write(FAT32_sb_info *fsb, unsigned int index, unsigned int content){
    unsigned int sec_index     = (index) / (fsb->byte_per_sec >> 2);
    unsigned int sec_offset    = (index) % (fsb->byte_per_sec >> 2);
    unsigned int *fat_slace = (unsigned int*)kmalloc(fsb->byte_per_sec, 0);
    ide_transfer(ATA_READ_CMD, fsb->fat1_start_sector + sec_index, 1, (unsigned char *)fat_slace);
    fat_slace[sec_offset]   =  content;
    ide_transfer(ATA_WRITE_CMD, fsb->fat1_start_sector + sec_index, 1, (unsigned char *)fat_slace);
    ide_transfer(ATA_WRITE_CMD, fsb->fat1_start_sector + fsb->sec_per_fat + sec_index, 1, (unsigned char *)fat_slace);

    kfree(fat_slace);
}


int fat32_create(index_node * inode, dir_entry * dir, int mode){}
int fat32_mkdir(index_node * inode, dir_entry * dir, int mode){}
int fat32_rmdir(index_node * inode, dir_entry * dir){}
int fat32_rename(index_node * old_inode, dir_entry * old_dentry, index_node * new_inode, dir_entry * new_dentry){}
int fat32_getattr(dir_entry * dir, unsigned long * attr){}
int fat32_setattr(dir_entry * dir, unsigned long * attr){}

dir_entry* fat32_lookup(index_node* parent_inode, dir_entry * dir){

    unsigned long lba;
    unsigned long clus;
    char lname_buf[256] = {0};
    char sname_buf[12] = {0};
    memset(lname_buf, 0, 256);

    FAT32_LongDirectory *lentry;
    FAT32_Directory *entry;
    FAT32_inode_info * finode = (FAT32_inode_info*)parent_inode->private_index_info;
    FAT32_sb_info * fsb_info = parent_inode->sb->private_sb_info;

    char * disk_buf = kmalloc(fsb_info->byte_per_clus, 0);

    clus = finode->first_cluster;

    while(clus != 0){
        lba = CLUS_TO_LBA(fsb_info->fst_data_sector, finode->first_cluster, fsb_info->sec_per_clus);
        ide_transfer(ATA_READ_CMD, lba, fsb_info->sec_per_clus, (unsigned char*)disk_buf);
        entry = (FAT32_Directory*)disk_buf;

        for(unsigned long i = 0; i < fsb_info->byte_per_clus / sizeof(FAT32_Directory); i++){
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
                    if(!strcmp(dir->name, lname_buf)) goto found_target_dir;
                }else{
                    fsde(sname_buf, (entry+i)->dir_name);
                    if(!strcmp(dir->name, sname_buf)) goto found_target_dir;
                }
                memset(lname_buf, 0, 256);
            }

            continue;

            found_target_dir:{
                index_node * tnode = (index_node *)kmalloc(sizeof(index_node), 0);
                memset(tnode, sizeof(index_node), 0);
                tnode->f_ops = &FAT32_f_ops;
                tnode->inode_ops = &FAT32_inode_ops;
                tnode->sb = fsb;
                tnode->file_size = (entry + i)->dir_file_size;
                tnode->blocks = (tnode->file_size + fsb_info->byte_per_clus - 1) >> BYTE_PER_VSEC_SHIFT;
                FAT32_inode_info* tinfo = (FAT32_inode_info*)kmalloc(sizeof(FAT32_inode_info), 0);
                memset(tnode, sizeof(FAT32_inode_info), 0);
                tinfo->first_cluster = ((entry + i)->dir_fst_clus_hi << 16) + (entry + i)->dir_fst_clus_lo;
                tinfo->create_date = (entry + i)->dir_crt_Date;
                tinfo->create_time = (entry + i)->dir_crt_time;
                tinfo->dentry_location = clus;
                tinfo->dentry_position = (entry + i) - entry;
                tinfo->write_date = (entry + i)->dir_wrt_date;
                tinfo->write_time = (entry + i)->dir_wrt_time;
                tnode->private_index_info = (void *)tinfo;
                dir->dir_node = tnode;
                dir->dir_ops = &FAT32_dir_ops;
                kfree(disk_buf);

                return dir;
            }

        }

        clus = fat32_table_read(fsb_info, clus);
    }

    kfree(disk_buf);
    return NULL; 
}


int fat32_compare(dir_entry * pdentry, char * source_filename, char * dest_filename){}
int fat32_hash(dir_entry * dentry, char *filename){}
int fat32_release(dir_entry * dentry){}
int fat32_iput(dir_entry * dentry, index_node * inode){}

int fat32_open(index_node * inode, file * filp){
    return 1;
}
int fat32_read(file * filp, char * buf, unsigned long count, long * position){
    long errno;
    unsigned long remainder;
    unsigned long lba;
    int ret_val;
    unsigned long length;
    FAT32_inode_info * inode_info = filp->dentry->dir_node->private_index_info;
    FAT32_sb_info * sb_info = filp->dentry->dir_node->sb->private_sb_info;
    unsigned long byte_per_clus = sb_info->byte_per_clus;
    unsigned long offset = *position % byte_per_clus;
    unsigned long index = 0;
    unsigned long clus = inode_info->first_cluster + (*position / byte_per_clus);

    if(*position + count > filp->dentry->dir_node->file_size)
        remainder = filp->dentry->dir_node->file_size - *position;
    else
        remainder = count;

    unsigned char * buffer = kmalloc(byte_per_clus, 0);

    do{
        lba = CLUS_TO_LBA(sb_info->fst_data_sector, clus, sb_info->sec_per_clus);
        errno = ide_transfer(ATA_READ_CMD, lba, sb_info->sec_per_clus, buffer);
        if(!errno){
            ret_val = -EIO;
            break;
        }
        
        length = remainder < byte_per_clus ? remainder : byte_per_clus;
        length = offset ? byte_per_clus - offset : length;

        if(buf + offset < TASK_SIZE)
            copy_to_user(buffer + offset, buf + index, length);
        else
            memcopy(buffer + offset, buf + index, length);

        remainder -= length;
        index += length;
        offset = 0;
        clus = fat32_table_read(sb_info, clus);
    }while(remainder && clus);

    *position += index;

    kfree(buffer);
    if(!remainder){
        ret_val = index;
    }
    return ret_val;
}

int fat32_write(file * filp, char * buf, unsigned long count, long * position){}
int fat32_close(index_node * inode, file * filp){}
int fat32_lseek(file * filp, long offset, long origin){}
int fat32_ioctl(index_node * inode, file * filp, unsigned long cmd, unsigned long arg){}

super_block * fat32_read_superblock(disk_partition_table_entry *dpte, void *buf){

    fsb = (super_block *)kmalloc(sizeof(super_block), 0);
    fsb->sb_ops = &FAT32_sb_ops;

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
    private_sb_info->sec_per_clus           = fbs->BPB_SecsPerClus;

    

    FAT32_FSInfo * fs_info = (FAT32_FSInfo*)kmalloc(sizeof(FAT32_FSInfo), 0);
    ide_transfer(ATA_READ_CMD, dpte->start_lba + fbs->BPB_FSInfo, 1, (unsigned char *)fs_info);
    private_sb_info->fs_info                = fs_info;

    fsb->private_sb_info = (void *)private_sb_info;

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
    finode->first_cluster = fbs->BPB_RootClus;


    inode->private_index_info = finode;
    inode->sb           = fsb;

    root->dir_node      = inode;


    fsb->root = root;

    return fsb;
}

void fat32_write_superblock(super_block * lsb){

}
void fat32_put_superblock(super_block * lsb){
    kfree(lsb->private_sb_info);
    kfree(lsb->root->name);
    kfree(lsb->root->dir_node);
    kfree(lsb->root);
    kfree(lsb);
}

void fat32_write_inode(index_node * inode){
    FAT32_inode_info * inode_info = (FAT32_inode_info *)inode->private_index_info;
    FAT32_sb_info * sb_info = fsb->private_sb_info;
    FAT32_Directory * buf = kmalloc(sizeof(sb_info->byte_per_clus), 0);
    ide_transfer(ATA_READ_CMD, CLUS_TO_LBA(sb_info->fst_data_sector, inode_info->dentry_position, sb_info->sec_per_clus), sb_info->sec_per_clus, (unsigned char *)buf);
    FAT32_Directory * tentry = (FAT32_Directory *)((char *)buf + inode_info->dentry_location);
    tentry->dir_file_size = inode->file_size;
    tentry->dir_fst_clus_hi = GetBits(inode_info->first_cluster, 16, 16);
    tentry->dir_fst_clus_lo = GetBits(inode_info->first_cluster, 0, 16);
    tentry->dir_wrt_date = inode_info->write_date;
    tentry->dir_wrt_time = inode_info->write_time;

    ide_transfer(ATA_WRITE_CMD, CLUS_TO_LBA(sb_info->fst_data_sector, inode_info->dentry_position, sb_info->sec_per_clus), sb_info->sec_per_clus, (unsigned char *)buf);

    kfree(buf);
}


void DISK1_FAT32_FS_INIT(){

    
    unsigned char buf[512] = {0};

    ide_transfer(ATA_READ_CMD, 0, 1, buf);
    register_filesystem(&FAT32_filesystem);
    mount_fs(FAT32_filesystem.name, &((disk_partition_table*)buf)->DPTE[0], buf);
 
    // dir_entry * dentry = path_walk("/SJKLDJLK/shdadhajskh/SAD/LLLKSNNMM.txt", 0);
    // ColorPrintfk(BLUE, BLACK, "entry.dir_name: %s\n", dentry->name);

    // ide_transfer(ATA_READ_CMD, 0, 1, buf);
    // dpt = *(disk_partition_table*)buf;
    // ColorPrintfk(BLUE, BLACK, "dpt bs_trailsignature: %x\n", dpt.bs_trailsignature);

    // memset(buf, 0, 512);
    // ide_transfer(ATA_READ_CMD, dpt.DPTE[0].start_lba, 1, buf);
    // dbs = *(disk_boot_sector*)buf;
    // ColorPrintfk(BLUE, BLACK, "dbs BS_FileSysType: %d\n", dbs.BPB_SecsPerClus);

    // memset(buf, 0, 512);
    // ide_transfer(ATA_READ_CMD, dpt.DPTE[0].start_lba + dbs.BPB_FSInfo, 1, buf);
    // fs_info = *(FAT32_FSInfo*)buf;
    // ColorPrintfk(BLUE, BLACK, "fs_info free_count: %d\n", fs_info.free_count);

    // fat1_start_sector   = dpt.DPTE[0].start_lba + dbs.BPB_RsvdSecCnt;
    // fat2_start_sector   = fat1_start_sector + dbs.BPB_FATsz32;
    // fst_data_sector     = fat1_start_sector + (dbs.BPB_FATsz32 * dbs.BPB_NumFATs);
    // byte_per_clus       = dbs.BPB_BytesPerSec * dbs.BPB_SecsPerClus;

    // FAT32_Directory entry;
    // path_walk(&entry, "/SJKLDJLK/shdadhajskh/SAD/LLLKSNNMM.txt", 0);
    // ColorPrintfk(BLUE, BLACK, "entry.dir_name: %s\n", entry.dir_name);
}