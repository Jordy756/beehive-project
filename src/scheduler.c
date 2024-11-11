#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/core/scheduler.h"
#include "../include/core/utils.h"

// Define global variables
ProcessInfo* job_queue = NULL;
int job_queue_size = 0;
SchedulerState scheduler_state;

void init_scheduler(void) {
    // Inicializar estado del planificador
    scheduler_state.current_quantum = random_range(MIN_QUANTUM, MAX_QUANTUM);
    scheduler_state.current_policy = ROUND_ROBIN;
    scheduler_state.quantum_counter = 0;
    scheduler_state.policy_switch_counter = 0;
    scheduler_state.sort_by_bees = true;
    scheduler_state.running = true;

    // Inicializar semáforo general
    sem_init(&scheduler_state.scheduler_sem, 0, 1);
    
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
    
    // Limpiar semáforos de procesos
    for (int i = 0; i < job_queue_size; i++) {
        cleanup_process_semaphores(&job_queue[i]);
    }
    
    free(job_queue);
}

void* policy_control_thread(void* arg) {
    (void)arg;  // Evitar warning de parámetro no usado
    
    while (scheduler_state.running) {
        sem_wait(&scheduler_state.scheduler_sem);
        
        // Verificar si es tiempo de cambiar la política
        if (scheduler_state.policy_switch_counter >= POLICY_SWITCH_THRESHOLD) {
            switch_scheduling_policy();
            scheduler_state.policy_switch_counter = 0;
        }
        
        // En RR, verificar si es tiempo de actualizar el quantum
        if (scheduler_state.current_policy == ROUND_ROBIN && 
            scheduler_state.quantum_counter >= QUANTUM_UPDATE_INTERVAL) {
            update_quantum();
            scheduler_state.quantum_counter = 0;
        }
        
        sem_post(&scheduler_state.scheduler_sem);
        sleep(1);  // Esperar 1 segundo antes de la siguiente verificación
    }
    
    return NULL;
}

void switch_scheduling_policy(void) {
    scheduler_state.current_policy = (scheduler_state.current_policy == ROUND_ROBIN) ? SHORTEST_JOB_FIRST : ROUND_ROBIN;
    
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

void update_job_queue(Beehive** beehives, int total_beehives) {
    sem_wait(&scheduler_state.scheduler_sem);
    
    // Limpiar semáforos antiguos
    for (int i = 0; i < job_queue_size; i++) {
        cleanup_process_semaphores(&job_queue[i]);
    }
    
    job_queue_size = 0;
    
    // Agregar colmenas activas a la cola
    for (int i = 0; i < total_beehives; i++) {
        if (beehives[i] != NULL) {
            job_queue[job_queue_size].hive = beehives[i];
            job_queue[job_queue_size].index = i;
            init_process_semaphores(&job_queue[job_queue_size]);
            job_queue_size++;
        }
    }
    
    // Ordenar si estamos usando SJF
    if (scheduler_state.current_policy == SHORTEST_JOB_FIRST && job_queue_size > 1) {
        sort_processes_sjf(job_queue, job_queue_size, scheduler_state.sort_by_bees);
    }
    
    sem_post(&scheduler_state.scheduler_sem);
}

void schedule_process(ProcessControlBlock* pcb) {
    sem_wait(&scheduler_state.scheduler_sem);
    
    scheduler_state.policy_switch_counter++;
    
    if (scheduler_state.current_policy == ROUND_ROBIN) {
        scheduler_state.quantum_counter++;
        
        // Verificar si el proceso actual ha consumido su quantum
        for (int i = 0; i < job_queue_size; i++) {
            if (job_queue[i].pcb == pcb) {
                time_t current_time = time(NULL);
                if (difftime(current_time, job_queue[i].last_quantum_start) >= scheduler_state.current_quantum) {
                    // Resetear el tiempo de inicio del quantum
                    job_queue[i].last_quantum_start = current_time;
                    printf("\nQuantum consumido para proceso %d\n", pcb->process_id);
                }
                break;
            }
        }
    }
    
    pcb->iterations++;
    sem_post(&scheduler_state.scheduler_sem);
}

void update_quantum(void) {
    scheduler_state.current_quantum = random_range(MIN_QUANTUM, MAX_QUANTUM);
    printf("\nNuevo Quantum: %d\n", scheduler_state.current_quantum);
}