#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "beehive_types.h"

#define MAX_PROCESSES 40

typedef enum {
    ROUND_ROBIN,
    SHORTEST_JOB_FIRST
} SchedulingPolicy;

typedef struct {
    ProcessControlBlock* pcb;
    Beehive* hive;
    int index;  // Index in beehives array
} ProcessInfo;

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