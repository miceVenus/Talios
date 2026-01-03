#include "disk.h"
#include "apic.h"
#include "interrupt.h"
#include "memory.h"
#include "printk.h"

// DISK USE IRQ 14 OR 15

enum PRIMARY_CHANNEL_CMD{
    PRIMARY_CHANNEL_CMD_DATA = 0x1F0,
    PRIMARY_CHANNEL_CMD_ERROR_STATUS, // READ ONLY
    PRIMARY_CHANNEL_CMD_SECTOR_NUM,
    PRIMARY_CHANNEL_CMD_SECTOR,
    PRIMARY_CHANNEL_CMD_COLUMN1,
    PRIMARY_CHANNEL_CMD_COLUMN2,
    PRIMARY_CHANNEL_CMD_CONF_REGISTER,
    PRIMARY_CHANNEL_CMD_STATUS_CMD // STATUS FOR READ CMD FOR WRITE
};

enum SECONDARY_CHANNEL_CMD{
    SECONDARY_CHANNEL_CMD_DATA = 0x170,
    SECONDARY_CHANNEL_CMD_ERROR_STATUS, // READ ONLY
    SECONDARY_CHANNEL_CMD_SECTOR_NUM,
    SECONDARY_CHANNEL_CMD_SECTOR,
    SECONDARY_CHANNEL_CMD_COLUMN1,
    SECONDARY_CHANNEL_CMD_COLUMN2,
    SECONDARY_CHANNEL_CMD_CONF_REGISTER,
    SECONDARY_CHANNEL_CMD_STATUS_CMD // STATUS FOR READ CMD FOR WRITE
};

/*  
    DISK_CMD_CONF_REGISTER
    bit7 must be 1 In LBA 28
    bit6 means address mode 0 CHS mode 1 LBA mode
    bit5 must be 1 In LBA 28
    bit4 0 means Master disk 1 means Slave disk
    bit0~3 means Disk Head In CHS mode LBA(27:24) In LBA mode

    DISK_CMD_ERROR_STATUS
    bit7 = 1 bad sector
    bit6 = 1 unrestorable data error
    bit4 = 1 no found ID OR sector

    DISK_CMD_STATUS_CMD for CMD 
    0xEC : device information
    0x20 : read sector (used for LBA 28 mode)
    0x24 : read sector (used for LBA 48 mode)
    0x30 : write sector (used for LBA 28 mode)
    0x34 : write sector (used for LBA 48 mode)

    DISK_CMD_STATUS_CMD for STATUS IS Same like 3f6/376
*/

enum PRIMARY_CHANNEL_CTRL{
    PRIMARY_CHANNEL_CTRL_STATUS_CTRL = 0x3F6, // STATUS FOR READ CMD FOR WRITE
};

enum SECONDARY_CHANNEL_CTRL{
    SECONDARY_CHANNEL_CTRL_STATUS_CTRL = 0x376, // STATUS FOR READ CMD FOR WRITE
};

/*  
    DISK_CTRL_STATUS_CTRL for CTRL 
    bit2 = 1 reboot controler
    bit1 = 1 mask IRQ 14 In 3F6 mask IRQ 15 in 376 

    DISK_CTRL_STATUS_CTRL for STATUS
    bit7 = 1 controller is busy
    bit6 = 1 controller is ready
    bit3 = 1 data is requsting
    bit0 = 1 command excute in error
*/

#define DISK_STATUS_BUSY    (1 << 7)
#define DISK_STATUS_READY   (1 << 6)
#define DISK_STATUS_REQ     (1 << 3)
#define DISK_STATUS_ERROR   (1 << 0)

void ListInit(struct List * list);
void ListBackAdd(struct List *list, struct List *new);
int ListDelete(struct List *list);

void read_handler(unsigned long nr, unsigned long arg);
void write_handler(unsigned long nr, unsigned long arg);
void other_handler();
void get_disk_id_handler(unsigned long nr, unsigned long arg);

#include "lib.h"

block_device_operation ide_device_operation = {
    .close      = ide_close,
    .ioctl      = ide_ioctl,
    .open       = ide_open,
    .transfer   = ide_transfer
};

request_queue disk_request_queue;

static unsigned int disk_flags = 0;

static HwInterruptT disk_irq_controller;

block_buffer_node * make_request(long cmd, unsigned long blocks, long count, unsigned char *buffer){
   block_buffer_node * node =  (block_buffer_node*)kmalloc(sizeof(block_buffer_node), 0);
   ListInit(&node->list);
   
   switch (cmd){
        case ATA_READ_CMD:
            node->cmd = ATA_READ_CMD;
            node->end_handler = read_handler;
            break;

        case ATA_WRITE_CMD:
            node->cmd = ATA_WRITE_CMD;
            node->end_handler = write_handler;
            break;

        case ATA_GET_DISK_ID_CMD:
            node->cmd = ATA_GET_DISK_ID_CMD;
            node->end_handler = get_disk_id_handler;
        default:
            node->cmd = cmd;
            node->end_handler = other_handler;
            break;
   }
   node->buffer = buffer;
   node->count  = count;
   node->lba    = blocks;

   return node;
}

long cmd_out(){
    block_buffer_node * node = ContainerOf(&disk_request_queue.queue_list, block_buffer_node, list);
    disk_request_queue.in_using = node;
    ListDelete(&disk_request_queue.queue_list);
    disk_request_queue.block_request_count--;

    while(IN8b(SECONDARY_CHANNEL_CTRL_STATUS_CTRL) & DISK_STATUS_BUSY)
        nop();

    switch (node->cmd){
        case ATA_READ_CMD:
            OUT8b(SECONDARY_CHANNEL_CMD_CONF_REGISTER, 0x40);

            OUT8b(SECONDARY_CHANNEL_CMD_ERROR_STATUS, 0);
            OUT8b(SECONDARY_CHANNEL_CMD_SECTOR_NUM, GetBits(node->count, 8, 8));
            OUT8b(SECONDARY_CHANNEL_CMD_SECTOR, GetBits(node->lba, 24, 8));
            OUT8b(SECONDARY_CHANNEL_CMD_COLUMN1, GetBits(node->lba, 32, 8));
            OUT8b(SECONDARY_CHANNEL_CMD_COLUMN2, GetBits(node->lba, 40, 8));

            OUT8b(SECONDARY_CHANNEL_CMD_ERROR_STATUS, 0);
            OUT8b(SECONDARY_CHANNEL_CMD_SECTOR_NUM, GetBits(node->count, 0, 8));
            OUT8b(SECONDARY_CHANNEL_CMD_SECTOR, GetBits(node->lba, 0, 8));
            OUT8b(SECONDARY_CHANNEL_CMD_COLUMN1, GetBits(node->lba, 8, 8));
            OUT8b(SECONDARY_CHANNEL_CMD_COLUMN2, GetBits(node->lba, 16, 8));

            while(!(IN8b(SECONDARY_CHANNEL_CTRL_STATUS_CTRL) & DISK_STATUS_READY))
                nop();
            
            OUT8b(SECONDARY_CHANNEL_CMD_STATUS_CMD, node->cmd);

            break;
        
        case ATA_WRITE_CMD:
            OUT8b(SECONDARY_CHANNEL_CMD_CONF_REGISTER, 0x40);


            OUT8b(SECONDARY_CHANNEL_CMD_ERROR_STATUS, 0);
            OUT8b(SECONDARY_CHANNEL_CMD_SECTOR_NUM, GetBits(node->count, 8, 8));
            OUT8b(SECONDARY_CHANNEL_CMD_SECTOR, GetBits(node->lba, 24, 8));
            OUT8b(SECONDARY_CHANNEL_CMD_COLUMN1, GetBits(node->lba, 32, 8));
            OUT8b(SECONDARY_CHANNEL_CMD_COLUMN2, GetBits(node->lba, 40, 8));

            OUT8b(SECONDARY_CHANNEL_CMD_ERROR_STATUS, 0);
            OUT8b(SECONDARY_CHANNEL_CMD_SECTOR_NUM, GetBits(node->count, 0, 8));
            OUT8b(SECONDARY_CHANNEL_CMD_SECTOR, GetBits(node->lba, 0, 8));
            OUT8b(SECONDARY_CHANNEL_CMD_COLUMN1, GetBits(node->lba, 8, 8));
            OUT8b(SECONDARY_CHANNEL_CMD_COLUMN2, GetBits(node->lba, 16, 8));

            while(!(IN8b(SECONDARY_CHANNEL_CTRL_STATUS_CTRL) & DISK_STATUS_READY))
                nop();
            
            OUT8b(SECONDARY_CHANNEL_CMD_STATUS_CMD, node->cmd);

            while(!(IN8b(SECONDARY_CHANNEL_CMD_STATUS_CMD) & DISK_STATUS_REQ))
                nop();

            port_outsw(node->buffer, SECONDARY_CHANNEL_CMD_DATA, 256);

            break;

        case ATA_GET_DISK_ID_CMD:

            OUT8b(SECONDARY_CHANNEL_CMD_CONF_REGISTER, 0xe0);

            OUT8b(SECONDARY_CHANNEL_CMD_ERROR_STATUS, 0);
            OUT8b(SECONDARY_CHANNEL_CMD_SECTOR_NUM, GetBits(node->count, 8, 8));
            OUT8b(SECONDARY_CHANNEL_CMD_SECTOR, GetBits(node->lba, 24, 8));
            OUT8b(SECONDARY_CHANNEL_CMD_COLUMN1, GetBits(node->lba, 32, 8));
            OUT8b(SECONDARY_CHANNEL_CMD_COLUMN2, GetBits(node->lba, 40, 8));

            while(!(IN8b(SECONDARY_CHANNEL_CTRL_STATUS_CTRL) & DISK_STATUS_READY))
                nop();

            OUT8b(SECONDARY_CHANNEL_CMD_STATUS_CMD, node->cmd);

            break;

        default:

            ColorPrintfk(RED, BLACK, "UnKown CMD %x In cmd_out()", node->cmd);
            break;
    }
}
void submit(block_buffer_node * node){
    disk_request_queue.block_request_count++;
    ListBackAdd(&disk_request_queue.queue_list, &node->list);

    if(disk_request_queue.in_using == NULL)
        cmd_out();
}

void wait_for_finish(){
    disk_flags = 1;
    while(disk_flags)
        nop();
}
long ide_close(){

}

long ide_open(){

}

long ide_ioctl(long cmd, long arg){

    switch(cmd){
    case ATA_GET_DISK_ID_CMD:
        OUT8b(SECONDARY_CHANNEL_CMD_STATUS_CMD, cmd);
        disk_device_info * device_info = (disk_device_info *)kmalloc(sizeof(disk_device_info), 0); 
        block_buffer_node * node = make_request(cmd, 0, 0, device_info);
        submit(node);
        wait_for_finish();
        return 1;
    
    default:
        ColorPrintfk(RED, BLACK, "UnKown CMD %x In ide_ioctl()", cmd);
        break;
    }
}

long ide_transfer(long cmd, unsigned long blocks, long count, unsigned char *buffer){
    block_buffer_node *node = NULL;

    if(cmd != ATA_READ_CMD && cmd != ATA_WRITE_CMD) return 0;

    node = make_request(cmd, blocks, count, buffer);
    submit(node);
    wait_for_finish();

    return 1;
}

void end_request(){
    kfree(disk_request_queue.in_using);
    disk_request_queue.in_using = NULL;

    disk_flags = 0;

    if(disk_request_queue.block_request_count != 0)
     cmd_out();
}

void get_disk_id_handler(unsigned long nr, unsigned long arg){
    block_buffer_node * node = ((request_queue*)arg)->in_using;
    if(IN8b(SECONDARY_CHANNEL_CMD_STATUS_CMD) & DISK_STATUS_ERROR){
        ColorPrintfk(RED, BLACK, "Get Disk Id Error : %x", IN8b(SECONDARY_CHANNEL_CMD_ERROR_STATUS));
    }else
        port_insw(node->buffer, SECONDARY_CHANNEL_CMD_DATA, 256);

    end_request();
}

void read_handler(unsigned long nr, unsigned long arg){
    block_buffer_node *node = ((request_queue*)arg)->in_using;
    if(IN8b(SECONDARY_CHANNEL_CMD_STATUS_CMD) & DISK_STATUS_ERROR){
        ColorPrintfk(RED, BLACK, "Read Handler Error : %x", IN8b(SECONDARY_CHANNEL_CMD_ERROR_STATUS));
    }else
        port_insw(node->buffer, SECONDARY_CHANNEL_CMD_DATA, 256);

    end_request();
}

void write_handler(unsigned long nr, unsigned long arg){
    block_buffer_node *node = ((request_queue*)arg)->in_using;
    if(IN8b(SECONDARY_CHANNEL_CMD_STATUS_CMD) & DISK_STATUS_ERROR){
        ColorPrintfk(RED, BLACK, "Read Handler Error : %x", IN8b(SECONDARY_CHANNEL_CMD_ERROR_STATUS));
    }

    end_request();
}

void other_handler(unsigned long nr, unsigned long arg){

}

void disk_irq_handler(struct PtRegs * regs, unsigned long nr, unsigned long arg){
    block_buffer_node *node = ((request_queue*)arg)->in_using;
    node->end_handler(nr, arg);
}

void disk_init(){
    IoApicRetEntry entry;
    BuildController(&disk_irq_controller);

    disk_request_queue.block_request_count = 0;
    ListInit(&disk_request_queue.queue_list);
    disk_request_queue.in_using = NULL;
    disk_flags = 0;

    entry.vector        = 0x2f;
    entry.DelivMode     = DELIV_M_FIXED;
    entry.DestMode      = DEST_M_PHYSICAL;
    entry.IntMask       = IOAPIC_INT_MASKED;
    entry.IntPol        = IOAPIC_INTPOL_H;
    entry.IRR           = IOAPIC_IRR_RESET;
    entry.Trigger       = IOAPIC_TRIGGER_EDGE;
    entry.DelivStatus   = DELIV_S_IDLE;
    entry.reserverd     = 0;
    entry.DestField.physical.reserverd1 = 0;
    entry.DestField.physical.physic_dst = 0;
    entry.DestField.physical.reserverd2 = 0;

    RegisterIrq(0x2f, &entry, disk_irq_handler, (unsigned long)(&disk_request_queue), &disk_irq_controller, "disk1");

    OUT8b(SECONDARY_CHANNEL_CMD_ERROR_STATUS, 0);
    OUT8b(SECONDARY_CHANNEL_CMD_SECTOR_NUM, 0);
    OUT8b(SECONDARY_CHANNEL_CMD_SECTOR, 0);
    OUT8b(SECONDARY_CHANNEL_CMD_COLUMN1, 0);
    OUT8b(SECONDARY_CHANNEL_CMD_COLUMN2, 0);
    OUT8b(SECONDARY_CHANNEL_CMD_CONF_REGISTER, 0);

    OUT8b(SECONDARY_CHANNEL_CMD_STATUS_CMD, 0xec);
}