#ifndef FAT32_H
#define FAT32_H


#define ATTR_READ_ONLY  (1 << 0)
#define ATTR_HIDDEN     (1 << 1)
#define ATTR_SYSTEM     (1 << 2)
#define ATTR_VOLUME_ID  (1 << 3)
#define ATTR_DIRECTORY  (1 << 4)
#define ATTR_ARCHIVE    (1 << 5)
#define ATTR_LONG_NAME  0x0f

// 16B totally
typedef struct disk_partition_table_entry{

    unsigned int    boot_indicator: 8,      // 0x80 active partition 0x00 inactive
                    start_head:     8,
                    start_sector:   6,
                    start_cylinder: 10;
    
    unsigned int    partition_type: 8,      // 0x0c FAR32 LBA 0x0b FAT32 CHS
                    end_head:       8,
                    end_sector:     6,
                    end_cylinder:   10;

    unsigned int    start_lba;
    unsigned int    total_sectors;


}__attribute__((packed))disk_partition_table_entry;


// MBR
typedef struct disk_partition_table{
    // loader code
    unsigned char   bs_reserved[446];
    disk_partition_table_entry DPTE[4];
    unsigned short  bs_trailsignature;  // 0xaa55
}__attribute__((packed))disk_partition_table;


// totally 90B
typedef struct disk_boot_sector{
    unsigned char BS_jmpBoot[3];
    unsigned char BS_OEMName[8];
    unsigned short BPB_BytesPerSec;
    unsigned char BPB_SecsPerClus;
    unsigned short BPB_RsvdSecCnt;
    unsigned char BPB_NumFATs;
    unsigned char BPB_RootEntCnt[2];    // Usually 0
    unsigned char BPB_TotSec16[2];      // Usually 0
    unsigned char BPB_Media;
    unsigned char BPB_FATsz16[2];       // Usually 0
    unsigned char BPB_SecPerTrk[2];
    unsigned char BPB_NumHeads[2];
    unsigned char BPB_HiddSec[4];
    unsigned int BPB_TotSec32;
    unsigned int  BPB_FATsz32;
    unsigned char BPB_ExtFlags[2];
    unsigned char BPB_FSVer[2];         // Usually 0
    unsigned int  BPB_RootClus;      // Usually 2
    unsigned short BPB_FSInfo;
    unsigned char BPB_BKBootSec[2];     // Usually 6
    unsigned char BPB_Reserved[12];
    unsigned char BS_DrvNum[1];         // DISK is 0x80
    unsigned char BS_Reserved1[1];
    unsigned char BS_BootSig[1];        // Usually 0x29
    unsigned char BS_VolID[4];          // Randomly
    unsigned char BS_VolLab[11];
    unsigned char BS_FileSysType[8];    // "FAT 32"
    unsigned char BootCode[420];
    unsigned short BS_TrailSignature;   // 0xaa55
}__attribute__((packed))disk_boot_sector;

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
    unsigned char dir_ntres;
    unsigned char dir_crt_time_tenth;
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
int path_walk(FAT32_Directory* res, char *path, unsigned long flags);

#endif