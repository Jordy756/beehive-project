#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../types/scheduler_types.h"
#include "../types/beehive_types.h"

// Inicialización y limpieza
void init_scheduler(void);
void cleanup_scheduler(void);

// Control de política de planificación
void switch_scheduling_policy(void);
void* policy_control_thread(void* arg);

// Funciones de planificación
void schedule_process(ProcessControlBlock* pcb);
void update_quantum(void);
void sort_processes_sjf(ProcessInfo* processes, int count, bool by_bees);
void update_job_queue(Beehive** beehives, int total_beehives);

// Control de semáforos
void init_process_semaphores(ProcessInfo* process);
void cleanup_process_semaphores(ProcessInfo* process);

#endif