#include "floppy.h"
#include "apic.h"
#include "interrupt.h"
#include "lib.h"
#include "printk.h"

static HwInterruptT FloppyController;
static FDCPort port;
volatile int ReceivedInterrupt = 0;
volatile int ReadFromFDC = 0;
volatile int WriteToFDC = 0;

void FDCWriteByte(unsigned char byte){

    for(int i = 0; i < 50000; i++){
        unsigned char MSR = IN8b(port.MSR);
        if((MSR & 0xc0) == 0x80) {
            OUT8b(port.FIFO, byte);
            return;
        }
    }
}

unsigned char FDCReadByte(){
    for(int i = 0; i < 50000; i++){
        unsigned char MSR = IN8b(port.MSR);
        if((MSR & 0xD0) == 0xD0) // FDC -> CPU
            return IN8b(port.FIFO);
        nop();
    }
    return 0;
}

FDCStatus FloppySenseIntr(){

    FDCStatus result = {0};
    FDCWriteByte(FLOPPY_SENSE_INTERRUPT);
    result.st0 = FDCReadByte();
    result.cylinder = FDCReadByte();
    return result;
}

unsigned int FloppySpecify(unsigned char SRT_HUT, unsigned char HLT_ND){
    FDCWriteByte(FLOPPY_SPECIFY);
    FDCWriteByte(SRT_HUT);
    FDCWriteByte(HLT_ND);
    return 1;
}


unsigned int FloppyRecalibrate(unsigned char DriveNum){

    FDCStatus status;
    ReceivedInterrupt = 0;

    FDCWriteByte(FLOPPY_RECALIBRATE);
    FDCWriteByte(DriveNum);
    while(!ReceivedInterrupt) nop();
    status = FloppySenseIntr();
    if(status.cylinder){
        ColorPrintfk(RED, BLUE, "Error in RECALIBRATE false to ReSet");
        return 0;
    }
    return 1;
}

unsigned int FloppySeek(unsigned char HeadDrive, unsigned char DriveNum){

    FDCStatus status;
    ReceivedInterrupt = 0;

    FDCWriteByte(FLOPPY_SEEK);
    FDCWriteByte(HeadDrive);
    FDCWriteByte(DriveNum);

    while(!ReceivedInterrupt) nop();
    status = FloppySenseIntr();
    if(status.cylinder){
        ColorPrintfk(RED, BLUE, "Error in RECALIBRATE false to ReSet");
        return 0;
    }
    return 1;
}

void FloppySendAccessCmd(   unsigned char DriveNum, unsigned char cylinder, unsigned char head, 
                            unsigned char sector, unsigned char SectorSize, unsigned char EndOfTrack,
                            unsigned char GapLength, unsigned char DataLength, unsigned char cmd)
{
    FDCWriteByte(cmd);
    FDCWriteByte(DriveNum);
    FDCWriteByte(cylinder);
    FDCWriteByte(head);
    FDCWriteByte(sector);
    FDCWriteByte(SectorSize);
    FDCWriteByte(EndOfTrack);
    FDCWriteByte(GapLength);
    FDCWriteByte(DataLength);
}

FDCStatus FloppyReadData(unsigned char DriveNum, unsigned char cylinder, unsigned char head, unsigned char sector, unsigned char *buffer)
{
    FDCStatus result = {0};
    ReadFromFDC = 1;
    ReceivedInterrupt = 0;

    FloppySendAccessCmd(DriveNum, cylinder, head, sector, 0x2 , 0x12, 0x1B, 0xff, FLOPPY_WRITE_DATA);

    while(!ReceivedInterrupt) {
        unsigned char msr = IN8b(port.MSR);
        if((msr & 0xD0) == 0xD0) {
            *(buffer++) = FDCReadByte();
        }
    }

    result.st0      = FDCReadByte();
    result.st1      = FDCReadByte();
    result.st2      = FDCReadByte();
    result.cylinder = FDCReadByte();
    result.head     = FDCReadByte();
    result.sector   = FDCReadByte();
    result.st3      = FDCReadByte(); // In Actually st3 = SectorSize

    return result;
}

FDCStatus FloppyWriteData(unsigned char DriveNum, unsigned char cylinder, unsigned char head, unsigned char sector, unsigned char *buffer)
{

    FDCStatus result = {0};
    ReadFromFDC = 1;
    ReceivedInterrupt = 0;

    FloppySendAccessCmd(DriveNum, cylinder, head, sector, 0x2 , SECTOR_PER_TRACK, 0x1B, 0xff, FLOPPY_READ_DATA);

    while(!ReceivedInterrupt) {
        unsigned char msr = IN8b(port.MSR);
        if((msr & 0xc0) == 0x80) {
            FDCWriteByte(*(buffer++));
        }
    }

    result.st0      = FDCReadByte();
    result.st1      = FDCReadByte();
    result.st2      = FDCReadByte();
    result.cylinder = FDCReadByte();
    result.head     = FDCReadByte();
    result.sector   = FDCReadByte();
    result.st3      = FDCReadByte(); // In Actually st3 = SectorSize

    return result;
}

void FloppyReset(){
    ReceivedInterrupt = 0; 	// This will prevent the FDC from being faster than us!

    // Enter, then exit reset mode.
    OUT8b(port.DOR,0x00);
    OUT8b(port.DOR,0x1C);

    for(int i = 0; i < 4; i++){
        FDCStatus status = FloppySenseIntr();
        if(!status.cylinder) break;
        ColorPrintfk(RED, BLACK, "Sense Intr Try again");
    }

    OUT8b(port.CCR,0x00);	// 500Kbps -- for 1.44M floppy

    // configure the drive
    FloppySpecify((8 << 4) | 0, (5 << 1) | 1);
}


void FloppyHandler(struct PtRegs * regs, unsigned long nr, unsigned long arg){
    ReceivedInterrupt = 1;

    // if(ReadFromFDC) *buffer = FDCReadByte();
    ColorPrintfk(BLUE, BLACK, "1");
    // if(ReadFromFDC)
    // if(WriteToFDC) FDCWriteByte(*buffer);
}

CHS LBA2CHS(unsigned long lba){
    CHS chs = (CHS){
        .cylinder   = (lba) / (HEADS * SECTOR_PER_TRACK),
        .head       = (lba / SECTOR_PER_TRACK) % (HEADS),
        .sector     = (lba % SECTOR_PER_TRACK) + 1
    };
    return chs;
}

unsigned long CHS2LBA(CHS chs){
    return ((chs.cylinder * HEADS) + chs.head)*SECTOR_PER_TRACK + (chs.sector - 1);
}

void read(unsigned long lba, unsigned char * buffer){
    CHS chs = LBA2CHS(lba);
    FloppyReadData(0, chs.cylinder, chs.head, chs.sector, buffer);
}

void write(unsigned long lba, unsigned char * buffer){
    CHS chs = LBA2CHS(lba);
    FloppyWriteData(0, chs.cylinder, chs.head, chs.sector, buffer);
}

void FloppyInit(){
    
    port = (FDCPort){0x3F0, 0x3F1, 0x3F2, 0x3F3, 0x3F4, 0x3F4, 0x3F5, 0x3F7, 0x3F7};

    IoApicRetEntry entry;
    BuildController(&FloppyController);

    entry.vector        = 0x26;
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

    FloppyReset();
    RegisterIrq(0x26, &entry, FloppyHandler, 0 , &FloppyController, "PS/2 FloppyInterrupt");
}