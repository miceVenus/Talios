#ifndef SMP_H
#define SMP_H

/*
    ICR IN MSR 0x830

    delivery target In x2 APIC Is 63 ~ 32 In x APIC & APIC Is 63 ~ 56 

    ShortHand In x2 APIC xAPIC APIC Is bit 19 ~ 18
    00 No Short Hand
    01 Only Self
    10 Send To ALL (include self)
    11 Send To ALL (exclude self)

    Trigger Mode bit 15 0 means edge trigger 1 means level trigger
    Drive Level bit 14 0 means invalid 1 means valid
    Delivery Status bit 12 0 means free 1 means hangging
    Target Mode bit 11 0 means physic mode 1 means logical mode

    Deliver Mode 10~8 sames like APIC Delivery mode
    000 Fixed
    001 Lower Priority
    010 SMI
    100 NMI
    101 INIT
    110 Start UP

    Vector bit 7 ~ 0 means page frame number that AP start From
*/

#define MSR_ICR         0x830
#define smp_cpu_id() (CURRENT->cpu_id)

typedef struct IcrEntry{
    unsigned int    vector      :8, //0~7
                    DelivMode   :3, //8~10
                    TarMode     :1, //11
                    DelivStatus :1, //12
                    reservered1 :1, //13
                    DriveLevel  :1, //14
                    Trigger     :1, //15
                    reservered2 :2, //16~17
                    short_hand  :2, //18~19
                    reservered3 :12;//20~31 
    union
    {
        struct x2apic{
            unsigned int target; 
        }__attribute__((packed))x2apic;

        struct apic{
            unsigned int    reservered  :24,    //32~55
                            target      :8;     //56~63
        }__attribute__((packed))apic;
    }delivery_target;

}__attribute__((packed))IcrEntry;


void smp_init();
void start_smp();
void interrupt_cpu(unsigned long nr, unsigned long cpu);

#endif