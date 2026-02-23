#include "softirq.h"
#include "lib.h"


unsigned long softirq_status = 0; // A status register

softirq softirq_vector[64] = {0};

void add_softirq_status(unsigned long status){
    softirq_status |= status;
}

unsigned long get_softirq_status(){
    return softirq_status;
}

void register_softirq(int nr, void (*action)(void * data), void *data){
    softirq_vector[nr].action = action;
    softirq_vector[nr].data = data;
}

void unregitser_softirq(int nr){
    softirq_vector[nr].action   = NULL;
    softirq_vector[nr].data     = NULL;
}

void softirq_init(){
    softirq_status = 0;
    memset(softirq_vector, 0, sizeof(softirq) * 64);
}

void do_softirq(){
    sti();
    for(int i = 0; i < 64; i++){
        if(!GetBits(softirq_status, i, 1)) continue;
        softirq_status &= ~(1 << i);
        softirq_vector[i].action(softirq_vector[i].data);
    }
    cli();
}