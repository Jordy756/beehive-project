#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../types/scheduler_types.h"
#include "../types/beehive_types.h"

void init_scheduler(void);
void cleanup_scheduler(void);  // Nueva función para limpieza
void switch_scheduling_policy(void);
void schedule_process(ProcessControlBlock* pcb);
void update_quantum(void);
SchedulingPolicy get_current_policy(void);
int get_current_quantum(void);
void sort_processes_sjf(ProcessInfo* processes, int count, bool by_bees);
void update_process_info(ProcessInfo* info, ProcessControlBlock* pcb, Beehive* hive, int index);
void update_job_queue(Beehive** beehives, int total_beehives);

// Nuevas funciones para el hilo de control
void start_scheduler_control(void);
void request_policy_change(SchedulingPolicy new_policy);
void* scheduler_control_thread(void* arg);

#endif