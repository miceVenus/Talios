#include "timer.h"
#include "lib.h"
#include "printk.h"
#include "memory.h"
#include "softirq.h"
#include "task.h"

#define CMOS_READ(index) ({         \
    OUT8b(0x70, 0x80 | (index));    \
    IN8b(0x71);                     \
})

void ListInit(struct List *list);
void ListForeAdd(struct List *new, struct List *list);
void ListBackAdd(struct List *list, struct List *new);
struct List* ListNext(struct List *list);
struct List* ListPrev(struct List *list);
int ListIsEmpty(struct List *list);
int ListDelete(struct List *list);

Time global_time = {0};

timer_list timer_list_header;

extern unsigned long jiffies;

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



void init_timer(timer_list *timer, void (*func)(void *data), void *data, unsigned long expire_jiffies){
    timer->data = data;
    timer->expire_jiffies = expire_jiffies;
    timer->func = func;
    ListInit(&timer->list);
}

void add_timer(timer_list *timer){

    if(ListIsEmpty(&timer_list_header.list)){
        ListBackAdd(&timer_list_header.list, &timer->list);
        return;
    }

    timer_list *tmp = ContainerOf(ListNext(&timer_list_header.list), timer_list, list);

    // here timer_list_header `s expire num is infinite so there is no end problem
    while(tmp->expire_jiffies < timer->expire_jiffies){
        tmp = ContainerOf(ListNext(&tmp->list), timer_list, list);
    }

    ListForeAdd(&timer->list, &tmp->list);
}
void delete_timer(timer_list *timer){
    ListDelete(&timer->list);
}

// This is softirq

void do_time(void *data){
    timer_list *tmp = ContainerOf(ListNext(&timer_list_header.list), timer_list, list);
    while(tmp->expire_jiffies <= jiffies && !ListIsEmpty(&timer_list_header.list)){
        delete_timer(tmp);
        tmp->func(tmp->data);
        tmp = ContainerOf(ListNext(&timer_list_header.list), timer_list, list);
        // kfree(tmp); // should be removed
    }
}

void test_timer(void *datd){
    ColorPrintfk(BLUE, BLACK, "timer is running\n");
}

void time_init(){
    jiffies = 0;
    init_timer(&timer_list_header, NULL, NULL, -1UL);
    register_softirq(0, &do_time, NULL);

    timer_list *tmp = (timer_list*)kmalloc(sizeof(timer_list), 0);
    init_timer(tmp, &test_timer, NULL, 500);
    add_timer(tmp);
}

