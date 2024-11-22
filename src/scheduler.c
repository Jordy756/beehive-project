#include <stdio.h> // Biblioteca de entrada/salida estándar
#include <stdlib.h> // Biblioteca de funciones de uso general
#include <string.h> // Biblioteca de strings
#include <unistd.h> // Biblioteca de llamadas al sistema
#include "../include/core/scheduler.h" // Planificador
#include "../include/core/file_manager.h" // Gestión de archivos
#include "../include/core/utils.h" // Utilidades

// Instancia del estado del planificador
SchedulerState scheduler_state;

// Funciones de utilidad privadas
static bool is_queue_empty(ReadyQueue* queue) { // Verifica si la cola de listos está vacía
    return queue->size == 0; // Devuelve si la cola de listos está vacía
}

// Verifica si la cola de E/S está llena
static bool is_queue_full(ReadyQueue* queue) { 
    return queue->size >= MAX_PROCESSES; // Devuelve si la cola de listos está llena
}

// Inicialización de semáforos y recursos
void init_process_semaphores(ProcessInfo* process) {
    if (!process) return; // Si no hay bloque de control de procesos, devuelve
    process->shared_resource_sem = malloc(sizeof(sem_t)); // Crea un semáforo compartido
    sem_init(process->shared_resource_sem, 0, 1); // Inicializa el semáforo
    process->last_quantum_start = time(NULL); // Obtiene la hora de inicio del quantum
}

// Limpia y destruye los semáforos asociados a un proceso
void cleanup_process_semaphores(ProcessInfo* process) {
    if (!process || !process->shared_resource_sem) return; // Si no hay bloque de control de procesos o semáforo compartido, devuelve
    sem_destroy(process->shared_resource_sem); // Destruye el semáforo compartido
    free(process->shared_resource_sem); // Libera el semáforo compartido
    process->shared_resource_sem = NULL; // Libera la memoria del semáforo compartido
}

// Gestión de cola de listos
void add_to_ready_queue(ProcessInfo* process) {
    if (!process || !scheduler_state.ready_queue) return; // Si no hay bloque de control de procesos o cola de listos, devuelve

    pthread_mutex_lock(&scheduler_state.ready_queue->mutex); // Bloquea el mutex para el acceso a la cola de listos
    
    if (!is_queue_full(scheduler_state.ready_queue)) { // Si la cola de listos no está llena
        scheduler_state.ready_queue->processes[scheduler_state.ready_queue->size] = process; // Añade el proceso a la cola de listos
        scheduler_state.ready_queue->size++; // Incrementa el número de procesos en la cola de listos
        
        if (scheduler_state.current_policy == SHORTEST_JOB_FIRST) { // Si la política es FSJ
            sort_ready_queue_fsj(); // Ordena la cola de listos
        }
    }

    scheduler_state.process_table->ready_processes = scheduler_state.ready_queue->size; // Actualiza la tabla de procesos
    
    pthread_mutex_unlock(&scheduler_state.ready_queue->mutex); // Desbloquea el mutex para el acceso a la cola de listos
}

// Elimina un proceso de la cola de listos
void remove_from_ready_queue(ProcessInfo* process) {
    if (!process || !scheduler_state.ready_queue) return; // Si no hay bloque de control de procesos o cola de listos, devuelve

    int index = -1; // Obtiene el índice del proceso en la cola de listos
    for (int i = 0; i < scheduler_state.ready_queue->size; i++) { // Recorre la cola de listos
        if (scheduler_state.ready_queue->processes[i] == process) { // Si el proceso está en la cola de listos
            index = i; // Actualiza el índice
            break; // Salir de la bucle
        }
    }
    
    if (index != -1) { // Si se ha encontrado el proceso en la cola de listos
        for (int i = index; i < scheduler_state.ready_queue->size - 1; i++) { // Recorre la cola de listos
            scheduler_state.ready_queue->processes[i] = scheduler_state.ready_queue->processes[i + 1]; // Mueve el proceso a la posición anterior
        }
        scheduler_state.ready_queue->size--; // Decrementa el número de procesos en la cola de listos
    }

    scheduler_state.process_table->ready_processes = scheduler_state.ready_queue->size; // Actualiza la tabla de procesos
}

// Obtiene el siguiente proceso en la cola de listos
ProcessInfo* get_next_ready_process(void) {
    pthread_mutex_lock(&scheduler_state.ready_queue->mutex); // Bloquea el mutex para el acceso a la cola de listos
    
    ProcessInfo* next_process = NULL; // Proceso siguiente
    if (!is_queue_empty(scheduler_state.ready_queue)) { // Si la cola de listos no está vacía
        next_process = scheduler_state.ready_queue->processes[0]; // Obtiene el siguiente proceso
        remove_from_ready_queue(next_process); // Elimina el proceso de la cola de listos
    }
    
    pthread_mutex_unlock(&scheduler_state.ready_queue->mutex); // Desbloquea el mutex para el acceso a la cola de listos
    return next_process; // Retorna el siguiente proceso
}

// Gestión de cola de E/S
void init_io_queue(void) {
    scheduler_state.io_queue = malloc(sizeof(IOQueue)); // Crea la cola de E/S
    scheduler_state.io_queue->size = 0; // Inicializa el tamaño de la cola de E/S
    pthread_mutex_init(&scheduler_state.io_queue->mutex, NULL); // Crea el mutex para el acceso a la cola de E/S
    pthread_cond_init(&scheduler_state.io_queue->condition, NULL); // Crea la condición para espera de E/S
}

//Se Limpian los recursos asociados a la cola de entrada/salida
void cleanup_io_queue(void) {
    if (!scheduler_state.io_queue) return; // Si no hay cola de E/S, devuelve
    pthread_mutex_destroy(&scheduler_state.io_queue->mutex); // Libera el mutex para el acceso a la cola de E/S
    pthread_cond_destroy(&scheduler_state.io_queue->condition); // Libera la condición para espera de E/S
    free(scheduler_state.io_queue); // Libera la cola de E/S
}

//Se añade un proceso a la cola de entrada/salida
void add_to_io_queue(ProcessInfo* process) {
    if (!process || !scheduler_state.io_queue) return; // Si no hay bloque de control de procesos o cola de E/S, devuelve

    pthread_mutex_lock(&scheduler_state.io_queue->mutex); // Bloquea el mutex para el acceso a la cola de E/S
    
    if (scheduler_state.io_queue->size < MAX_IO_QUEUE_SIZE) { // Si la cola de E/S no está llena
        IOQueueEntry* entry = &scheduler_state.io_queue->entries[scheduler_state.io_queue->size]; // Obtiene el índice del proceso en la cola de E/S
        entry->process = process; // Añade el proceso a la cola de E/S
        process->pcb->current_io_wait_time = random_range(MIN_IO_WAIT, MAX_IO_WAIT); // Obtiene el tiempo promedio de espera de E/S
        entry->wait_time = process->pcb->current_io_wait_time; // Añade el tiempo promedio de espera de E/S a la cola de E/S
        entry->start_time = time(NULL); // Obtiene la hora de inicio de la cola de E/S
        scheduler_state.io_queue->size++; // Incrementa el número de procesos en la cola de E/S
        
        printf("Proceso %d añadido a cola de E/S. Tiempo de espera: %d ms\n", process->index, entry->wait_time); // Imprime un mensaje de debug
    }

    scheduler_state.process_table->io_waiting_processes = scheduler_state.io_queue->size; // Actualiza la tabla de procesos
    
    pthread_mutex_unlock(&scheduler_state.io_queue->mutex); // Desbloquea el mutex para el acceso a la cola de E/S
    pthread_cond_signal(&scheduler_state.io_queue->condition); // Señaliza la cola de E/S
}

//Se elimina un proceso a la cola de entrada/salida
void remove_from_io_queue(int index) {
    for (int j = index; j < scheduler_state.io_queue->size - 1; j++) { // Recorre la cola de E/S
        scheduler_state.io_queue->entries[j] = scheduler_state.io_queue->entries[j + 1]; // Mueve el proceso a la posición anterior
    }
    scheduler_state.io_queue->size--; // Decrementa el número de procesos en la cola de E/S
    scheduler_state.process_table->io_waiting_processes = scheduler_state.io_queue->size; // Actualiza la tabla de procesos
}

// Este procesa los procesos en la cola de entrada/salida.
void process_io_queue(void) {
    pthread_mutex_lock(&scheduler_state.io_queue->mutex); // Bloquea el mutex para el acceso a la cola de E/S
    
    time_t current_time = time(NULL); // Obtiene la hora actual
    int i = 0; // Índice del proceso en la cola de E/S
    
    while (i < scheduler_state.io_queue->size) { // Recorre la cola de E/S
        IOQueueEntry* entry = &scheduler_state.io_queue->entries[i]; // Obtiene el índice del proceso en la cola de E/S
        double elapsed_ms = difftime(current_time, entry->start_time) * 1000; // Obtiene el tiempo transcurrido desde la última vez que se inició el proceso
        
        if (elapsed_ms >= entry->wait_time) { // Si el tiempo transcurrido es mayor o igual al tiempo promedio de espera de E/S
            ProcessInfo* process = entry->process; // Obtiene el proceso
            
            remove_from_io_queue(i); // Eliminar de la cola de E/S
            
            update_process_state(process, READY); // Actualizar estado y añadir a cola de listos
            add_to_ready_queue(process); // Añadir al cola de listos
            
            printf("Proceso %d completó E/S\n", process->index); // Imprime un mensaje de debug
        } else {
            i++; // Incrementa el índice del proceso en la cola de E/S
        }
    }
    
    pthread_mutex_unlock(&scheduler_state.io_queue->mutex); // Desbloquea el mutex para el acceso a la cola de E/S
}

//Gestión del hilo de entrada/salida
void* io_manager_thread(void* arg) {
    (void)arg; // Ignora el argumento pasado al hilo
    
    while (scheduler_state.running) { // Mientras la cola de E/S no esté vacía y la cola de listos no esté llena
        pthread_mutex_lock(&scheduler_state.io_queue->mutex); // Bloquea el mutex para el acceso a la cola de E/S
        
        while (scheduler_state.io_queue->size == 0 && scheduler_state.running) { // Mientras la cola de E/S esté vacía y la cola de listos no esté llena
            pthread_cond_wait(&scheduler_state.io_queue->condition, &scheduler_state.io_queue->mutex); // Espera a que se añada un proceso a la cola de E/S
        }
        
        if (!scheduler_state.running) { // Si la cola de E/S está vacía y la cola de listos no está llena
            pthread_mutex_unlock(&scheduler_state.io_queue->mutex); // Desbloquea el mutex para el acceso a la cola de E/S
            break;
        }
        
        pthread_mutex_unlock(&scheduler_state.io_queue->mutex); // Desbloquea el mutex para el acceso a la cola de E/S
        process_io_queue(); // Procesa la cola de E/S
        delay_ms(1); // Espera 1 ms
    }
    
    return NULL;
}

// Gestión de FSJ
void sort_ready_queue_fsj(void) {
    ReadyQueue* queue = scheduler_state.ready_queue; // Obtiene la cola de listos
    
    for (int i = 0; i < queue->size - 1; i++) { // Recorre la cola de listos
        for (int j = 0; j < queue->size - i - 1; j++) { // Recorre la cola de listos
            ProcessInfo* p1 = queue->processes[j]; // Obtiene el proceso 1
            ProcessInfo* p2 = queue->processes[j + 1]; // Obtiene el proceso 2
            
            if (p1->hive->bees_and_honey_count > p2->hive->bees_and_honey_count) { // Si el número de abejas es mayor que el de la colmena
                ProcessInfo* temp = queue->processes[j]; // Intercambia los procesos
                queue->processes[j] = queue->processes[j + 1]; // Intercambia los procesos
                queue->processes[j + 1] = temp; // Intercambia los procesos
            }
        }
    }
}

//Alternancia del proceso
bool should_preempt_fsj(ProcessInfo* new_process) {
    if (!new_process || !scheduler_state.active_process) return false; // Si no hay bloque de control de procesos o proceso activo, devuelve falso
    
    return new_process->hive->bees_and_honey_count < scheduler_state.active_process->hive->bees_and_honey_count; // Retorna verdadero si el número de abejas del nuevo proceso es menor que el de la colmena actual
}

//Manejo de la alternancia con FSJ 
void handle_fsj_preemption(void) {
    if (!scheduler_state.active_process) return; // Si no hay proceso activo, devuelve
    
    pthread_mutex_lock(&scheduler_state.scheduler_mutex); // Bloquea el mutex para el acceso al proceso activo
    
    ProcessInfo* current = scheduler_state.active_process; // Obtiene el proceso activo
    ProcessInfo* next = get_next_ready_process(); // Obtiene el siguiente proceso en la cola de listos
    
    if (next && should_preempt_fsj(next)) { // Si el siguiente proceso es menor que el de la colmena actual y debe ser preemptivo
        preempt_current_process(READY); // Preemptiva el proceso activo
        add_to_ready_queue(current); // Añade el proceso activo a la cola de listos
        scheduler_state.active_process = next; // Actualiza el proceso activo
        resume_process(next); // Resume el proceso
    } else if (next) { // Si el siguiente proceso es menor que el de la colmena actual
        add_to_ready_queue(next); // Añade el siguiente proceso a la cola de listos
    }
    
    pthread_mutex_unlock(&scheduler_state.scheduler_mutex); // Desbloquea el mutex para el acceso al proceso activo
}

// Gestión de procesos
void update_process_state(ProcessInfo* process, ProcessState new_state) {
    if (!process || !process->pcb) return; // Si no hay bloque de control de procesos o bloque de control de procesos de proceso, devuelve
    
    ProcessState old_state = process->pcb->state; // Obtiene el estado anterior del proceso
    update_pcb_state(process->pcb, new_state, process->hive); // Actualiza el estado del bloque de control de procesos
    
    if (new_state == RUNNING) { // Si el nuevo estado es RUNNING
        process->last_quantum_start = time(NULL); // Obtiene la hora de inicio del quantum
    } else if (old_state == RUNNING && new_state == READY) { // Si el estado anterior era RUNNING y el nuevo es READY
        process->last_quantum_start = 0; // Obtiene la hora de inicio del quantum
    }
}

// Gestión principal de planificación
void preempt_current_process(ProcessState new_state) {
    if (scheduler_state.active_process) { // Si hay proceso activo
        ProcessInfo* process = scheduler_state.active_process; // Obtiene el proceso activo
        update_process_state(process, new_state); // Actualiza el estado del proceso
        scheduler_state.active_process = NULL; // Libera el proceso activo
    }
}

void resume_process(ProcessInfo* process) {
    if (!process) return; // Si no hay bloque de control de procesos, devuelve
    update_process_state(process, RUNNING); // Actualiza el estado del proceso
}

// Planificación principal
void schedule_process(void) {
    pthread_mutex_lock(&scheduler_state.scheduler_mutex); // Bloquea el mutex para el acceso al proceso activo
    
    if (!scheduler_state.active_process) { // Si no hay proceso activo
        ProcessInfo* next = get_next_ready_process(); // Obtiene el siguiente proceso
        if (next) {
            scheduler_state.active_process = next; // Actualiza el proceso activo
            resume_process(next); // Resume el proceso
        }
    } else { // Si hay proceso activo
        time_t now = time(NULL); // Obtiene la hora actual
        ProcessInfo* current = scheduler_state.active_process; // Obtiene el proceso activo
        
        if (random_range(1, 100) <= IO_PROBABILITY) { // Verificar si el proceso actual necesita E/S
            printf("Proceso %d requiere E/S\n", current->index); // Imprime un mensaje de debug
            preempt_current_process(WAITING); // Preemptiva el proceso activo
            add_to_io_queue(current); // Añade el proceso activo a la cola de E/S
            
            ProcessInfo* next = get_next_ready_process(); // Obtiene el siguiente proceso
            if (next) { // Si hay siguiente proceso
                scheduler_state.active_process = next; // Actualiza el proceso activo
                resume_process(next); // Resume el proceso
            }
            pthread_mutex_unlock(&scheduler_state.scheduler_mutex); // Desbloquea el mutex para el acceso al proceso activo
            return; // Salir de la función
        }
        
        if (scheduler_state.current_policy == ROUND_ROBIN) { // Verificar quantum en RR
            double elapsed = difftime(now, current->last_quantum_start); // Obtiene el tiempo transcurrido desde la última vez que se inició el quantum
            if (elapsed >= scheduler_state.current_quantum) { // Si ha transcurrido el tiempo de quantum
                printf("Quantum expirado para proceso %d\n", current->index); // Imprime un mensaje de debug
                preempt_current_process(READY); // Preemptiva el proceso activo
                add_to_ready_queue(current); // Añade el proceso activo a la cola de listos
                
                ProcessInfo* next = get_next_ready_process(); // Obtiene el siguiente proceso
                if (next) { // Si hay siguiente proceso
                    scheduler_state.active_process = next; // Actualiza el proceso activo
                    resume_process(next); // Resume el proceso
                }
            }
        }
    }
    
    pthread_mutex_unlock(&scheduler_state.scheduler_mutex); // Desbloquea el mutex para el acceso al proceso activo
}

// Control de política
void update_quantum(void) {
    time_t current_time = time(NULL); // Obtiene la hora actual
    if (difftime(current_time, scheduler_state.last_quantum_update) >= QUANTUM_UPDATE_INTERVAL) { // Si ha transcurrido un tiempo suficiente desde la última actualización de quantum
        scheduler_state.current_quantum = random_range(MIN_QUANTUM, MAX_QUANTUM); // Obtiene un nuevo quantum aleatorio
        scheduler_state.last_quantum_update = current_time; // Actualiza la hora de última actualización de quantum
        printf("\nNuevo Quantum: %d segundos\n", scheduler_state.current_quantum); // Imprime un mensaje de debug
    }
}

// Cambio de política de planificación 
void switch_scheduling_policy(void) {
    pthread_mutex_lock(&scheduler_state.scheduler_mutex); // Bloquea el mutex para el acceso al proceso activo
    
    scheduler_state.current_policy = (scheduler_state.current_policy == ROUND_ROBIN) ? SHORTEST_JOB_FIRST : ROUND_ROBIN; // Cambia la política de planificación
    scheduler_state.last_policy_switch = time(NULL); // Obtiene la hora de última vez que cambió de política
    
    if (scheduler_state.current_policy == SHORTEST_JOB_FIRST) { // Si la política es FSJ
        sort_ready_queue_fsj(); // Ordena la cola de listos
    }
    
    printf("\nCambiando política de planificación a: %s\n", scheduler_state.current_policy == ROUND_ROBIN ? "Round Robin" : "Shortest Job First (FSJ)"); // Imprime un mensaje de debug
    
    pthread_mutex_unlock(&scheduler_state.scheduler_mutex); // Desbloquea el mutex para el acceso al proceso activo
}

void* policy_control_thread(void* arg) {
    (void)arg; // Ignora el argumento pasado al hilo
    
    while (scheduler_state.running) { // Mientras la cola de E/S no esté vacía y la cola de listos no esté llena
        time_t current_time = time(NULL); // Obtiene la hora actual
        
        if (difftime(current_time, scheduler_state.last_policy_switch) >= POLICY_SWITCH_THRESHOLD) { // Si ha transcurrido un tiempo suficiente desde la última vez que cambió de política
            switch_scheduling_policy(); // Cambia la política de planificación
        }
        
        if (scheduler_state.current_policy == ROUND_ROBIN) { // Si la política es RR 
            update_quantum(); // Actualiza el quantum
        } else { // Si la política es FSJ
            handle_fsj_preemption(); // Maneja la alternancia con FSJ
        }
        
        schedule_process(); // Planifica el siguiente proceso
        delay_ms(1000); // Espera 1 segundo
    }
    
    return NULL; // Devuelve NULL
}

// Inicialización y limpieza
void init_scheduler(void) {
    // Inicializar estado
    scheduler_state.current_policy = ROUND_ROBIN; // Inicializa la política de planificación
    scheduler_state.current_quantum = random_range(MIN_QUANTUM, MAX_QUANTUM); // Inicializa el quantum
    scheduler_state.last_quantum_update = time(NULL); // Obtiene la hora de última actualización de quantum
    scheduler_state.last_policy_switch = time(NULL); // Obtiene la hora de última vez que cambió de política
    scheduler_state.running = true; // Inicializa el estado del planificador
    scheduler_state.active_process = NULL; // Inicializa el proceso activo
    
    // Inicializa mutex y semáforos
    pthread_mutex_init(&scheduler_state.scheduler_mutex, NULL);
    sem_init(&scheduler_state.scheduler_sem, 0, 1);
    
    scheduler_state.ready_queue = malloc(sizeof(ReadyQueue)); // Inicializa cola de listos
    scheduler_state.ready_queue->size = 0; // Inicializa el tamaño de la cola de listos
    pthread_mutex_init(&scheduler_state.ready_queue->mutex, NULL); // Crea el mutex para el acceso a la cola de listos
    
    init_io_queue(); // Inicializa cola de E/S
    
    scheduler_state.process_table = malloc(sizeof(ProcessTable)); // Inicializa tabla de procesos
    init_process_table(scheduler_state.process_table); // Inicializa la tabla de procesos
    
    printf("Planificador inicializado - Política: %s, Quantum: %d\n", scheduler_state.current_policy == ROUND_ROBIN ? "Round Robin" : "FSJ", scheduler_state.current_quantum); // Imprime un mensaje de debug
    
    pthread_create(&scheduler_state.policy_control_thread, NULL, policy_control_thread, NULL); // Inicia los hilos
    pthread_create(&scheduler_state.io_thread, NULL, io_manager_thread, NULL); // Inicia el hilo de E/S
}

void cleanup_scheduler(void) {
    scheduler_state.running = false; // Libera el hilo de E/S
    
    pthread_mutex_lock(&scheduler_state.io_queue->mutex); // DEspierta hilos bloqueados
    pthread_cond_broadcast(&scheduler_state.io_queue->condition); // Señaliza la cola de E/S
    pthread_mutex_unlock(&scheduler_state.io_queue->mutex); // Desbloquea el mutex para el acceso a la cola de E/S
    
    pthread_join(scheduler_state.policy_control_thread, NULL); // Espera la finalización de los hilos
    pthread_join(scheduler_state.io_thread, NULL); // Espera a que termine el hilo de E/S
    
    pthread_mutex_destroy(&scheduler_state.scheduler_mutex); // Limpia los recursos
    sem_destroy(&scheduler_state.scheduler_sem); // Libera el semáforo de planificación
    
    pthread_mutex_destroy(&scheduler_state.ready_queue->mutex);  // Limpia la cola de listos
    free(scheduler_state.ready_queue); // Libera la cola de listos
    
    free(scheduler_state.process_table); // Limpia la tabla de procesos
    
    cleanup_io_queue(); // Limpia la cola de E/S
    
    printf("Planificador limpiado correctamente\n"); // Imprime un mensaje de debug
}