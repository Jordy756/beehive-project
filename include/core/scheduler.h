#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../types/scheduler_types.h"

// Inicialización y limpieza
void init_scheduler(void);
void cleanup_scheduler(void);

// Control de política de planificación
void switch_scheduling_policy(void);
void* policy_control_thread(void* arg);
void update_quantum(void);

// Gestión de procesos
void schedule_process(ProcessControlBlock** pcb);
void preempt_current_process(void);
void suspend_process(ProcessInfo* process);
void resume_process(ProcessInfo* process);
void update_process_resources(ProcessInfo* process);
void update_process_priority(ProcessInfo* process);
void reset_process_timeslice(ProcessInfo* process);

// Gestión de cola de trabajo
void update_job_queue(ProcessInfo* processes, int total_processes);
void reorder_ready_queue(void);

// Gestión de FSJ
void sort_processes_fsj(ProcessInfo* processes, int count);
bool should_preempt_fsj(ProcessInfo* new_process, ProcessInfo* current_process);
void check_fsj_preemption(ProcessInfo* process);
void handle_fsj_preemption(void);

// Gestión de E/S
void init_io_queue(void);
void cleanup_io_queue(void);
void* io_manager_thread(void* arg);
bool check_and_handle_io(ProcessInfo* process);
void add_to_io_queue(ProcessInfo* process);
void process_io_queue(void);
void return_to_ready_queue(ProcessInfo* process);

// Gestión de semáforos
void init_process_semaphores(ProcessInfo* process);
void cleanup_process_semaphores(ProcessInfo* process);

// Gestión de estado del proceso
void update_process_state(ProcessInfo* process, ProcessState new_state);

#endif