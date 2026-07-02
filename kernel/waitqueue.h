#ifndef WAITQUEUE_H
#define WAITQUEUE_H

#include "lib.h"

typedef struct wait_queue_t{
    struct List wait_list;
    struct TaskStruct *tsk;
}wait_queue_t;


void wait_queue_init(wait_queue_t * wait_queue, struct TaskStruct *tsk);
void wake_up(wait_queue_t * wait_queue, long state);
void sleep_on(wait_queue_t * wait_queue_head);


#endif