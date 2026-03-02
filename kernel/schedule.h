#ifndef SCHEDULE_H
#define SCHEDULE_H

#include "task.h"

typedef struct scheduler{
    
    long running_task_count;
    long CPU_exec_task_jiffies;
    TaskStruct task_queue;

}scheduler;



void schedule();
void scheduler_init();

#endif