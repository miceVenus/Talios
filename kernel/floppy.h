#ifndef FLOPPY_H
#define FLOPPY_H



enum FloppyCommands{
   // Floppy_READ_TRACK =                 2,	      // generates IRQ6
   FLOPPY_SPECIFY =                    3,       // * set drive parameters
   // Floppy_SENSE_DRIVE_STATUS =         4,
   FLOPPY_WRITE_DATA =                 5,       // * write to the disk
   FLOPPY_READ_DATA =                  6,       // * read from the disk
   FLOPPY_RECALIBRATE =                7,       // * seek to cylinder 0
   FLOPPY_SENSE_INTERRUPT =            8,       // * ack IRQ6, get status of last command
   // Floppy_WRITE_DELETED_DATA =         9,
   // Floppy_READ_ID =                    10,	   // generates IRQ6
   // Floppy_READ_DELETED_DATA =          12,
   // Floppy_FORMAT_TRACK =               13,      // *
   // Floppy_DUMPREG =                    14,
   FLOPPY_SEEK =                       15,      // * seek both heads to cylinder X
   // Floppy_VERSION =                    16,	   // * used during initialization, once
   // Floppy_SCAN_EQUAL =                 17,
   // Floppy_PERPENDICULAR_MODE =         18,	   // * used during initialization, once, maybe
   // Floppy_CONFIGURE =                  19,      // * set controller parameters
   // Floppy_LOCK =                       20,      // * protect controller params from a reset
   // Floppy_VERIFY =                     22,
   // Floppy_SCAN_LOW_OR_EQUAL =          25,
   // Floppy_SCAN_HIGH_OR_EQUAL =         29
};


typedef struct FDCPort{
   unsigned short SRA;  // read-only
   unsigned short SRB;  // read-only
   unsigned short DOR;
   unsigned short TDR;
   unsigned short MSR;  // read-only
   unsigned short DSR;  // write-only
   unsigned short FIFO;
   unsigned short DIR;  // read-only
   unsigned short CCR;  // write-only
}FDCPort;

#define SECTOR_PER_TRACK   18
#define HEADS              2

typedef struct CHS{
   unsigned long cylinder;
   unsigned long head;
   unsigned long sector;
}CHS;

typedef struct FDCStatus{
   unsigned char st0;
   unsigned char st1;
   unsigned char st2;
   unsigned char st3; // This Is a fake register
   unsigned char cylinder;
   unsigned char head;
   unsigned char sector;
}FDCStatus;


void read(unsigned long lba, unsigned char * buffer);
void write(unsigned long lba, unsigned char * buffer);

void FloppyInit();

#endif