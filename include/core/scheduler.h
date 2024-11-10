#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../types/scheduler_types.h"
#include "../types/beehive_types.h"

void init_scheduler(void);
void switch_scheduling_policy(void);
void schedule_process(ProcessControlBlock* pcb);
void update_quantum(void);
void sort_processes_sjf(ProcessInfo* processes, int count, bool by_bees);
void update_job_queue(Beehive** beehives, int total_beehives);

#endif