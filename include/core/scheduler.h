#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdbool.h>
#include "../types/scheduler_types.h"

void init_scheduler(void);
void switch_scheduling_policy(void);
void schedule_process(ProcessControlBlock* pcb);
void update_quantum(void);
SchedulingPolicy get_current_policy(void);
int get_current_quantum(void);
void sort_processes_sjf(ProcessInfo* processes, int count, bool by_bees);
void update_process_info(ProcessInfo* info, ProcessControlBlock* pcb, Beehive* hive, int index);
void update_job_queue(Beehive** beehives, int total_beehives);

// Declare external variables that will be defined in scheduler.c
extern ProcessInfo* job_queue;
extern int job_queue_size;

#endif