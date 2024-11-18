#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

// Includes de tipos
#include "../include/types/beehive_types.h"
#include "../include/types/scheduler_types.h"
#include "../include/types/file_manager_types.h"

// Includes de core
#include "../include/core/beehive.h"
#include "../include/core/scheduler.h"
#include "../include/core/file_manager.h"
#include "../include/core/utils.h"

// Variables globales del programa
static volatile sig_atomic_t running = 1;
static ProcessInfo processes[MAX_PROCESSES];
static int total_processes = 0;

// Funciones privadas para manejo de señales y manejo de procesos
static void handle_signal(int sig) {
    (void)sig;
    printf("\nRecibida señal de terminación (Ctrl+C). Finalizando el programa...\n");
    running = 0;
    
    // Detener todos los procesos inmediatamente
    for (int i = 0; i < total_processes; i++) {
        if (processes[i].hive != NULL) {
            processes[i].hive->should_terminate = 1;
        }
    }
}

static void setup_signal_handlers(void) {
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error configurando el manejador de señales");
        exit(1);
    }
}

static void print_io_stats(void) {
    printf("\n=== Estado de E/S ===\n");
    pthread_mutex_lock(&scheduler_state.io_queue->mutex);
    printf("Procesos en E/S: %d\n", scheduler_state.io_queue->size);
    
    if (scheduler_state.io_queue->size > 0) {
        printf("Detalles de procesos en E/S:\n");
        for (int i = 0; i < scheduler_state.io_queue->size; i++) {
            IOQueueEntry* entry = &scheduler_state.io_queue->entries[i];
            time_t current_time = time(NULL);
            double elapsed_ms = difftime(current_time, entry->start_time) * 1000;
            printf("- Proceso %d: Tiempo restante: %.0f/%d ms\n",
                   entry->process->index,
                   (double)entry->wait_time - elapsed_ms,
                   entry->wait_time);
        }
    }
    pthread_mutex_unlock(&scheduler_state.io_queue->mutex);
    printf("==================\n");
}

static void print_scheduling_info(void) {
    printf("\n=== Estado del Planificador ===\n");
    printf("Política actual: %s\n",
           scheduler_state.current_policy == ROUND_ROBIN ?
           "Round Robin" : "Shortest Job First (FSJ)");
           
    if (scheduler_state.current_policy == ROUND_ROBIN) {
        printf("Quantum actual: %d\n", scheduler_state.current_quantum);
        printf("Contador de quantum: %d/%d\n",
               scheduler_state.quantum_counter,
               QUANTUM_UPDATE_INTERVAL);
    }
    
    printf("Contador para cambio de política: %d/%d\n",
           scheduler_state.policy_switch_counter,
           POLICY_SWITCH_THRESHOLD);
    printf("Procesos en cola: %d\n", job_queue_size);

    if (scheduler_state.active_process != NULL) {
        printf("\nProceso activo: %d\n", scheduler_state.active_process->index);
        printf("Recursos del proceso activo:\n");
        printf(" - Abejas: %d\n", scheduler_state.active_process->hive->bee_count);
        printf(" - Miel: %d\n", scheduler_state.active_process->hive->honey_count);
        printf(" - Total: %d\n", (scheduler_state.active_process->hive->bee_count + scheduler_state.active_process->hive->honey_count));
        printf(" - Iteraciones: %d\n", scheduler_state.active_process->pcb->iterations);
    }
    
    print_io_stats();
    printf("=============================\n");
}

static void init_processes(void) {
    memset(processes, 0, sizeof(processes));
    
    // Crear proceso inicial
    ProcessInfo* initial_process = &processes[0];
    initial_process->index = 0;
    init_process_semaphores(initial_process);
    init_beehive_process(initial_process, 0);
    total_processes = 1;
}

static void cleanup_processes(void) {
    printf("\nLimpiando todos los procesos...\n");
    for (int i = 0; i < total_processes; i++) {
        if (processes[i].hive != NULL) {
            printf("Limpiando proceso #%d...\n", i);
            cleanup_beehive_process(&processes[i]);
        }
    }
    cleanup_scheduler();
    printf("Limpieza completada.\n");
}

static void handle_new_process(ProcessInfo* process_info) {
    if (check_new_queen(process_info) && total_processes < MAX_PROCESSES) {
        int new_index = -1;
        
        // Buscar siguiente índice disponible
        for (int i = 0; i < MAX_PROCESSES; i++) {
            if (processes[i].hive == NULL) {
                new_index = i;
                break;
            }
        }
        
        if (new_index != -1) {
            ProcessInfo* new_process = &processes[new_index];
            new_process->index = new_index;
            init_process_semaphores(new_process);
            init_beehive_process(new_process, new_index);
            total_processes++;
            
            printf("\n=== ¡Nuevo Proceso Creado! ===\n");
            printf("- ID del proceso: %d\n", new_index);
            printf("- Total de procesos activos: %d/%d\n\n", total_processes, MAX_PROCESSES);
        }
    }
}

static void print_initial_state(void) {
    printf("\n=== Simulación de Colmenas Iniciada ===\n");
    printf("- Cada colmena tiene %d cámaras\n", NUM_CHAMBERS);
    printf("- Política inicial: Round Robin (Quantum: %d)\n",
           scheduler_state.current_quantum);
    printf("- FSJ se basa en total de recursos (abejas + miel)\n");
    printf("- Presione Ctrl+C para finalizar la simulación\n\n");
}

static void update_statistics(time_t* last_stats_time) {
    time_t current_time = time(NULL);
    if (difftime(current_time, *last_stats_time) >= 5) {
        print_scheduling_info();
        *last_stats_time = current_time;
    }
}

static void handle_scheduling_policy(void) {
    if (scheduler_state.current_policy == SHORTEST_JOB_FIRST) {
        handle_fsj_preemption();
    }
}

static void process_active_processes(void) {
    ProcessControlBlock* pcb;
    for (int i = 0; i < total_processes && running; i++) {
        if (!running) break;
        if (processes[i].hive != NULL) {
            if (sem_wait(processes[i].shared_resource_sem) == 0) {
                schedule_process(&pcb);
                if (scheduler_state.current_policy == SHORTEST_JOB_FIRST) {
                    check_fsj_preemption(&processes[i]);
                }
                handle_new_process(&processes[i]);
                sem_post(processes[i].shared_resource_sem);
            } else {
                printf("Error: Fallo en sem_wait para proceso %d\n", i);
            }
        }
    }
    if (scheduler_state.active_process != NULL) {
        update_process_table(scheduler_state.active_process->pcb);
    }
}

static void run_simulation(void) {
    time_t last_stats_time = time(NULL);
    
    while (running) {
        // Actualizar cola de trabajo
        update_job_queue(processes, total_processes);
        
        // Manejar política de planificación
        handle_scheduling_policy();
        
        // Actualizar estadísticas
        update_statistics(&last_stats_time);
        
        // Procesar procesos activos si hay alguno
        if (total_processes > 0) {
            process_active_processes();
        }
        
        if (!running) {
            printf("\nIniciando proceso de terminación...\n");
            break;
        }
        
        delay_ms(2000);
    }
}

int main() {
    setup_signal_handlers();
    init_scheduler();
    init_file_manager();
    init_processes();
    print_initial_state();
    run_simulation();
    
    printf("\nEsperando a que todos los procesos terminen...\n");
    cleanup_processes();
    
    printf("\n=== Simulación Finalizada ===\n");
    printf("- Total de procesos procesados: %d\n", total_processes);
    printf("- Todos los procesos han sido limpiados\n");
    printf("- Recursos liberados correctamente\n\n");
    
    return 0;
}