#ifndef TIMER_H
#define TIMER_H


#include "lib.h"
/*
    Real Time Clock (RTC)
    
    Stored In Binary Coded Decimal as Default

    RTC index register In IO port 0x70
    RTC data register In IO port 0x71
    
    index table

    0x00 second 0~59
    0x02 minute 0~59
    0x04 hour   12H; 1~12, 24H 0~23
    0x06 week   1~7
    0x07 day    1~31
    0x08 month  1~12
    0x09 year   0~99
    0x32 century 0~99

*/

typedef struct Time{
    unsigned int second;
    unsigned int minute;
    unsigned int hour;
    unsigned int week;
    unsigned int day;
    unsigned int month;
    unsigned int year;      // 0x09 + 0x32
}Time;

typedef struct timer_list{
    struct List list;
    unsigned long expire_jiffies;
    void (*func)(void *data);
    void *data;
}timer_list;

void init_timer(timer_list *timer, void (*func)(void *data), void *data, unsigned long expire_jiffies);
void add_timer(timer_list *list);
void delete_timer(timer_list *list);

void get_cmos_time(Time * time);
void time_init();

#endif