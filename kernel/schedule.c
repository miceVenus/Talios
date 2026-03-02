#include "schedule.h"
#include "task.h"
#include "lib.h"


scheduler task_scheduler;
extern union TaskUnion InitTaskUnion;

struct List * ListNext(struct List *next);
void ListDelete(struct List *list);
int ListIsEmpty(struct List * list);
void ListForeAdd(struct List *new, struct List *list);

struct TaskStruct * get_next_task(){
    TaskStruct * task = NULL;
    if(ListIsEmpty(&task_scheduler.task_queue.list))
        return &InitTaskUnion.task;
    
    task = ContainerOf(ListNext(&task->list), struct TaskStruct, list);
    ListDelete(&task->list);
    task_scheduler.running_task_count -= 1;
    return task;
}

void insert_task_queue(TaskStruct * task){
    TaskStruct * tmp = ContainerOf(ListNext(&task_scheduler.task_queue.list), TaskStruct, list);
    if(task == &InitTaskUnion.task)
        return;
    if(ListIsEmpty(&task_scheduler.task_queue.list)){

    }else{
        while(tmp->vrun_time < task->vrun_time)
            tmp = ContainerOf(ListNext(&tmp->list), TaskStruct, list);
    }
    ListForeAdd(&task->list, &tmp->list);
    task_scheduler.running_task_count += 1;
    
}

void schedule(){
    struct TaskStruct *current = CURRENT;
    current->flags &= (~NEED_SCHEDULE);

    struct TaskStruct *task = get_next_task();
    if(current->vrun_time >= task->vrun_time){
        if(current->state == TASK_RUNING)
            insert_task_queue(current);
        if(!task_scheduler.CPU_exec_task_jiffies){
            switch(task->priority){
                case 0:
                case 1:
                    task_scheduler.CPU_exec_task_jiffies = 4 / task_scheduler.running_task_count;
                    break;
                
                case 2:
                    task_scheduler.CPU_exec_task_jiffies = 4 / task_scheduler.running_task_count * 3;
                    break;
            }

            SWITCH_TO(current, task);
        }
    }else{
        insert_task_queue(task);
        if(!task_scheduler.CPU_exec_task_jiffies){
            switch (task->priority){
                case 0:
                case 1:
                    task_scheduler.CPU_exec_task_jiffies = 4 / task_scheduler.running_task_count;
                    break;
                case 2:
                default:
                    task_scheduler.CPU_exec_task_jiffies = 4 / task_scheduler.running_task_count * 3;
                    break;
            }
        }
    }
}

void scheduler_init(){
    memset(&task_scheduler, 0, sizeof(scheduler));
    task_scheduler.CPU_exec_task_jiffies    = 4;
    task_scheduler.running_task_count       = 1;
    ListInit(&task_scheduler.task_queue.list);
    task_scheduler.task_queue.vrun_time     = 0x7fffffffffffffff;
}