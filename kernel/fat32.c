#include "fat32.h"
#include "disk.h"
#include "lib.h"
#include "printk.h"
#include "memory.h"
#include "errno.h"
#include "stdio.h"

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
    .readdir= fat32_readdir,
};
index_node_operations FAT32_inode_ops = {
    .create     = &fat32_create,
    .truncate   = &fat32_truncate,
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


static void short_filename(const FAT32_Directory *e, char *out){
    unsigned int n = 0;
    for(int i = 0; i < 11; i++){
        unsigned char c = e->dir_name[i];
        if(c == ' ') continue;
        if(i == 8) out[n++] = '.';
        if((e->dir_ntres & (i < 8 ? 8 : 16)) && c >= 'A' && c <= 'Z') c += 'a' - 'A';
        out[n++] = c;
    }
    out[n] = 0;
}

static int filename_equal(const char *a, const char *b){
    while(*a && *b){
        unsigned char x = *a++, y = *b++;
        if(x >= 'A' && x <= 'Z') x += 'a' - 'A';
        if(y >= 'A' && y <= 'Z') y += 'a' - 'A';
        if(x != y) return 0;
    }
    return *a == *b;
}

static void long_filename_part(char name[256], const FAT32_LongDirectory *e){
    unsigned int order = e->ldir_ord & 0x3f;
    if(e->ldir_ord & 0x40) memset(name, 0, 256);
    if(!order || order > 20) return;
    unsigned int pos = (order - 1) * 13;
    for(int i = 0; i < 13 && pos < 255; i++, pos++){
        unsigned int ch = i < 5 ? e->ldir_name1[i] :
                          i < 11 ? e->ldir_name2[i - 5] : e->ldir_name3[i - 11];
        name[pos] = ch == 0xffff ? 0 : (char)ch;
    }
    name[255] = 0;
}

static unsigned long cluster_limit(FAT32_sb_info *sb){
    return 2 + (sb->sec_count - (sb->fst_data_sector - sb->start_sec)) / sb->sec_per_clus;
}

static long find_free_cluster(FAT32_sb_info *sb){
    unsigned int *entries = kmalloc(sb->byte_per_sec, 0);
    if(!entries) return -ENOMEM;
    unsigned long per_sector = sb->byte_per_sec / sizeof(*entries);
    unsigned long limit = cluster_limit(sb);
    long result = -ENOSPC;
    for(unsigned long i = 0; i < sb->sec_per_fat && i * per_sector < limit; i++){
        if(!ide_transfer(ATA_READ_CMD, sb->fat1_start_sector + i, 1, (unsigned char *)entries)){
            result = -EIO;
            break;
        }
        for(unsigned long j = i ? 0 : 2; j < per_sector && i * per_sector + j < limit; j++){
            if(!(entries[j] & 0x0fffffff)){
                result = i * per_sector + j;
                goto out;
            }
        }
    }
out:
    kfree(entries);
    return result;
}

/* Return zero for the end of a chain and propagate media errors to callers. */
static long next_cluster(FAT32_sb_info *sb, unsigned long cluster){
    if(cluster < 2 || cluster >= cluster_limit(sb)) return -EIO;
    unsigned int *entries = kmalloc(sb->byte_per_sec, 0);
    if(!entries) return -ENOMEM;
    unsigned long per_sector = sb->byte_per_sec / sizeof(*entries);
    long next = -EIO;
    if(ide_transfer(ATA_READ_CMD, sb->fat1_start_sector + cluster / per_sector, 1, (unsigned char *)entries)){
        next = entries[cluster % per_sector] & 0x0fffffff;
        if(next >= 0x0ffffff8) next = 0;
        else if(next < 2 || (unsigned long)next >= cluster_limit(sb)) next = -EIO;
    }
    kfree(entries);
    return next;
}

static int set_cluster(FAT32_sb_info *sb, unsigned long cluster, unsigned int value){
    unsigned int *entries = kmalloc(sb->byte_per_sec, 0);
    if(!entries) return -ENOMEM;
    unsigned long per_sector = sb->byte_per_sec / sizeof(*entries);
    unsigned long sector = sb->fat1_start_sector + cluster / per_sector;
    int ok = ide_transfer(ATA_READ_CMD, sector, 1, (unsigned char *)entries);
    if(ok){
        unsigned int *entry = &entries[cluster % per_sector];
        *entry = (*entry & 0xf0000000) | value;
        /* Publish the link in the primary table after writing its mirrors. */
        for(unsigned long copy = sb->fat_num; copy && ok; copy--)
            ok = ide_transfer(ATA_WRITE_CMD, sector + (copy - 1) * sb->sec_per_fat, 1, (unsigned char *)entries);
    }
    kfree(entries);
    return ok ? 0 : -EIO;
}


int fat32_create(index_node * inode, dir_entry * dir, int mode){}
int fat32_mkdir(index_node * inode, dir_entry * dir, int mode){}
int fat32_rmdir(index_node * inode, dir_entry * dir){}
int fat32_rename(index_node * old_inode, dir_entry * old_dentry, index_node * new_inode, dir_entry * new_dentry){}
int fat32_getattr(dir_entry * dir, unsigned long * attr){}
int fat32_setattr(dir_entry * dir, unsigned long * attr){}

dir_entry* fat32_lookup(index_node* parent_inode, dir_entry * dir){

    unsigned long lba;
    long clus;
    char lname_buf[256] = {0};
    char sname_buf[13] = {0};
    memset(lname_buf, 0, 256);

    FAT32_Directory *entry;
    FAT32_inode_info * finode = (FAT32_inode_info*)parent_inode->private_index_info;
    FAT32_sb_info * fsb_info = parent_inode->sb->private_sb_info;

    char * disk_buf = kmalloc(fsb_info->byte_per_clus, 0);

    if(!disk_buf) return NULL;
    clus = finode->first_cluster & (~0xf0000000);

    while(clus > 0){
        lba = CLUS_TO_LBA(fsb_info->fst_data_sector, clus, fsb_info->sec_per_clus);
        if(!ide_transfer(ATA_READ_CMD, lba, fsb_info->sec_per_clus, (unsigned char*)disk_buf)){ kfree(disk_buf); return NULL; }
        entry = (FAT32_Directory*)disk_buf;

        for(unsigned long i = 0; i < fsb_info->byte_per_clus / sizeof(FAT32_Directory); i++){
            FAT32_Directory *item = entry + i;
            if(!item->dir_name[0]){ kfree(disk_buf); return NULL; }
            if(item->dir_name[0] == 0xe5){ lname_buf[0] = 0; continue; }
            if(item->dir_attr == ATTR_LONG_NAME){
                long_filename_part(lname_buf, (FAT32_LongDirectory *)item);
                continue;
            }
            short_filename(item, sname_buf);
            if((item->dir_attr & 8) ||
               (!filename_equal(dir->name, lname_buf) && !filename_equal(dir->name, sname_buf))){
                lname_buf[0] = 0;
                continue;
            }
            {
                index_node * tnode = (index_node *)kmalloc(sizeof(index_node), 0);
                if(!tnode){ kfree(disk_buf); return NULL; }
                memset(tnode, 0, sizeof(index_node));
                tnode->ino = (clus << 16) | i;
                tnode->f_ops = &FAT32_f_ops;
                tnode->inode_ops = &FAT32_inode_ops;
                tnode->sb = fsb;
                tnode->file_size = (entry + i)->dir_file_size;
                
                tnode->attribute = ((entry + i)->dir_attr & ATTR_DIRECTORY) ? FS_ATTR_DIR : FS_ATTR_FILE;

                tnode->blocks = (tnode->file_size + fsb_info->byte_per_clus - 1) >> BYTE_PER_VSEC_SHIFT;
                FAT32_inode_info* tinfo = (FAT32_inode_info*)kmalloc(sizeof(FAT32_inode_info), 0);
                if(!tinfo){ kfree(tnode); kfree(disk_buf); return NULL; }
                memset(tinfo, 0, sizeof(*tinfo));
                tinfo->first_cluster = ((entry + i)->dir_fst_clus_hi << 16) + (entry + i)->dir_fst_clus_lo;
                tinfo->create_date = (entry + i)->dir_crt_Date;
                tinfo->create_time = (entry + i)->dir_crt_time;
                tinfo->dentry_location = clus;
                tinfo->dentry_position = i;
                tinfo->write_date = (entry + i)->dir_wrt_date;
                tinfo->write_time = (entry + i)->dir_wrt_time;
                tnode->private_index_info = (void *)tinfo;
                dir->dir_node = tnode;
                dir->dir_ops = &FAT32_dir_ops;
                if((entry + i)->dir_fst_clus_hi >> 12 && (tnode->attribute & FS_ATTR_FILE)){
                    dir->dir_node->attribute |= FS_ATTR_DEVICE;
                }
                kfree(disk_buf);

                return dir;
            }

        }

        clus = next_cluster(fsb_info, clus);
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

int fat32_readdir(file *filp, char *buf, filldir_t filler){

    FAT32_inode_info *info = filp->dentry->dir_node->private_index_info;
    FAT32_sb_info *sb = filp->dentry->dir_node->sb->private_sb_info;
    unsigned long size = sb->byte_per_clus;
    long cluster = info->first_cluster & 0x0fffffff;
    char *block = kmalloc(size, 0);
    if(!block) return -ENOMEM;
    char name[256] = {0};
    int rc = 0;
    for(long skip = filp->position / size; skip && cluster > 0; skip--)
        cluster = next_cluster(sb, cluster);
    while(cluster > 0){
        unsigned long lba = CLUS_TO_LBA(sb->fst_data_sector, cluster, sb->sec_per_clus);
        if(!ide_transfer(ATA_READ_CMD, lba, sb->sec_per_clus, (unsigned char *)block)){
            rc = -EIO;
            break;
        }
        for(unsigned long off = filp->position % size; off < size; off += sizeof(FAT32_Directory)){
            FAT32_Directory *e = (FAT32_Directory *)(block + off);
            if(!e->dir_name[0]) goto out;
            filp->position += sizeof(*e);
            if(e->dir_name[0] == 0xe5){ name[0] = 0; continue; }
            if(e->dir_attr == ATTR_LONG_NAME){
                long_filename_part(name, (FAT32_LongDirectory *)e);
                continue;
            }
            if(e->dir_attr & 8){ name[0] = 0; continue; }
            if(!name[0]) short_filename(e, name);
            rc = filler(buf, name, StringLen(name), 0, 0);
            goto out;
        }
        cluster = next_cluster(sb, cluster);
    }
    if(cluster < 0) rc = cluster;
out:
    kfree(block);
    return rc;
}

static long append_cluster(FAT32_sb_info *sb, unsigned long prev){
    long next = find_free_cluster(sb);
    if(next < 0) return next;
    int rc = set_cluster(sb, next, 0x0fffffff);
    if(rc) return rc;
    if(prev){
        rc = set_cluster(sb, prev, next);
        if(rc){
            set_cluster(sb, next, 0);
            return rc;
        }
    }
    return next;
}

static int fat32_file_io(file *filp, char *buf, unsigned long count, long *position, int write){
    index_node *inode = filp->dentry->dir_node;
    FAT32_inode_info *info = inode->private_index_info;
    FAT32_sb_info *sb = inode->sb->private_sb_info;
    unsigned long size = sb->byte_per_clus;
    if(*position < 0 || (unsigned long)*position > inode->file_size) return write ? -EINVAL : 0;
    if(write && count && *position == 0xffffffffUL) return -EFBIG;
    if(write) count = min(count, 0xffffffffUL - *position);
    else count = min(count, inode->file_size - *position);
    if(!count) return 0;
    char *block = kmalloc(size, 0);
    if(!block) return -ENOMEM;
    long cluster = info->first_cluster & 0x0fffffff;
    unsigned long prev = 0;
    for(unsigned long n = (unsigned long)*position / size; n && cluster > 0; n--){
        prev = cluster;
        cluster = next_cluster(sb, cluster);
    }
    unsigned long done = 0;
    unsigned long offset = (unsigned long)*position % size;
    int err = -EIO;
    while(done < count){
        if(cluster < 0){ err = cluster; break; }
        if(!cluster){
            if(!write || (unsigned long)*position + done < inode->file_size) break;
            cluster = append_cluster(sb, prev);
            if(cluster < 0){ err = cluster; break; }
            if(!prev) info->first_cluster = cluster;
            memset(block, 0, size);
        }else{
            unsigned long lba = CLUS_TO_LBA(sb->fst_data_sector, cluster, sb->sec_per_clus);
            if(!ide_transfer(ATA_READ_CMD, lba, sb->sec_per_clus, (unsigned char *)block)) break;
        }
        unsigned long bytes = min(size - offset, count - done);
        if(write){
            memcopy(buf + done, block + offset, bytes);
            unsigned long lba = CLUS_TO_LBA(sb->fst_data_sector, cluster, sb->sec_per_clus);
            if(!ide_transfer(ATA_WRITE_CMD, lba, sb->sec_per_clus, (unsigned char *)block)) break;
        }else{
            memcopy(block + offset, buf + done, bytes);
        }
        done += bytes;
        offset = 0;
        if(done < count){
            prev = cluster;
            cluster = next_cluster(sb, cluster);
        }
    }
    if(write && (unsigned long)*position + done > inode->file_size){
        unsigned long old_size = inode->file_size;
        inode->file_size = *position + done;
        err = fat32_write_inode(inode);
        if(err){
            inode->file_size = old_size;
            done = old_size - *position;
        }
    }
    *position += done;
    kfree(block);
    return done ? (int)done : err;
}

int fat32_read(file *filp, char *buf, unsigned long count, long *position){
    return fat32_file_io(filp, buf, count, position, 0);
}

int fat32_write(file *filp, char *buf, unsigned long count, long *position){
    return fat32_file_io(filp, buf, count, position, 1);
}

int fat32_close(index_node *inode, file *filp){ return 0; }
long fat32_lseek(file *filp, long offset, long origin){
    long base = 0;
    if(origin == SEEK_CUR) base = filp->position;
    else if(origin == SEEK_END) base = filp->dentry->dir_node->file_size;
    else if(origin != SEEK_SET) return -EINVAL;
    long pos;
    if(__builtin_add_overflow(base, offset, &pos) || pos < 0 ||
       (unsigned long)pos > filp->dentry->dir_node->file_size) return -EOVERFLOW;
    filp->position = pos;
    return pos;
}
int fat32_ioctl(index_node * inode, file * filp, unsigned long cmd, unsigned long arg){ return -ENOTTY; }

super_block * fat32_read_superblock(disk_partition_table_entry *dpte, void *buf){

    fsb = (super_block *)kmalloc(sizeof(super_block), 0);
    fsb->sb_ops = &FAT32_sb_ops;

    FAT32_sb_info * private_sb_info = kmalloc(sizeof(*private_sb_info), 0);
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

    memset(inode, 0, sizeof(*inode));
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

int fat32_write_inode(index_node *inode){
    FAT32_inode_info * inode_info = (FAT32_inode_info *)inode->private_index_info;
    FAT32_sb_info * sb_info = inode->sb->private_sb_info;
    FAT32_Directory * buf = kmalloc(sb_info->byte_per_clus, 0);
    if(!buf) return -ENOMEM;
    unsigned long lba = CLUS_TO_LBA(sb_info->fst_data_sector, inode_info->dentry_location, sb_info->sec_per_clus);
    if(!ide_transfer(ATA_READ_CMD, lba, sb_info->sec_per_clus, (unsigned char *)buf)){
        kfree(buf);
        return -EIO;
    }
    FAT32_Directory * tentry = buf + inode_info->dentry_position;
    tentry->dir_file_size = inode->file_size;
    tentry->dir_fst_clus_hi = GetBits(inode_info->first_cluster, 16, 16);
    tentry->dir_fst_clus_lo = GetBits(inode_info->first_cluster, 0, 16);
    tentry->dir_wrt_date = inode_info->write_date;
    tentry->dir_wrt_time = inode_info->write_time;

    int ok = ide_transfer(ATA_WRITE_CMD, lba, sb_info->sec_per_clus, (unsigned char *)buf);
    kfree(buf);
    return ok ? 0 : -EIO;
}

int fat32_truncate(index_node *inode){
    FAT32_inode_info *info = inode->private_index_info;
    FAT32_sb_info *sb = inode->sb->private_sb_info;
    long cluster = info->first_cluster;
    unsigned long old_size = inode->file_size;
    info->first_cluster = 0;
    inode->file_size = 0;
    int rc = fat32_write_inode(inode);
    if(rc){
        inode->file_size = old_size;
        info->first_cluster = cluster;
        return rc;
    }
    while(cluster > 0){
        long next = next_cluster(sb, cluster);
        if(next < 0) return next;
        rc = set_cluster(sb, cluster, 0);
        if(rc) return rc;
        cluster = next;
    }
    return 0;
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
