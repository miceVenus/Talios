#include "schedule.h"
#include "task.h"
#include "lib.h"
#include "printk.h"


scheduler task_schedulers[NR_CPUS];
extern union TaskUnion *InitTaskUnions[NR_CPUS];

struct List * ListNext(struct List *next);
int ListDelete(struct List *list);
int ListIsEmpty(struct List * list);
void ListForeAdd(struct List *new, struct List *list);

struct TaskStruct * get_next_task(){
    scheduler *task_scheduler = &task_schedulers[CURRENT->cpu_id];
    union TaskUnion *InitTaskUnion = InitTaskUnions[CURRENT->cpu_id];
    TaskStruct * task = NULL;

    if(ListIsEmpty(&task_scheduler->task_queue.list)){
        // ColorPrintfk(BLUE, BLACK, "empty task queue, %X\n", CURRENT->cpu_id);
        task_scheduler->min_vrun_time = InitTaskUnion->task.vrun_time;
        return &InitTaskUnion->task;
    }

    task = ContainerOf(ListNext(&task_scheduler->task_queue.list), struct TaskStruct, list);
    ListDelete(&task->list);
    task_scheduler->running_task_count -= 1;

    if(ListIsEmpty(&task_scheduler->task_queue.list))
        task_scheduler->min_vrun_time = InitTaskUnion->task.vrun_time;
    else
        task_scheduler->min_vrun_time = ContainerOf(ListNext(&task_scheduler->task_queue.list), struct TaskStruct, list)->vrun_time;
        
    return task;
}

void insert_task_queue(TaskStruct * task){
    union TaskUnion *InitTaskUnion = InitTaskUnions[CURRENT->cpu_id];
    scheduler *task_scheduler = &task_schedulers[CURRENT->cpu_id];

    TaskStruct * tmp = ContainerOf(ListNext(&task_scheduler->task_queue.list), TaskStruct, list);
    if(task == &InitTaskUnion->task){
        // ColorPrintfk(BLUE, BLACK, "try to insert IDLE TASK\n");
        return;
    }
    if(ListIsEmpty(&task_scheduler->task_queue.list)){

    }else{
        while(tmp->vrun_time < task->vrun_time)
            tmp = ContainerOf(ListNext(&tmp->list), TaskStruct, list);
    }
    ListForeAdd(&task->list, &tmp->list);
    task_scheduler->running_task_count += 1;
    task_scheduler->min_vrun_time = min(task_scheduler->min_vrun_time, task->vrun_time);   
}

void schedule(){
    scheduler *task_scheduler = &task_schedulers[CURRENT->cpu_id];
    struct TaskStruct *current = CURRENT;
    current->flags &= (~NEED_SCHEDULE);

    struct TaskStruct *task = get_next_task();

    if(current->vrun_time >= task->vrun_time || current->state != TASK_RUNING){

        // if current == task it is means this schedule is not start from interruption
        // so it would break some thing
        if(current == task){
            insert_task_queue(task);
            return;
        }

        if(current->state == TASK_RUNING)
            insert_task_queue(current);
        if(task_scheduler->CPU_exec_task_jiffies <= 0){
            switch(task->priority){
                case 0:
                case 1:
                    task_scheduler->CPU_exec_task_jiffies = 4 / task_scheduler->running_task_count;
                    break;
                
                case 2:
                    task_scheduler->CPU_exec_task_jiffies = 4 / task_scheduler->running_task_count * 3;
                    break;
            }
            // ColorPrintfk(BLUE, BLACK, "schedule happened, %X, %X\n", current, task);
            SWITCH_MM(current, task);
            SWITCH_TO(current, task);
        }
    }else{
        insert_task_queue(task);
        if(!task_scheduler->CPU_exec_task_jiffies){
            switch (task->priority){
                case 0:
                case 1:
                    task_scheduler->CPU_exec_task_jiffies = 4 / task_scheduler->running_task_count;
                    break;
                case 2:
                default:
                    task_scheduler->CPU_exec_task_jiffies = 4 / task_scheduler->running_task_count * 3;
                    break;
            }
        }
    }
}

void scheduler_init(){
    memset(task_schedulers, 0, sizeof(scheduler) * NR_CPUS);
    scheduler *task_scheduler = NULL;
    for(int i = 0; i < NR_CPUS; i++){
        task_scheduler = &task_schedulers[i];
        task_scheduler->CPU_exec_task_jiffies    = 4;
        task_scheduler->running_task_count       = 1;
        ListInit(&task_scheduler->task_queue.list);
        task_scheduler->min_vrun_time = CURRENT->vrun_time;
        task_scheduler->task_queue.vrun_time     = 0x7fffffffffffffff;
    }
}