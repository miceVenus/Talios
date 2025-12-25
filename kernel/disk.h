#ifndef DISK_H
#define DISK_H

#define ATA_WRITE_CMD   0x34 // In LBA 48
#define ATA_READ_CMD    0x24 // In LBA 48

#define ATA_GET_DISK_ID_CMD 0xec


#include "lib.h"

typedef struct block_buffer_node{
    unsigned int count;
    unsigned char cmd;
    unsigned long lba;
    unsigned char *buffer;

    void (* end_handler)(unsigned long nr, unsigned long parameter);

    struct List list;

}block_buffer_node;

typedef struct request_queue{
    struct List queue_list;
    block_buffer_node *in_using;
    long block_request_count;

}request_queue;

typedef struct block_device_operation{
    long (*open)();
    long (*close)();
    long (*ioctl)(long cmd, long arg);
    long (*transfer)(long cmd, unsigned long blocks, long count, unsigned char *buffer);

}block_device_operation;

typedef struct disk_device_info disk_device_info;

struct disk_device_info{

    struct normal_config{
        unsigned long   reserved1   : 15,   // 0~14
                        is_atapi    : 1,    // 15
                        reserved2   : 48;   // 16 ~ 63

        unsigned long   reserved3;
        unsigned int    reserved4;
    }__attribute__((packed));               // 0~19B

    char serial_num[20];                    // 20~39B

    char reserved1[6];                      // 40~45B
    char firm_verison[8];                   // 46~53B
    char product_type[40];                  // 54~93B
    char reserved2[4];                      // 94~97B

    struct support_function_status{
        unsigned short  reserved1   : 9,    // 0~8
                        has_LBA     : 1,    // 9
                        has_DMA     : 1,    // 10
                        reserved2   : 5;    // 11~15
    }__attribute__((packed));               // 98~99B

    char reserved3[20];                     // 100B~119B

    unsigned int max_logic_sectors_28;      // 120~123B

    char reserved4[28];                     // 124~151B

    struct SATA_function_status{
        unsigned short  reserved1       : 2,        // 0~1
                        SATA_gen2_3_0GB : 1,        // 2
                        SATA_gen1_1_5GB : 1,        // 3
                        reserved2       : 12;       // 4~15
    }__attribute__((packed));               // 152~153B

    char reserved5[6];                      // 154~159B

    struct main_version{
        unsigned short  reserved1       : 4,        // 0~3
                        is_ATAPI_4      : 1,        // 4
                        is_ATAPI_5      : 1,        // 5
                        is_ATAPI_6      : 1,        // 6
                        is_ATAPI_7      : 1,        // 7
                        is_ATAPI_8      : 1,        // 8
                        reserved2       : 8;        // 9~15
    }__attribute__((packed));                       // 160~161B

    unsigned short vice_version;                    // 162~163B

    char reserved6[36];                             // 164~199B     

    unsigned long max_logic_sectors_48;             // 200~207B

    char reserved7[44];                             // 208~351B

    char media_serial_number[60];                   // 352~411B

    char reserved8[32];                             // 412~443B

    struct trans_main_version{
        unsigned short  APT_AST                 : 1,        // 0
                        ATAPI_7_SATA_1_0        : 1,        // 1
                        SATA_extension          : 1,        // 2
                        SATA_REV_2_5            : 1,        // 3
                        SATA_REV_2_6            : 1,        // 4
                        reserved                : 7,        // 5~11
                        trans_type              : 4;        // 12~15 0 p 1 s
    }__attribute__((packed));                       // 444~445B

    unsigned short trans_vice_version;              // 446~447B

    char reserved9[62];                             // 448~509B

    struct check_sum{
        unsigned short  A5h                     : 8,        // 0~7
                        check_sum_compan        : 8;        // 8~15
    }__attribute__((packed));                       // 510~511B

}__attribute__((packed));

long ide_open();
long ide_close();
long ide_ioctl(long cmd, long arg);
long ide_transfer(long cmd, unsigned long blocks, long count, unsigned char *buffer);

#endif
