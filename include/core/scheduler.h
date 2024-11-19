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

// Gestión de colas y procesos
void add_to_ready_queue(ProcessInfo* process);
void remove_from_ready_queue(ProcessInfo* process);
ProcessInfo* get_next_ready_process(void);
void schedule_process(void);

// Gestión de estado de proceso
void update_process_state(ProcessInfo* process, ProcessState new_state);
void preempt_current_process(void);
void resume_process(ProcessInfo* process);

// Gestión de FSJ
void sort_ready_queue_fsj(void);
bool should_preempt_fsj(ProcessInfo* new_process);
void handle_fsj_preemption(void);

// Gestión de E/S
void init_io_queue(void);
void cleanup_io_queue(void);
void* io_manager_thread(void* arg);
void add_to_io_queue(ProcessInfo* process);
void process_io_queue(void);

// Utilidades
void init_process_semaphores(ProcessInfo* process);
void cleanup_process_semaphores(ProcessInfo* process);

#endif