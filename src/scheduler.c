#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/core/scheduler.h"
#include "../include/core/file_manager.h"
#include "../include/core/utils.h"

// Instancia del estado del planificador
SchedulerState scheduler_state;

// Funciones de utilidad privadas
static bool is_queue_empty(ReadyQueue* queue) {
    return queue->size == 0;
}

static bool is_queue_full(ReadyQueue* queue) {
    return queue->size >= MAX_PROCESSES;
}

// Inicialización de semáforos y recursos
void init_process_semaphores(ProcessInfo* process) {
    if (!process) return;
    process->shared_resource_sem = malloc(sizeof(sem_t));
    sem_init(process->shared_resource_sem, 0, 1);
    process->last_quantum_start = time(NULL);
}

// Limpia y destruye los semáforos asociados a un proceso
void cleanup_process_semaphores(ProcessInfo* process) {
    if (!process || !process->shared_resource_sem) return;
    sem_destroy(process->shared_resource_sem);
    free(process->shared_resource_sem);
    process->shared_resource_sem = NULL;
}

// Gestión de cola de listos
void add_to_ready_queue(ProcessInfo* process) {
    if (!process || !scheduler_state.ready_queue) return;

    pthread_mutex_lock(&scheduler_state.ready_queue->mutex);
    
    if (!is_queue_full(scheduler_state.ready_queue)) {
        scheduler_state.ready_queue->processes[scheduler_state.ready_queue->size] = process;
        scheduler_state.ready_queue->size++;
        
        if (scheduler_state.current_policy == SHORTEST_JOB_FIRST) {
            sort_ready_queue_fsj();
        }
    }

    scheduler_state.process_table->ready_processes = scheduler_state.ready_queue->size;
    
    pthread_mutex_unlock(&scheduler_state.ready_queue->mutex);
}

// Elimina un proceso de la cola de listos
void remove_from_ready_queue(ProcessInfo* process) {
    if (!process || !scheduler_state.ready_queue) return;

    int index = -1;
    for (int i = 0; i < scheduler_state.ready_queue->size; i++) {
        if (scheduler_state.ready_queue->processes[i] == process) {
            index = i;
            break;
        }
    }
    
    if (index != -1) {
        for (int i = index; i < scheduler_state.ready_queue->size - 1; i++) {
            scheduler_state.ready_queue->processes[i] = scheduler_state.ready_queue->processes[i + 1];
        }
        scheduler_state.ready_queue->size--;
    }

    scheduler_state.process_table->ready_processes = scheduler_state.ready_queue->size;
}

// Obtiene el siguiente proceso en la cola de listos
ProcessInfo* get_next_ready_process(void) {
    pthread_mutex_lock(&scheduler_state.ready_queue->mutex);
    
    ProcessInfo* next_process = NULL;
    if (!is_queue_empty(scheduler_state.ready_queue)) {
        next_process = scheduler_state.ready_queue->processes[0];
        remove_from_ready_queue(next_process);
    }
    
    pthread_mutex_unlock(&scheduler_state.ready_queue->mutex);
    return next_process;
}

// Gestión de cola de E/S
void init_io_queue(void) {
    scheduler_state.io_queue = malloc(sizeof(IOQueue));
    scheduler_state.io_queue->size = 0;
    pthread_mutex_init(&scheduler_state.io_queue->mutex, NULL);
    pthread_cond_init(&scheduler_state.io_queue->condition, NULL);
}

//Se Limpian los recursos asociados a la cola de entrada/salida
void cleanup_io_queue(void) {
    if (!scheduler_state.io_queue) return;
    pthread_mutex_destroy(&scheduler_state.io_queue->mutex);
    pthread_cond_destroy(&scheduler_state.io_queue->condition);
    free(scheduler_state.io_queue);
}

//Se añade un proceso a la cola de entrada/salida
void add_to_io_queue(ProcessInfo* process) {
    if (!process || !scheduler_state.io_queue) return;

    pthread_mutex_lock(&scheduler_state.io_queue->mutex);
    
    if (scheduler_state.io_queue->size < MAX_IO_QUEUE_SIZE) {
        IOQueueEntry* entry = &scheduler_state.io_queue->entries[scheduler_state.io_queue->size];
        entry->process = process;
        process->pcb->current_io_wait_time = random_range(MIN_IO_WAIT, MAX_IO_WAIT);
        entry->wait_time = process->pcb->current_io_wait_time;
        entry->start_time = time(NULL);
        scheduler_state.io_queue->size++;
        
        printf("Proceso %d añadido a cola de E/S. Tiempo de espera: %d ms\n", process->index, entry->wait_time);
    }

    scheduler_state.process_table->io_waiting_processes = scheduler_state.io_queue->size;
    
    pthread_mutex_unlock(&scheduler_state.io_queue->mutex);
    pthread_cond_signal(&scheduler_state.io_queue->condition);
}

//Se elimina un proceso a la cola de entrada/salida
void remove_from_io_queue(int index) {
    for (int j = index; j < scheduler_state.io_queue->size - 1; j++) {
        scheduler_state.io_queue->entries[j] = scheduler_state.io_queue->entries[j + 1];
    }
    scheduler_state.io_queue->size--;
    scheduler_state.process_table->io_waiting_processes = scheduler_state.io_queue->size;
}

// Este procesa los procesos en la cola de entrada/salida.
void process_io_queue(void) {
    pthread_mutex_lock(&scheduler_state.io_queue->mutex);
    
    time_t current_time = time(NULL);
    int i = 0;
    
    while (i < scheduler_state.io_queue->size) {
        IOQueueEntry* entry = &scheduler_state.io_queue->entries[i];
        double elapsed_ms = difftime(current_time, entry->start_time) * 1000;
        
        if (elapsed_ms >= entry->wait_time) {
            ProcessInfo* process = entry->process;
            
            // Eliminar de la cola de E/S
            remove_from_io_queue(i);
            
            // Actualizar estado y añadir a cola de listos
            update_process_state(process, READY);
            add_to_ready_queue(process);
            
            printf("Proceso %d completó E/S\n", process->index);
        } else {
            i++;
        }
    }
    
    pthread_mutex_unlock(&scheduler_state.io_queue->mutex);
}

//Gestión del hilo de entrada/salida
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
        delay_ms(1);
    }
    
    return NULL;
}

// Gestión de FSJ
void sort_ready_queue_fsj(void) {
    ReadyQueue* queue = scheduler_state.ready_queue;
    
    for (int i = 0; i < queue->size - 1; i++) {
        for (int j = 0; j < queue->size - i - 1; j++) {
            ProcessInfo* p1 = queue->processes[j];
            ProcessInfo* p2 = queue->processes[j + 1];
            
            if (p1->hive->bees_and_honey_count > p2->hive->bees_and_honey_count) {
                ProcessInfo* temp = queue->processes[j];
                queue->processes[j] = queue->processes[j + 1];
                queue->processes[j + 1] = temp;
            }
        }
    }
}

//Alternancia del proceso
bool should_preempt_fsj(ProcessInfo* new_process) {
    if (!new_process || !scheduler_state.active_process) return false;
    
    return new_process->hive->bees_and_honey_count < scheduler_state.active_process->hive->bees_and_honey_count;
}

//Manejo de la alternancia con FSJ 
void handle_fsj_preemption(void) {
    if (!scheduler_state.active_process) return;
    
    pthread_mutex_lock(&scheduler_state.scheduler_mutex);
    
    ProcessInfo* current = scheduler_state.active_process;
    ProcessInfo* next = get_next_ready_process();
    
    if (next && should_preempt_fsj(next)) {
        preempt_current_process(READY);
        add_to_ready_queue(current);
        scheduler_state.active_process = next;
        resume_process(next);
    } else if (next) {
        add_to_ready_queue(next);
    }
    
    pthread_mutex_unlock(&scheduler_state.scheduler_mutex);
}

// Gestión de procesos
void update_process_state(ProcessInfo* process, ProcessState new_state) {
    if (!process || !process->pcb) return;
    
    ProcessState old_state = process->pcb->state;
    update_pcb_state(process->pcb, new_state, process->hive);
    
    if (new_state == RUNNING) {
        process->last_quantum_start = time(NULL);
    } else if (old_state == RUNNING && new_state == READY) {
        process->last_quantum_start = 0;
    }
}

// Gestión principal de planificación
void preempt_current_process(ProcessState new_state) {
    if (scheduler_state.active_process) {
        ProcessInfo* process = scheduler_state.active_process;
        update_process_state(process, new_state);
        scheduler_state.active_process = NULL;
    }
}

void resume_process(ProcessInfo* process) {
    if (!process) return;
    update_process_state(process, RUNNING);
}

// Planificación principal
void schedule_process(void) {
    pthread_mutex_lock(&scheduler_state.scheduler_mutex);
    
    // Si no hay proceso activo, obtener el siguiente
    if (!scheduler_state.active_process) {
        ProcessInfo* next = get_next_ready_process();
        if (next) {
            scheduler_state.active_process = next;
            resume_process(next);
        }
    } else {
        time_t now = time(NULL);
        ProcessInfo* current = scheduler_state.active_process;
        
        // Verificar si el proceso actual necesita E/S
        if (random_range(1, 100) <= IO_PROBABILITY) {
            printf("Proceso %d requiere E/S\n", current->index);
            preempt_current_process(WAITING);
            add_to_io_queue(current);
            
            // Obtener siguiente proceso
            ProcessInfo* next = get_next_ready_process();
            if (next) {
                scheduler_state.active_process = next;
                resume_process(next);
            }
            pthread_mutex_unlock(&scheduler_state.scheduler_mutex);
            return;
        }
        
        // Verificar quantum en RR
        if (scheduler_state.current_policy == ROUND_ROBIN) {
            double elapsed = difftime(now, current->last_quantum_start);
            if (elapsed >= scheduler_state.current_quantum) {
                printf("Quantum expirado para proceso %d\n", current->index);
                preempt_current_process(READY);
                add_to_ready_queue(current);
                
                // Obtener siguiente proceso
                ProcessInfo* next = get_next_ready_process();
                if (next) {
                    scheduler_state.active_process = next;
                    resume_process(next);
                }
            }
        }
    }
    
    pthread_mutex_unlock(&scheduler_state.scheduler_mutex);
}

// Control de política
void update_quantum(void) {
    time_t current_time = time(NULL);
    if (difftime(current_time, scheduler_state.last_quantum_update) >= QUANTUM_UPDATE_INTERVAL) {
        scheduler_state.current_quantum = random_range(MIN_QUANTUM, MAX_QUANTUM);
        scheduler_state.last_quantum_update = current_time;
        printf("\nNuevo Quantum: %d segundos\n", scheduler_state.current_quantum);
    }
}

// Cambio de política de planificación 
void switch_scheduling_policy(void) {
    pthread_mutex_lock(&scheduler_state.scheduler_mutex);
    
    scheduler_state.current_policy = (scheduler_state.current_policy == ROUND_ROBIN) ? SHORTEST_JOB_FIRST : ROUND_ROBIN;
    scheduler_state.last_policy_switch = time(NULL);
    
    if (scheduler_state.current_policy == SHORTEST_JOB_FIRST) {
        sort_ready_queue_fsj();
    }
    
    printf("\nCambiando política de planificación a: %s\n", scheduler_state.current_policy == ROUND_ROBIN ? "Round Robin" : "Shortest Job First (FSJ)");
    
    pthread_mutex_unlock(&scheduler_state.scheduler_mutex);
}

void* policy_control_thread(void* arg) {
    (void)arg;
    
    while (scheduler_state.running) {
        time_t current_time = time(NULL);
        
        // Cambio de política
        if (difftime(current_time, scheduler_state.last_policy_switch) >= POLICY_SWITCH_THRESHOLD) {
            switch_scheduling_policy();
        }
        
        // Actualización de quantum en RR
        if (scheduler_state.current_policy == ROUND_ROBIN) {
            update_quantum();
        } else {
            handle_fsj_preemption();
        }
        
        schedule_process();
        delay_ms(1000);
    }
    
    return NULL;
}

// Inicialización y limpieza
void init_scheduler(void) {
    // Inicializar estado
    scheduler_state.current_policy = ROUND_ROBIN;
    scheduler_state.current_quantum = random_range(MIN_QUANTUM, MAX_QUANTUM);
    scheduler_state.last_quantum_update = time(NULL);
    scheduler_state.last_policy_switch = time(NULL);
    scheduler_state.running = true;
    scheduler_state.active_process = NULL;
    
    // Inicializa mutex y semáforos
    pthread_mutex_init(&scheduler_state.scheduler_mutex, NULL);
    sem_init(&scheduler_state.scheduler_sem, 0, 1);
    
    // Inicializa cola de listos
    scheduler_state.ready_queue = malloc(sizeof(ReadyQueue));
    scheduler_state.ready_queue->size = 0;
    pthread_mutex_init(&scheduler_state.ready_queue->mutex, NULL);
    
    // Inicializa cola de E/S
    init_io_queue();
    
    // Inicializa tabla de procesos
    scheduler_state.process_table = malloc(sizeof(ProcessTable));
    init_process_table(scheduler_state.process_table);
    
    printf("Planificador inicializado - Política: %s, Quantum: %d\n", scheduler_state.current_policy == ROUND_ROBIN ? "Round Robin" : "FSJ", scheduler_state.current_quantum);
    
    // Inicia los hilos
    pthread_create(&scheduler_state.policy_control_thread, NULL, policy_control_thread, NULL);
    pthread_create(&scheduler_state.io_thread, NULL, io_manager_thread, NULL);
}

void cleanup_scheduler(void) {
    scheduler_state.running = false;
    
    // DEspierta hilos bloqueados
    pthread_mutex_lock(&scheduler_state.io_queue->mutex);
    pthread_cond_broadcast(&scheduler_state.io_queue->condition);
    pthread_mutex_unlock(&scheduler_state.io_queue->mutex);
    
    // Espera la finalización de los hilos
    pthread_join(scheduler_state.policy_control_thread, NULL);
    pthread_join(scheduler_state.io_thread, NULL);
    
    // Limpia los recursos
    pthread_mutex_destroy(&scheduler_state.scheduler_mutex);
    sem_destroy(&scheduler_state.scheduler_sem);
    
    // Limpia la cola de listos
    pthread_mutex_destroy(&scheduler_state.ready_queue->mutex);
    free(scheduler_state.ready_queue);
    
    // Limpia la tabla de procesos
    free(scheduler_state.process_table);
    
    // Limpia la cola de E/S
    cleanup_io_queue();
    
    printf("Planificador limpiado correctamente\n");
}