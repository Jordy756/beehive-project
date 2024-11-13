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

// Control de procesos
void preempt_current_process(void);
void resume_process(ProcessInfo* process);
void suspend_process(ProcessInfo* process);
bool should_preempt_process(ProcessInfo* process);
void reset_process_timeslice(ProcessInfo* process);
void update_process_priority(ProcessInfo* process);

// Control de semáforos
void init_process_semaphores(ProcessInfo* process);
void cleanup_process_semaphores(ProcessInfo* process);

// Nuevas funciones para E/S
void init_io_queue(void);
void cleanup_io_queue(void);
void* io_manager_thread(void* arg);
bool check_and_handle_io(ProcessInfo* process);
void add_to_io_queue(ProcessInfo* process);
void process_io_queue(void);
void return_to_ready_queue(ProcessInfo* process);
void reorder_ready_queue(void);

#endif