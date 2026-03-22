#ifndef SCHEDULE_H
#define SCHEDULE_H

#include "task.h"

typedef struct scheduler{
    
    long running_task_count;
    long CPU_exec_task_jiffies;
    TaskStruct task_queue;
    long min_vrun_time;

}scheduler;

extern scheduler task_schedulers[NR_CPUS];


void schedule();
void scheduler_init();
struct TaskStruct * get_next_task();
void insert_task_queue(TaskStruct * task);
#endif