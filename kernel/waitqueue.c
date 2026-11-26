#include "waitqueue.h"
#include "task.h"
#include "schedule.h"

void wait_queue_init(wait_queue_t * wait_queue, struct TaskStruct *tsk){
    ListInit(&wait_queue->wait_list);
    wait_queue->tsk = tsk;
}

/* Called with interrupts disabled and the condition lock held. */
void sleep_on(wait_queue_t *queue, SpinLock_T *condition_lock){
    wait_queue_t wait;
    wait_queue_init(&wait, CURRENT);
    ListForeAdd(&wait.wait_list, &queue->wait_list);
    sched_prepare_sleep();
    raw_spin_unlock(condition_lock);
    schedule();
    raw_spin_lock(condition_lock);
}

void wake_up(wait_queue_t *queue){
    if(ListIsEmpty(&queue->wait_list)) return;
    wait_queue_t *wait = ContainerOf(queue->wait_list.next, wait_queue_t, wait_list);
    ListDelete(&wait->wait_list);
    wakeup_process(wait->tsk);
}
