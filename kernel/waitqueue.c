#include "waitqueue.h"
#include "task.h"
#include "schedule.h"

void wait_queue_init(wait_queue_t * wait_queue, struct TaskStruct *tsk){
    ListInit(&wait_queue->wait_list);
    wait_queue->tsk = tsk;
}

void sleep_on(wait_queue_t * wait_queue_head){
    wait_queue_t wait;
    wait_queue_init(&wait, CURRENT);
    CURRENT->state = TASK_UNINTERRUPTABLE;
    ListForeAdd(&(wait.wait_list), &(wait_queue_head->wait_list));
    schedule();
}

void wake_up(wait_queue_t * wait_queue_head, long state){
    wait_queue_t *wait = NULL;
    if(ListIsEmpty(&wait_queue_head->wait_list)){
        return;
    }
    wait = ContainerOf(ListNext(&wait_queue_head->wait_list), wait_queue_t, wait_list);
    if(wait->tsk->state & state){
        ListDelete(&wait->wait_list);
        wakeup_process(wait->tsk);
    }
}