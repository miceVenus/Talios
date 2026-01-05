#ifndef APIC_H
#define APIC_H

#define I440FX 0x11
#define EOIR_MSR 0x80b

enum IOAPIC_DELIV_M{
    DELIV_M_FIXED=0b000,
    DELIV_M_LOW_PRIO,
    DELIV_M_SMI,
    DELIV_M_NMI=0b100,
    DELIV_M_INIT,
    DELIV_M_EXIT_INT=0b111,
};

enum IOAPIC_DEST_M{
    DEST_M_PHYSICAL=0,
    DEST_M_LOGICAL,
};

enum IOAPIC_DELIV_S{
    DELIV_S_IDLE=0,
    DELIV_S_SEND_PENDING,
};
enum IOAPIC_INTPOL{
    IOAPIC_INTPOL_H=0,
    IOAPIC_INTPOL_L,
};
enum IOAPIC_REMOTE_IRR{
    IOAPIC_IRR_RESET=0,
    IOAPIC_IRR_SET,
};
enum IOAPIC_TRIGGER_M{
    IOAPIC_TRIGGER_LEVEL=0,
    IOAPIC_TRIGGER_EDGE,
};
enum IOAPIC_INT_MASK{
    IOAPIC_INT_UNMASKED=0,
    IOAPIC_INT_MASKED,
};

struct IoApicMap{
    unsigned int    PhysicalAddr;
    unsigned char   * VirtualIndexAddr;
    unsigned int    * VirtualDataAddr;
    unsigned int    * VirtualEoiAddr;
};
typedef struct IoApicRetEntry IoApicRetEntry;

struct IoApicRetEntry{
    unsigned int    vector      :8, //0~7
                    DelivMode   :3, //8~10
                    DestMode    :1, //11
                    DelivStatus :1, //12
                    IntPol      :1, //13
                    IRR         :1, //14
                    Trigger     :1, //15
                    IntMask     :1, //16
                    reserverd   :15;//17~31 
    union
    {
        struct physical{
            unsigned int    reserverd1  :24,    //32~55
                            physic_dst  :4,     //56~59
                            reserverd2  :4;     //60~63

        }physical;
        struct logical{
            unsigned int    reserverd1  :24,    //32~55
                            logic_dst   :8;     //56~63
        }logical;
    }DestField;

}__attribute__((packed));

void enable_lapic();
unsigned long get_lapic_id();
unsigned long get_lapic_version();
void init_lapic_svr();
void set_lapic_tpr(unsigned long priority);
void set_lapic_lvt(unsigned long entry,unsigned long content);
void mask_lapic_lvt(unsigned long entry);
int check_apic_x2apic();

void InitLocalApic();
void InitIoApic();
void IoApicPageTableRemap();
unsigned long IoApicRteRead(unsigned char index);
void IoApicRteWrite(unsigned char index, unsigned long value);

void ApicEnable(unsigned long irq);

void ApicAck(unsigned long irq);

void ApicInstall(unsigned long irq, void * arg);
void ApicUninstall(unsigned long irq);
void ApicDisable(unsigned long irq);

#endif