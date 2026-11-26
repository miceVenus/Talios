#ifndef SCHEDULE_H
#define SCHEDULE_H
#include "task.h"

void scheduler_init(void);
void scheduler_tick(void);
void schedule(void);
void sched_idle(void) __attribute__((noreturn));
void sched_prepare_sleep(void);
long sched_publish(TaskStruct *child);
void sched_exit(void) __attribute__((noreturn));
void sched_finish_switch(TaskStruct *prev);
#endif
