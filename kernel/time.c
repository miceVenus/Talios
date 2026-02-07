#include "time.h"
#include "lib.h"

#define CMOS_READ(index) ({         \
    OUT8b(0x70, 0x80 | (index));    \
    IN8b(0x71);                     \
})

Time global_time = {0};

void get_cmos_time(Time * time){

    cli();

    do{
        time->second = BCD_TO_BIN(CMOS_READ(0x00));
        time->minute = BCD_TO_BIN(CMOS_READ(0x02));
        time->hour = BCD_TO_BIN(CMOS_READ(0x04));
        time->week = BCD_TO_BIN(CMOS_READ(0x06));
        time->day = BCD_TO_BIN(CMOS_READ(0x07));
        time->month = BCD_TO_BIN(CMOS_READ(0x08));
        time->year = BCD_TO_BIN(CMOS_READ(0x09)) + BCD_TO_BIN(CMOS_READ(0x32)) * 100;
    }while (time->second != BCD_TO_BIN(CMOS_READ(0x00)));

    sti();
}