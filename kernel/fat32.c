#include "fat32.h"
#include "disk.h"
#include "lib.h"
#include "printk.h"

void DISK1_FAT32_FS_INIT(){
    unsigned char buf[512];
    disk_partition_table dpt;
    disk_boot_sector dbs;
    FAT32_FSInfo fs_info;

    // pre read for some unknown reason result in zero read when first reading
    // maybe bochs or else

    memset(buf, 0, 512);
    ide_transfer(ATA_READ_CMD, 0, 1, buf);
    dpt = *(disk_partition_table*)buf;
    ColorPrintfk(BLUE, BLACK, "dpt bs_trailsignature: %x\n", dpt.bs_trailsignature);

    memset(buf, 0, 512);
    ide_transfer(ATA_READ_CMD, dpt.DPTE[0].start_lba, 1, buf);
    dbs = *(disk_boot_sector*)buf;
    ColorPrintfk(BLUE, BLACK, "dbs BS_FileSysType: %s\n", dbs.BS_FileSysType);

    memset(buf, 0, 512);
    ide_transfer(ATA_READ_CMD, dpt.DPTE[0].start_lba + dbs.BPB_FSInfo, 1, buf);
    fs_info = *(FAT32_FSInfo*)buf;
    ColorPrintfk(BLUE, BLACK, "fs_info free_count: %d\n", fs_info.free_count);

}