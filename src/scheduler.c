#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/core/scheduler.h"
#include "../include/core/file_manager.h"
#include "../include/core/utils.h"

ProcessInfo* job_queue = NULL;
int job_queue_size = 0;
SchedulerState scheduler_state;

void init_scheduler(void) {
    // Inicializar estado del planificador
    scheduler_state.current_quantum = random_range(MIN_QUANTUM, MAX_QUANTUM);
    scheduler_state.current_policy = ROUND_ROBIN;
    scheduler_state.quantum_counter = 0;
    scheduler_state.policy_switch_counter = 0;
    scheduler_state.running = true;
    scheduler_state.active_process = NULL;

    // Inicializar semáforos
    sem_init(&scheduler_state.scheduler_sem, 0, 1);
    sem_init(&scheduler_state.queue_sem, 0, 1);
    pthread_mutex_init(&scheduler_state.preemption_mutex, NULL);

    // Inicializar cola de trabajo
    job_queue = malloc(sizeof(ProcessInfo) * MAX_PROCESSES);
    job_queue_size = 0;

    // Inicializar cola de E/S
    init_io_queue();

    // Iniciar hilos de control
    pthread_create(&scheduler_state.policy_control_thread, NULL, policy_control_thread, NULL);
    pthread_create(&scheduler_state.io_thread, NULL, io_manager_thread, NULL);

    printf("Planificador inicializado - Política inicial: %s, Quantum: %d\n", 
           scheduler_state.current_policy == ROUND_ROBIN ? "Round Robin" : "FSJ", 
           scheduler_state.current_quantum);
}

void cleanup_scheduler(void) {
    // Marcar finalización
    scheduler_state.running = false;

    // Despertar hilos bloqueados
    pthread_mutex_lock(&scheduler_state.io_queue->mutex);
    pthread_cond_broadcast(&scheduler_state.io_queue->condition);
    pthread_mutex_unlock(&scheduler_state.io_queue->mutex);

    // Despertar semáforos
    sem_post(&scheduler_state.scheduler_sem);
    sem_post(&scheduler_state.queue_sem);

    // Esperar finalización de hilos
    usleep(100000); // 100ms
    pthread_join(scheduler_state.policy_control_thread, NULL);
    pthread_join(scheduler_state.io_thread, NULL);

    // Limpiar recursos de E/S
    cleanup_io_queue();

    // Destruir semáforos y mutex
    sem_destroy(&scheduler_state.scheduler_sem);
    sem_destroy(&scheduler_state.queue_sem);
    pthread_mutex_destroy(&scheduler_state.preemption_mutex);

    // Limpiar procesos
    for (int i = 0; i < job_queue_size; i++) {
        cleanup_process_semaphores(&job_queue[i]);
    }
    free(job_queue);

    printf("Planificador limpiado correctamente\n");
}

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

bool check_and_handle_io(ProcessInfo* process) {
    if (!process || !process->pcb || process->pcb->state == WAITING) return false;
    
    if (random_range(1, 100) <= IO_PROBABILITY) {
        printf("\nProceso %d entrando a E/S\n", process->index);
        update_process_state(process, WAITING);
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
        process->pcb->current_io_wait_time = random_range(MIN_IO_WAIT, MAX_IO_WAIT);
        entry->wait_time = process->pcb->current_io_wait_time;
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
            printf("Proceso %d completó E/S. Retornando a cola de listos\n", 
                   entry->process->index);
            return_to_ready_queue(entry->process);
            
            // Remover de la cola
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

void* io_manager_thread(void* arg) {
    (void)arg;
    while (scheduler_state.running) {
        pthread_mutex_lock(&scheduler_state.io_queue->mutex);
        while (scheduler_state.io_queue->size == 0 && scheduler_state.running) {
            pthread_cond_wait(&scheduler_state.io_queue->condition, 
                            &scheduler_state.io_queue->mutex);
        }
        
        if (!scheduler_state.running) {
            pthread_mutex_unlock(&scheduler_state.io_queue->mutex);
            break;
        }
        
        pthread_mutex_unlock(&scheduler_state.io_queue->mutex);
        process_io_queue();
        usleep(1000); // 1ms de espera
    }
    return NULL;
}

void return_to_ready_queue(ProcessInfo* process) {
    sem_wait(&scheduler_state.queue_sem);
    update_process_state(process, READY);
    
    if (scheduler_state.current_policy == SHORTEST_JOB_FIRST) {
        sort_processes_fsj(job_queue, job_queue_size);
        check_fsj_preemption(process);
    }
    sem_post(&scheduler_state.queue_sem);
}

void update_process_state(ProcessInfo* process, ProcessState new_state) {
    if (!process || !process->pcb) return;
    
    update_pcb_state(process->pcb, new_state, process->hive);
    if (new_state == RUNNING) {
        process->last_quantum_start = time(NULL);
    }
}

void preempt_current_process(void) {
    if (scheduler_state.active_process != NULL) {
        update_process_state(scheduler_state.active_process, READY);
        scheduler_state.active_process = NULL;
    }
}

void suspend_process(ProcessInfo* process) {
    if (process != NULL) {
        update_process_state(process, READY);
    }
}

void resume_process(ProcessInfo* process) {
    if (process != NULL) {
        update_process_state(process, RUNNING);
    }
}

void sort_processes_fsj(ProcessInfo* processes, int count) {
    // Ordenar por total de recursos (menor a mayor)
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if ((processes[j].hive->bee_count + processes[j].hive->honey_count) >  (processes[j + 1].hive->bee_count + processes[j + 1].hive->honey_count)) {
                ProcessInfo temp = processes[j];
                processes[j] = processes[j + 1];
                processes[j + 1] = temp;
            }
        }
    }
}

bool should_preempt_fsj(ProcessInfo* new_process, ProcessInfo* current_process) {
    if (!new_process || !current_process) return false;
    
    return (new_process->hive->bee_count + new_process->hive->honey_count) < (current_process->hive->bee_count + current_process->hive->honey_count);
}

void check_fsj_preemption(ProcessInfo* process) {
    pthread_mutex_lock(&scheduler_state.preemption_mutex);
    
    if (scheduler_state.current_policy == SHORTEST_JOB_FIRST && 
        scheduler_state.active_process != NULL) {
        if (should_preempt_fsj(process, scheduler_state.active_process)) {
            preempt_current_process();
            sort_processes_fsj(job_queue, job_queue_size);
            scheduler_state.active_process = process;
            resume_process(process);
        }
    }
    
    pthread_mutex_unlock(&scheduler_state.preemption_mutex);
}

void handle_fsj_preemption(void) {
    static time_t last_preemption_check = 0;
    time_t current_time = time(NULL);
    
    if (difftime(current_time, last_preemption_check) < 2.0) {
        return;
    }
    
    pthread_mutex_lock(&scheduler_state.preemption_mutex);
    
    if (scheduler_state.active_process != NULL) {
        double elapsed = difftime(current_time, 
                                scheduler_state.active_process->last_quantum_start);
        if (elapsed < 1.0) {
            pthread_mutex_unlock(&scheduler_state.preemption_mutex);
            return;
        }
        
        bool should_preempt = false;
        ProcessInfo* lowest_resource_process = NULL;
        
        for (int i = 0; i < job_queue_size; i++) {
            if (job_queue[i].pcb->state == READY) {
                if (lowest_resource_process == NULL || (job_queue[i].hive->bee_count + job_queue[i].hive->honey_count) < (lowest_resource_process->hive->bee_count + lowest_resource_process->hive->honey_count)) {
                    lowest_resource_process = &job_queue[i];
                    should_preempt = true;
                }
            }
        }
        
        if (should_preempt && lowest_resource_process != NULL) {
            if ((lowest_resource_process->hive->bee_count + lowest_resource_process->hive->honey_count) < 
            (scheduler_state.active_process->hive->bee_count + scheduler_state.active_process->hive->honey_count)) {
                printf("\nFSJ Preemption: Proceso %d (recursos: %d) interrumpe a Proceso %d (recursos: %d)\n",
                       lowest_resource_process->index,
                       (lowest_resource_process->hive->bee_count + lowest_resource_process->hive->honey_count),
                       scheduler_state.active_process->index,
                       (scheduler_state.active_process->hive->bee_count + scheduler_state.active_process->hive->honey_count));
                
                preempt_current_process();
                sort_processes_fsj(job_queue, job_queue_size);
                scheduler_state.active_process = lowest_resource_process;
                resume_process(lowest_resource_process);
            }
        }
    }
    
    last_preemption_check = current_time;
    pthread_mutex_unlock(&scheduler_state.preemption_mutex);
}

void switch_scheduling_policy(void) {
    scheduler_state.current_policy = (scheduler_state.current_policy == ROUND_ROBIN) ? 
                                    SHORTEST_JOB_FIRST : ROUND_ROBIN;
    
    printf("\nCambiando política de planificación a: %s\n",
           scheduler_state.current_policy == ROUND_ROBIN ? 
           "Round Robin" : "Shortest Job First (FSJ)");
           
    if (scheduler_state.current_policy == SHORTEST_JOB_FIRST) {
        sort_processes_fsj(job_queue, job_queue_size);
    }
}

bool should_preempt_process(ProcessInfo* process) {
    if (process == NULL) return false;
    
    time_t current_time = time(NULL);
    double elapsed = difftime(current_time, process->last_quantum_start);
    
    if (scheduler_state.current_policy == ROUND_ROBIN) {
        return elapsed >= scheduler_state.current_quantum;
    } else if (scheduler_state.current_policy == SHORTEST_JOB_FIRST) {
        // Verificar si hay procesos con menos recursos
        for (int i = 0; i < job_queue_size; i++) {
            if (job_queue[i].pcb->state == READY) {
                if (should_preempt_fsj(&job_queue[i], process)) {
                    return true;
                }
            }
        }
    }
    return false;
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

void update_quantum(void) {
    scheduler_state.current_quantum = random_range(MIN_QUANTUM, MAX_QUANTUM);
    printf("\nNuevo Quantum: %d\n", scheduler_state.current_quantum);
}

void init_process_semaphores(ProcessInfo* process) {
    process->shared_resource_sem = malloc(sizeof(sem_t));
    sem_init(process->shared_resource_sem, 0, 1);
    process->last_quantum_start = time(NULL);
}

void update_job_queue(ProcessInfo* processes, int total_processes) {
    sem_wait(&scheduler_state.queue_sem);
    
    // Limpiar semáforos antiguos
    for (int i = 0; i < job_queue_size; i++) {
        cleanup_process_semaphores(&job_queue[i]);
    }
    
    // Actualizar cola con procesos activos
    job_queue_size = 0;
    for (int i = 0; i < total_processes; i++) {
        if (processes[i].hive != NULL) {
            memcpy(&job_queue[job_queue_size], &processes[i], sizeof(ProcessInfo));
            ProcessInfo* process = &job_queue[job_queue_size];
            
            // Inicializar semáforos y estados
            init_process_semaphores(process);
            process->pcb->state = READY;
            job_queue_size++;
        }
    }
    
    // Ordenar si es FSJ
    if (scheduler_state.current_policy == SHORTEST_JOB_FIRST && job_queue_size > 1) {
        sort_processes_fsj(job_queue, job_queue_size);
    }
    
    sem_post(&scheduler_state.queue_sem);
}

void schedule_process(ProcessControlBlock** pcb) {
    sem_wait(&scheduler_state.scheduler_sem);
    scheduler_state.policy_switch_counter++;
    ProcessInfo* next_process = NULL;
    static time_t last_preemption_check = 0;
    time_t current_time = time(NULL);
    
    if (scheduler_state.current_policy == SHORTEST_JOB_FIRST) {
        if (scheduler_state.active_process != NULL) {
            double elapsed = difftime(current_time, 
                                   scheduler_state.active_process->last_quantum_start);
            if (elapsed < 1.0) {
                sem_post(&scheduler_state.scheduler_sem);
                return;
            }
        }
        
        if (difftime(current_time, last_preemption_check) >= 2.0) {
            for (int i = 0; i < job_queue_size; i++) {
                if (job_queue[i].pcb->state == READY) {
                    if (scheduler_state.active_process == NULL ||
                        should_preempt_fsj(&job_queue[i], scheduler_state.active_process)) {
                        next_process = &job_queue[i];
                        break;
                    }
                }
            }
            last_preemption_check = current_time;
        }
    } else { // Round Robin
        for (int i = 0; i < job_queue_size; i++) {
            if (job_queue[i].pcb->state == READY) {
                next_process = &job_queue[i];
                break;
            }
        }
    }
    
    if (next_process != NULL) {
        if (scheduler_state.active_process != NULL) {
            if (scheduler_state.current_policy == SHORTEST_JOB_FIRST) {
                if ((next_process->hive->bee_count + next_process->hive->honey_count) >= (scheduler_state.active_process->hive->bee_count + scheduler_state.active_process->hive->honey_count)) {
                    sem_post(&scheduler_state.scheduler_sem);
                    return;
                }
            }
            preempt_current_process();
        }
        
        scheduler_state.active_process = next_process;
        resume_process(next_process);
        *pcb = next_process->pcb;  // Asignar el puntero al PCB
    }
    
    sem_post(&scheduler_state.scheduler_sem);
}

void cleanup_process_semaphores(ProcessInfo* process) {
    if (process->shared_resource_sem) {
        sem_destroy(process->shared_resource_sem);
        free(process->shared_resource_sem);
        process->shared_resource_sem = NULL;
    }
}