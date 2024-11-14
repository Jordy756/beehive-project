#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

// Includes de tipos
#include "../include/types/beehive_types.h"
#include "../include/types/scheduler_types.h"
#include "../include/types/file_manager_types.h"

// Includes de core
#include "../include/core/beehive.h"
#include "../include/core/scheduler.h"
#include "../include/core/file_manager.h"
#include "../include/core/utils.h"

// Variables globales
volatile sig_atomic_t running = 1;
Beehive* beehives[MAX_BEEHIVES];
int total_beehives = 0;
ProcessControlBlock pcb;  // Modificación: Movido a variable global para mejor gestión

void handle_signal(int sig) {
    (void)sig;
    printf("\nRecibida señal de terminación (Ctrl+C). Finalizando el programa...\n");
    running = 0;

    // Detener todas las colmenas inmediatamente
    for (int i = 0; i < MAX_BEEHIVES; i++) {
        if (beehives[i] != NULL) {
            beehives[i]->should_terminate = 1;
            beehives[i]->threads.thread_running = false;
        }
    }
}

void cleanup_all_beehives() {
    printf("\nLimpiando todas las colmenas...\n");
    for (int i = 0; i < MAX_BEEHIVES; i++) {
        if (beehives[i] != NULL) {
            printf("Limpiando colmena #%d...\n", i);
            
            // Asegurar que los hilos se detengan
            beehives[i]->should_terminate = 1;
            beehives[i]->threads.thread_running = false;
            
            // Esperar un momento para que los hilos terminen
            usleep(100000);  // 100ms
            
            cleanup_beehive(beehives[i]);
            free(beehives[i]);
            beehives[i] = NULL;
        }
    }
    
    // Limpiar el planificador
    cleanup_scheduler();
    printf("Limpieza completada.\n");
}

void print_process_resources() {
    printf("\n=== Nada ===\n");
    // printf("\n=== Recursos de Procesos ===\n");
    // for (int i = 0; i < job_queue_size; i++) {
    //     ProcessInfo* process = &job_queue[i];
    //     update_process_resources(process);
    //     printf("Proceso %d (Colmena %d):\n", i, process->index);
    //     printf("  - Abejas: %d\n", process->resources.bee_count);
    //     printf("  - Miel: %d\n", process->resources.honey_count);
    //     printf("  - Total recursos: %d\n", process->resources.total_resources);
    //     printf("  - Estado: %s\n",
    //             process->is_running ? "En ejecución" :
    //             process->in_io ? "En E/S" : "Listo");
    //     if (process->preempted) {
    //         printf("  - Interrumpido en: %s", ctime(&process->preemption_time));
    //     }
    //     // Agregar información del PCB
    //     printf("  - Iteraciones: %d\n", process->pcb.iterations);
    //     printf("  - Promedio espera E/S: %.2f segundos\n", process->pcb.avg_io_wait_time);
    //     printf("  - Promedio espera Ready: %.2f segundos\n", process->pcb.avg_ready_wait_time);
    // }
    // printf("===========================\n");
}

void print_io_stats() {
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

void print_scheduling_info() {
    printf("\n=== Estado del Planificador ===\n");
    printf("Política actual: %s\n",
            scheduler_state.current_policy == ROUND_ROBIN ? "Round Robin" : "Shortest Job First (FSJ)");
    
    if (scheduler_state.current_policy == ROUND_ROBIN) {
        printf("Quantum actual: %d\n", scheduler_state.current_quantum);
        printf("Contador de quantum: %d/%d\n",
                scheduler_state.quantum_counter, QUANTUM_UPDATE_INTERVAL);
    }
    
    printf("Contador para cambio de política: %d/%d\n",
            scheduler_state.policy_switch_counter, POLICY_SWITCH_THRESHOLD);
    printf("Procesos en cola: %d\n", job_queue_size);
    
    // Mostrar proceso activo
    if (scheduler_state.active_process != NULL) {
        printf("\nProceso activo: %d\n", scheduler_state.active_process->index);
        printf("Recursos del proceso activo:\n");
        printf("  - Abejas: %d\n", scheduler_state.active_process->resources.bee_count);
        printf("  - Miel: %d\n", scheduler_state.active_process->resources.honey_count);
        printf("  - Total: %d\n", scheduler_state.active_process->resources.total_resources);
        printf("  - Iteraciones: %d\n", scheduler_state.active_process->pcb.iterations);
    }
    
    // Mostrar recursos de todos los procesos
    print_process_resources();
    
    // Añadir información de E/S
    print_io_stats();
    printf("=============================\n");
}

int main() {
    // Configurar el manejador de señales
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error configurando el manejador de señales");
        return 1;
    }

    // Inicializaciones
    init_random();
    init_scheduler();
    init_file_manager();

    // Inicializar array de colmenas
    for (int i = 0; i < MAX_BEEHIVES; i++) {
        beehives[i] = NULL;
    }

    // Crear colmena inicial
    beehives[0] = malloc(sizeof(Beehive));
    init_beehive(beehives[0], 0);
    total_beehives = 1;

    // Inicializar PCB inicial
    // init_pcb(&pcb, 0);  // Inicializar PCB con el id del primer proceso

    printf("\n=== Simulación de Colmenas Iniciada ===\n");
    printf("- Cada colmena tiene %d cámaras\n", NUM_CHAMBERS);
    printf("- Política inicial: Round Robin (Quantum: %d)\n", scheduler_state.current_quantum);
    printf("- FSJ se basa en total de recursos (abejas + miel)\n");
    printf("- Presione Ctrl+C para finalizar la simulación\n\n");

    time_t last_stats_time = time(NULL);
    
    while (running) {
        // Actualizar cola de trabajo con colmenas actuales
        update_job_queue(beehives, total_beehives);

        // Verificar preemption en FSJ si hay proceso activo
        if (scheduler_state.current_policy == SHORTEST_JOB_FIRST &&
            scheduler_state.active_process != NULL) {
            handle_fsj_preemption();
        }

        // Mostrar estadísticas del planificador cada 5 segundos
        time_t current_time = time(NULL);
        if (difftime(current_time, last_stats_time) >= 5) {
            print_scheduling_info();
            last_stats_time = current_time;
        }

        // Procesar cada colmena en la cola de trabajo
        for (int i = 0; i < job_queue_size && running; i++) {
            if (!running) break;

            int current_index = job_queue[i].index;
            Beehive* current_hive = beehives[current_index];

            if (current_hive != NULL && !current_hive->should_terminate) {
                sem_wait(job_queue[i].shared_resource_sem);
                
                // Actualizar PCB y archivos usando el PCB del proceso actual
                save_beehive_history(current_hive);
                save_pcb(&job_queue[i].pcb);
                
                // Programar proceso usando su propio PCB
                schedule_process(&job_queue[i].pcb);
                
                if (scheduler_state.current_policy == SHORTEST_JOB_FIRST) {
                    check_fsj_preemption(&job_queue[i]);
                }
                
                print_beehive_stats(current_hive);

                // Verificar nueva reina y crear nueva colmena si es posible
                if (check_new_queen(current_hive) && total_beehives < MAX_BEEHIVES) {
                    // Encontrar el siguiente índice disponible
                    int new_index = -1;
                    for (int j = 0; j < MAX_BEEHIVES; j++) {
                        if (beehives[j] == NULL) {
                            new_index = j;
                            break;
                        }
                    }

                    if (new_index != -1) {
                        beehives[new_index] = malloc(sizeof(Beehive));
                        init_beehive(beehives[new_index], new_index);
                        total_beehives++;

                        printf("\n=== ¡Nueva Colmena Creada! ===\n");
                        printf("- ID de la nueva colmena: %d\n", new_index);
                        printf("- Total de colmenas activas: %d/%d\n\n",
                                total_beehives, MAX_BEEHIVES);
                    }
                }

                sem_post(job_queue[i].shared_resource_sem);
            }
        }

        // Actualizar tabla de procesos
        if (scheduler_state.active_process != NULL) {
            update_process_table(&scheduler_state.active_process->pcb);
        }

        if (!running) {
            printf("\nIniciando proceso de terminación...\n");
            break;
        }

        // Pequeña pausa para no saturar el CPU
        delay_ms(100);
    }

    printf("\nEsperando a que todas las colmenas terminen...\n");
    cleanup_all_beehives();

    printf("\n=== Simulación Finalizada ===\n");
    printf("- Total de colmenas procesadas: %d\n", total_beehives);
    printf("- Todas las colmenas han sido limpiadas\n");
    printf("- Recursos liberados correctamente\n\n");

    return 0;
}