#ifndef FAT32_H
#define FAT32_H


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
    unsigned char   bs_reserved[446];
    disk_partition_table_entry DPTE[4];
    unsigned short  bs_trailsignature;  // 0xaa55
}__attribute__((packed))disk_partition_table;


// totally 90B
typedef struct disk_boot_sector{
    unsigned char BS_jmpBoot[3];
    unsigned char BS_OEMName[8];
    unsigned char BPB_BytesPerSec[2];
    unsigned char BPB_SecsPerClus;
    unsigned char BPB_RsvdSecCnt[2];
    unsigned char BPB_NumFATs;
    unsigned char BPB_RootEntCnt[2];    // Usually 0
    unsigned char BPB_TotSec16[2];      // Usually 0
    unsigned char BPB_Media;
    unsigned char BPB_FATsz16[2];       // Usually 0
    unsigned char BPB_SecPerTrk[2];
    unsigned char BPB_NumHeads[2];
    unsigned char BPB_HiddSec[4];
    unsigned char BPB_TotSec32[4];
    unsigned char BPB_FATsz32[4];
    unsigned char BPB_ExtFlags[2];
    unsigned char BPB_FSVer[2];         // Usually 0
    unsigned char BPB_RootClus[4];      // Usually 2
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


void DISK1_FAT32_FS_INIT();

#endif