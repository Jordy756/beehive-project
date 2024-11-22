#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../types/scheduler_types.h"

// Inicialización y limpieza
void init_scheduler(void);// Inicializar el planificador
void cleanup_scheduler(void);// Limpiar el planificador

// Control de política de planificación
void switch_scheduling_policy(void);// Cambiar la política de planificación
void* policy_control_thread(void* arg);// El hilo del control de la política de planificación
void update_quantum(void);// Actualizar la quantum del planificador

// Gestión de colas y procesos
void add_to_ready_queue(ProcessInfo* process);// Añadir un proceso a la cola de procesos
void remove_from_ready_queue(ProcessInfo* process);// Eliminar un proceso de la cola de procesos
ProcessInfo* get_next_ready_process(void);// Obtener el siguiente proceso en la cola de procesos
void schedule_process(void);// Programar el siguiente proceso en la cola de procesos

// Gestión de estado de proceso
void update_process_state(ProcessInfo* process, ProcessState new_state);// Actualizar el estado de un proceso
void preempt_current_process(ProcessState new_state);// Preemptuar el proceso actual
void resume_process(ProcessInfo* process);// Reanudar un proceso suspendido

// Gestión de FSJ
void sort_ready_queue_fsj(void);// Ordenar la cola de procesos según su prioridad
bool should_preempt_fsj(ProcessInfo* new_process);// Comprobar si se debe preemptuar el proceso actual
void handle_fsj_preemption(void);// Manejar la preemptión del proceso actual según la política de planificación

// Gestión de E/S
void init_io_queue(void);// Inicializar la cola de E/S
void cleanup_io_queue(void);// Limpiar la cola de E/S
void* io_manager_thread(void* arg);// El hilo del gestor de E/S
void add_to_io_queue(ProcessInfo* process);// Añadir un proceso a la cola de E/S
void remove_from_io_queue(int index);// Eliminar un proceso de la cola de E/S
void process_io_queue(void);// Procesar la cola de E/S

// Utilidades
void init_process_semaphores(ProcessInfo* process);// Inicializar los semáforos de un proceso
void cleanup_process_semaphores(ProcessInfo* process);// Limpiar los semáforos de un proceso

#endif