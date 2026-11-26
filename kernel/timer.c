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

static timer_list timer_list_header;
static unsigned long jiffies;
static SpinLock_T timer_lock;

void get_cmos_time(Time * time){

    unsigned long irq = irq_save();

    do{
        time->second = BCD_TO_BIN(CMOS_READ(0x00));
        time->minute = BCD_TO_BIN(CMOS_READ(0x02));
        time->hour = BCD_TO_BIN(CMOS_READ(0x04));
        time->week = BCD_TO_BIN(CMOS_READ(0x06));
        time->day = BCD_TO_BIN(CMOS_READ(0x07));
        time->month = BCD_TO_BIN(CMOS_READ(0x08));
        time->year = BCD_TO_BIN(CMOS_READ(0x09)) + BCD_TO_BIN(CMOS_READ(0x32)) * 100;
    }while (time->second != BCD_TO_BIN(CMOS_READ(0x00)));

    irq_restore(irq);
}



void init_timer(timer_list *timer, void (*func)(void *data), void *data, unsigned long expire_jiffies){
    timer->data = data;
    timer->expire_jiffies = expire_jiffies;
    timer->func = func;
    ListInit(&timer->list);
}

void add_timer(timer_list *timer){
    unsigned long irq = spin_lock_irqsave(&timer_lock);
    struct List *p = timer_list_header.list.next;
    while(p != &timer_list_header.list && ContainerOf(p, timer_list, list)->expire_jiffies < timer->expire_jiffies)
        p = p->next;
    ListForeAdd(&timer->list, p);
    spin_unlock_irqrestore(&timer_lock, irq);
}
void delete_timer(timer_list *timer){
    unsigned long irq = spin_lock_irqsave(&timer_lock);
    ListDelete(&timer->list);
    spin_unlock_irqrestore(&timer_lock, irq);
}

static timer_list *first_expired(void){
    if(ListIsEmpty(&timer_list_header.list)) return NULL;
    timer_list *first = ContainerOf(timer_list_header.list.next, timer_list, list);
    return first->expire_jiffies <= jiffies ? first : NULL;
}

void timer_tick(void){
    unsigned long irq = spin_lock_irqsave(&timer_lock);
    jiffies++;
    if(first_expired()) add_softirq_status(TIME_SIRQ);
    spin_unlock_irqrestore(&timer_lock, irq);
}

void do_time(void *data){
    (void)data;
    unsigned long irq = spin_lock_irqsave(&timer_lock);
    timer_list *timer;
    while((timer = first_expired())){
        void (*func)(void *) = timer->func;
        void *arg = timer->data;
        ListDelete(&timer->list);
        spin_unlock_irqrestore(&timer_lock, irq);
        func(arg);
        irq = spin_lock_irqsave(&timer_lock);
    }
    spin_unlock_irqrestore(&timer_lock, irq);
}

void test_timer(void *datd){
    ColorPrintfk(BLUE, BLACK, "timer is running\n");
}

void time_init(){
    jiffies = 0;
    init_timer(&timer_list_header, NULL, NULL, -1UL);
    register_softirq(0, &do_time, NULL);

    static timer_list boot_timer;
    init_timer(&boot_timer, &test_timer, NULL, 500);
    add_timer(&boot_timer);
}

