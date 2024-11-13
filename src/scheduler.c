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

void init_io_queue(void) {
    scheduler_state.io_queue = malloc(sizeof(IOQueue));
    scheduler_state.io_queue->size = 0;
    pthread_mutex_init(&scheduler_state.io_queue->mutex, NULL);
    pthread_cond_init(&scheduler_state.io_queue->condition, NULL);
}

void cleanup_io_queue(void) {
    pthread_mutex_destroy(&scheduler_state.io_queue->mutex);
    pthread_cond_destroy(&scheduler_state.io_queue->condition);
    free(scheduler_state.io_queue);
}

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
    
    // Inicializar cola de E/S
    init_io_queue();
    
    // Iniciar hilos de control
    pthread_create(&scheduler_state.policy_control_thread, NULL, policy_control_thread, NULL);
    pthread_create(&scheduler_state.io_thread, NULL, io_manager_thread, NULL);
}

void cleanup_scheduler(void) {
    scheduler_state.running = false;
    pthread_join(scheduler_state.policy_control_thread, NULL);
    pthread_join(scheduler_state.io_thread, NULL);
    
    sem_destroy(&scheduler_state.scheduler_sem);
    sem_destroy(&scheduler_state.queue_sem);
    
    for (int i = 0; i < job_queue_size; i++) {
        cleanup_process_semaphores(&job_queue[i]);
    }
    
    cleanup_io_queue();
    free(job_queue);
}

bool check_and_handle_io(ProcessInfo* process) {
    if (process == NULL || process->in_io) return false;
    
    // 5% de probabilidad de entrar a E/S
    if (random_range(1, 100) <= IO_PROBABILITY) {
        printf("\nProceso %d entrando a E/S\n", process->index);
        process->in_io = true;
        add_to_io_queue(process);
        return true;
    }
    return false;
}

void add_to_io_queue(ProcessInfo* process) {
    pthread_mutex_lock(&scheduler_state.io_queue->mutex);
    
    if (scheduler_state.io_queue->size < MAX_IO_QUEUE_SIZE) {
        IOQueueEntry* entry = &scheduler_state.io_queue->entries[scheduler_state.io_queue->size];
        entry->process = process;
        entry->wait_time = random_range(MIN_IO_WAIT, MAX_IO_WAIT);
        entry->start_time = time(NULL);
        scheduler_state.io_queue->size++;
        
        printf("Proceso %d añadido a cola de E/S. Tiempo de espera: %d ms\n", 
               process->index, entry->wait_time);
    }
    
    pthread_mutex_unlock(&scheduler_state.io_queue->mutex);
    pthread_cond_signal(&scheduler_state.io_queue->condition);
}

void process_io_queue(void) {
    pthread_mutex_lock(&scheduler_state.io_queue->mutex);
    
    time_t current_time = time(NULL);
    int i = 0;
    
    while (i < scheduler_state.io_queue->size) {
        IOQueueEntry* entry = &scheduler_state.io_queue->entries[i];
        double elapsed_ms = difftime(current_time, entry->start_time) * 1000;
        
        if (elapsed_ms >= entry->wait_time) {
            // Proceso completó su tiempo de E/S
            printf("Proceso %d completó E/S. Retornando a cola de listos\n", entry->process->index);
            
            entry->process->in_io = false;
            return_to_ready_queue(entry->process);
            
            // Remover de la cola de E/S
            for (int j = i; j < scheduler_state.io_queue->size - 1; j++) {
                scheduler_state.io_queue->entries[j] = scheduler_state.io_queue->entries[j + 1];
            }
            scheduler_state.io_queue->size--;
        } else {
            i++;
        }
    }
    
    pthread_mutex_unlock(&scheduler_state.io_queue->mutex);
}

void return_to_ready_queue(ProcessInfo* process) {
    sem_wait(&scheduler_state.queue_sem);
    
    process->is_running = false;
    process->in_io = false;
    reset_process_timeslice(process);
    
    if (scheduler_state.current_policy == SHORTEST_JOB_FIRST) {
        reorder_ready_queue();
    }
    
    sem_post(&scheduler_state.queue_sem);
}

void reorder_ready_queue(void) {
    sort_processes_sjf(job_queue, job_queue_size, scheduler_state.sort_by_bees);
}

void* io_manager_thread(void* arg) {
    (void)arg;
    
    while (scheduler_state.running) {
        pthread_mutex_lock(&scheduler_state.io_queue->mutex);
        
        while (scheduler_state.io_queue->size == 0 && scheduler_state.running) {
            pthread_cond_wait(&scheduler_state.io_queue->condition, &scheduler_state.io_queue->mutex);
        }
        
        if (!scheduler_state.running) {
            pthread_mutex_unlock(&scheduler_state.io_queue->mutex);
            break;
        }
        
        pthread_mutex_unlock(&scheduler_state.io_queue->mutex);
        
        process_io_queue();
        usleep(1000); // 1ms de espera entre revisiones
    }
    
    return NULL;
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
        if (scheduler_state.active_process != NULL) {
            if (should_preempt_process(scheduler_state.active_process) ||
                check_and_handle_io(scheduler_state.active_process)) {
                preempt_current_process();
            }
        }
        
        sem_post(&scheduler_state.scheduler_sem);
        usleep(50000); // 50ms
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
            process->in_io = false;
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
        if (!job_queue[i].is_running && !job_queue[i].in_io) {
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