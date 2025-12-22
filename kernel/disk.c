#include "disk.h"
#include "apic.h"
#include "interrupt.h"

static HwInterruptT disk_irq_controller;
// DISK USE IRQ 14 OR 15

enum MASTER_DISK_CMD{
    MASTER_DISK_CMD_DATA = 0x1F0,
    MASTER_DISK_CMD_ERROR_STATUS, // READ ONLY
    MASTER_DISK_CMD_SECTOR_NUM,
    MASTER_DISK_CMD_SECTOR,
    MASTER_DISK_CMD_COLUMN1,
    MASTER_DISK_CMD_COLUMN2,
    MASTER_DISK_CMD_CONF_REGISTER,
    MASTER_DISK_CMD_STATUS_CMD // STATUS FOR READ CMD FOR WRITE
};

enum SLAVE_DISK_CMD{
    SLAVE_DISK_CMD_DATA = 0x170,
    SLAVE_DISK_CMD_ERROR_STATUS, // READ ONLY
    SLAVE_DISK_CMD_SECTOR_NUM,
    SLAVE_DISK_CMD_SECTOR,
    SLAVE_DISK_CMD_COLUMN1,
    SLAVE_DISK_CMD_COLUMN2,
    SLAVE_DISK_CMD_CONF_REGISTER,
    SLAVE_DISK_CMD_STATUS_CMD // STATUS FOR READ CMD FOR WRITE
};

/*  
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

enum MASTER_DISK_CTRL{
    MASTER_DISK_CTRL_STATUS_CTRL = 0x3F6, // STATUS FOR READ CMD FOR WRITE
};

enum SLAVE_DISK_CTRL{
    SLAVE_DISK_CTRL_STATUS_CTRL = 0x376, // STATUS FOR READ CMD FOR WRITE
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

void disk_irq_handler(struct PtRegs * regs, unsigned long nr, unsigned long arg){

}
void disk_init(){
    IoApicRetEntry entry;
    BuildController(&disk_irq_controller);

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

    RegisterIrq(0x2f, &entry, disk_irq_handler, 0, &disk_irq_controller, "disk1");

    OUT8b(SLAVE_DISK_CMD_ERROR_STATUS, 0);
    OUT8b(SLAVE_DISK_CMD_SECTOR_NUM, 0);
    OUT8b(SLAVE_DISK_CMD_SECTOR, 0);
    OUT8b(SLAVE_DISK_CMD_COLUMN1, 0);
    OUT8b(SLAVE_DISK_CMD_COLUMN2, 0);
    OUT8b(SLAVE_DISK_CMD_CONF_REGISTER, 0);
    OUT8b(SLAVE_DISK_CMD_STATUS_CMD, 0xec);
}