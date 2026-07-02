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

    // long flags = get_rflags();
    // cli();

    scheduler *task_scheduler = &task_schedulers[CURRENT->cpu_id];
    union TaskUnion *InitTaskUnion = InitTaskUnions[CURRENT->cpu_id];
    TaskStruct * task = NULL;

    if(ListIsEmpty(&task_scheduler->task_queue.list)){
        // ColorPrintfk(BLUE, BLACK, "empty task queue, %X\n", CURRENT->cpu_id);
        task_scheduler->min_vrun_time = CURRENT->vrun_time;
        task = &InitTaskUnion->task;
        goto out;
    }

    task = ContainerOf(ListNext(&task_scheduler->task_queue.list), struct TaskStruct, list);
    ListDelete(&task->list);
    task_scheduler->running_task_count -= 1;

    // task_scheduler->min_vrun_time = ContainerOf(ListNext(&task_scheduler->task_queue.list), struct TaskStruct, list)->vrun_time;
    
    task_scheduler->min_vrun_time = min(task->vrun_time, CURRENT->vrun_time);
    // if(ListIsEmpty(&task_scheduler->task_queue.list))
    //     task_scheduler->min_vrun_time = task->vrun_time;
    // else
    //     task_scheduler->min_vrun_time = ContainerOf(ListNext(&task_scheduler->task_queue.list), struct TaskStruct, list)->vrun_time;

    out:
        // if(flags & 0x200) sti();
        return task;
}

void insert_task_queue(TaskStruct * task){
    // long flags = get_rflags();
    // cli();

    union TaskUnion *InitTaskUnion = InitTaskUnions[CURRENT->cpu_id];
    scheduler *task_scheduler = &task_schedulers[CURRENT->cpu_id];

    TaskStruct * tmp = ContainerOf(ListNext(&task_scheduler->task_queue.list), TaskStruct, list);
    if(task == &InitTaskUnion->task){
        goto out;
    }
    if(ListIsEmpty(&task_scheduler->task_queue.list)){

    }else{
        while(tmp->vrun_time <= task->vrun_time){
            if(tmp == task) goto out;
            tmp = ContainerOf(ListNext(&tmp->list), TaskStruct, list);
        }
    }
    ListForeAdd(&task->list, &tmp->list);

    task_scheduler->running_task_count += 1;
    task_scheduler->min_vrun_time = min(task_scheduler->min_vrun_time, task->vrun_time);
    
    out:

        return;
        // if(flags & 0x200) 
        //     sti();
}

void schedule(){

    CURRENT->preempt_count++;
    scheduler *task_scheduler = &task_schedulers[CURRENT->cpu_id];
    struct TaskStruct *current = CURRENT;
    current->flags &= (~NEED_SCHEDULE);

    struct TaskStruct *task = get_next_task();

    if(current->vrun_time >= task->vrun_time || current->state != TASK_RUNING){

        // if current == task it is means this schedule is not start from interruption
        // so it would break some thing
        if(current == task){
            goto no_switch_out;
        }

        // make task priority has its sense for now we have not perserve task`s cpu time
        task_scheduler->CPU_exec_task_jiffies = 0;
        
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
        }

        SWITCH_MM(current, task);
        SWITCH_TO(current, task);

        return;
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

    no_switch_out:
        CURRENT->preempt_count--;
        return;
}

void scheduler_init(){
    memset(task_schedulers, 0, sizeof(scheduler) * NR_CPUS);
    scheduler *task_scheduler = NULL;
    for(int i = 0; i < NR_CPUS; i++){
        task_scheduler = &task_schedulers[i];
        task_scheduler->CPU_exec_task_jiffies    = 4;
        task_scheduler->running_task_count       = 1;
        ListInit(&task_scheduler->task_queue.list);
        task_scheduler->min_vrun_time            = CURRENT->vrun_time;
        task_scheduler->task_queue.vrun_time     = 0x7fffffffffffffff;
    }
}