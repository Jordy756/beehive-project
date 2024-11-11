#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/core/scheduler.h"
#include "../include/core/utils.h"

// Variables globales
ProcessInfo* job_queue = NULL;
int job_queue_size = 0;
SchedulerState scheduler_state;

void switch_scheduling_policy(void) {
    scheduler_state.current_policy = (scheduler_state.current_policy == ROUND_ROBIN) ? 
                                   SHORTEST_JOB_FIRST : ROUND_ROBIN;
    
    if (scheduler_state.current_policy == SHORTEST_JOB_FIRST) {
        scheduler_state.sort_by_bees = !scheduler_state.sort_by_bees;
    }
    
    printf("\nCambiando política de planificación a: %s",
           scheduler_state.current_policy == ROUND_ROBIN ? "Round Robin" : "Shortest Job First");
    if (scheduler_state.current_policy == SHORTEST_JOB_FIRST) {
        printf(" (Ordenando por: %s)", scheduler_state.sort_by_bees ? "Abejas" : "Miel");
    }
    printf("\n");
}

void sort_processes_sjf(ProcessInfo* processes, int count, bool by_bees) {
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            bool should_swap = false;
            
            if (by_bees) {
                should_swap = processes[j].hive->bee_count > processes[j + 1].hive->bee_count;
            } else {
                should_swap = processes[j].hive->honey_count > processes[j + 1].hive->honey_count;
            }
            
            if (should_swap) {
                ProcessInfo temp = processes[j];
                processes[j] = processes[j + 1];
                processes[j + 1] = temp;
            }
        }
    }
}

void init_scheduler(void) {
    // Inicializar estado del planificador
    scheduler_state.current_quantum = random_range(MIN_QUANTUM, MAX_QUANTUM);
    scheduler_state.current_policy = ROUND_ROBIN;
    scheduler_state.quantum_counter = 0;
    scheduler_state.policy_switch_counter = 0;
    scheduler_state.sort_by_bees = true;
    scheduler_state.running = true;
    scheduler_state.active_process = NULL;

    // Inicializar semáforos
    sem_init(&scheduler_state.scheduler_sem, 0, 1);
    sem_init(&scheduler_state.queue_sem, 0, 1);
    
    // Inicializar cola de trabajos
    job_queue = malloc(sizeof(ProcessInfo) * MAX_PROCESSES);
    job_queue_size = 0;
    
    // Iniciar hilo de control de política
    pthread_create(&scheduler_state.policy_control_thread, NULL, policy_control_thread, NULL);
}

void cleanup_scheduler(void) {
    scheduler_state.running = false;
    pthread_join(scheduler_state.policy_control_thread, NULL);
    
    sem_destroy(&scheduler_state.scheduler_sem);
    sem_destroy(&scheduler_state.queue_sem);
    
    for (int i = 0; i < job_queue_size; i++) {
        cleanup_process_semaphores(&job_queue[i]);
    }
    
    free(job_queue);
}

void preempt_current_process(void) {
    if (scheduler_state.active_process != NULL) {
        suspend_process(scheduler_state.active_process);
        scheduler_state.active_process = NULL;
    }
}

void suspend_process(ProcessInfo* process) {
    if (process != NULL) {
        process->is_running = false;
        sem_post(process->shared_resource_sem);
    }
}

void resume_process(ProcessInfo* process) {
    if (process != NULL) {
        process->is_running = true;
        process->last_quantum_start = time(NULL);
        reset_process_timeslice(process);
    }
}

bool should_preempt_process(ProcessInfo* process) {
    if (process == NULL) return false;
    
    time_t current_time = time(NULL);
    double elapsed = difftime(current_time, process->last_quantum_start);
    
    if (scheduler_state.current_policy == ROUND_ROBIN) {
        return elapsed >= scheduler_state.current_quantum;
    }
    return false;
}

void reset_process_timeslice(ProcessInfo* process) {
    if (process != NULL) {
        process->remaining_time_slice = PROCESS_TIME_SLICE;
    }
}

void update_process_priority(ProcessInfo* process) {
    if (process != NULL) {
        // Prioridad basada en tiempo de espera y recursos
        int wait_time = (int)difftime(time(NULL), process->last_quantum_start);
        process->priority = wait_time + (process->hive->bee_count / 10);
    }
}

void* policy_control_thread(void* arg) {
    (void)arg;
    
    while (scheduler_state.running) {
        sem_wait(&scheduler_state.scheduler_sem);
        
        if (scheduler_state.policy_switch_counter >= POLICY_SWITCH_THRESHOLD) {
            switch_scheduling_policy();
            scheduler_state.policy_switch_counter = 0;
        }
        
        if (scheduler_state.current_policy == ROUND_ROBIN &&
            scheduler_state.quantum_counter >= QUANTUM_UPDATE_INTERVAL) {
            update_quantum();
            scheduler_state.quantum_counter = 0;
        }
        
        // Verificar si el proceso actual debe ser interrumpido
        if (scheduler_state.active_process != NULL &&
            should_preempt_process(scheduler_state.active_process)) {
            preempt_current_process();
        }
        
        sem_post(&scheduler_state.scheduler_sem);
        usleep(50000); // Reducido a 50ms para mayor responsividad
    }
    
    return NULL;
}

void update_job_queue(Beehive** beehives, int total_beehives) {
    sem_wait(&scheduler_state.queue_sem);
    
    // Limpiar semáforos antiguos
    for (int i = 0; i < job_queue_size; i++) {
        cleanup_process_semaphores(&job_queue[i]);
    }
    
    job_queue_size = 0;
    
    // Agregar colmenas activas a la cola
    for (int i = 0; i < total_beehives; i++) {
        if (beehives[i] != NULL) {
            ProcessInfo* process = &job_queue[job_queue_size];
            process->hive = beehives[i];
            process->index = i;
            process->is_running = false;
            process->remaining_time_slice = PROCESS_TIME_SLICE;
            init_process_semaphores(process);
            update_process_priority(process);
            job_queue_size++;
        }
    }
    
    // Ordenar si estamos usando SJF
    if (scheduler_state.current_policy == SHORTEST_JOB_FIRST && job_queue_size > 1) {
        sort_processes_sjf(job_queue, job_queue_size, scheduler_state.sort_by_bees);
    }
    
    sem_post(&scheduler_state.queue_sem);
}

void schedule_process(ProcessControlBlock* pcb) {
    sem_wait(&scheduler_state.scheduler_sem);
    
    scheduler_state.policy_switch_counter++;
    
    // Encontrar el siguiente proceso a ejecutar
    ProcessInfo* next_process = NULL;
    
    for (int i = 0; i < job_queue_size; i++) {
        if (!job_queue[i].is_running) {
            next_process = &job_queue[i];
            break;
        }
    }
    
    if (next_process != NULL) {
        if (scheduler_state.active_process != NULL) {
            preempt_current_process();
        }
        
        scheduler_state.active_process = next_process;
        resume_process(next_process);
        
        // Actualizar PCB
        pcb->process_id = next_process->index;
        pcb->iterations++;
    }
    
    sem_post(&scheduler_state.scheduler_sem);
}

void init_process_semaphores(ProcessInfo* process) {
    process->shared_resource_sem = malloc(sizeof(sem_t));
    sem_init(process->shared_resource_sem, 0, 1);
    process->last_quantum_start = time(NULL);
}

void cleanup_process_semaphores(ProcessInfo* process) {
    if (process->shared_resource_sem) {
        sem_destroy(process->shared_resource_sem);
        free(process->shared_resource_sem);
        process->shared_resource_sem = NULL;
    }
}

void update_quantum(void) {
    scheduler_state.current_quantum = random_range(MIN_QUANTUM, MAX_QUANTUM);
    printf("\nNuevo Quantum: %d\n", scheduler_state.current_quantum);
}