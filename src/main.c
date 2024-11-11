#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

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

// Función para manejar señales (Ctrl+C)
void handle_signal(int sig) {
    (void)sig;
    printf("\nRecibida señal de terminación (Ctrl+C). Finalizando el programa...\n");
    running = 0;

    // Detener todas las colmenas inmediatamente
    for (int i = 0; i < MAX_BEEHIVES; i++) {
        if (beehives[i] != NULL) {
            beehives[i]->should_terminate = 1;
            beehives[i]->threads.threads_running = false;
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
            beehives[i]->threads.threads_running = false;
            
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

void print_scheduling_info() {
    printf("\n=== Estado del Planificador ===\n");
    printf("Política actual: %s\n",
           scheduler_state.current_policy == ROUND_ROBIN ? "Round Robin" : "Shortest Job First");
    
    if (scheduler_state.current_policy == ROUND_ROBIN) {
        printf("Quantum actual: %d\n", scheduler_state.current_quantum);
        printf("Contador de quantum: %d/%d\n",
               scheduler_state.quantum_counter, QUANTUM_UPDATE_INTERVAL);
    } else {
        printf("Ordenamiento por: %s\n",
               scheduler_state.sort_by_bees ? "Cantidad de abejas" : "Cantidad de miel");
    }
    
    printf("Contador para cambio de política: %d/%d\n",
           scheduler_state.policy_switch_counter, POLICY_SWITCH_THRESHOLD);
    printf("Procesos en cola: %d\n", job_queue_size);

    // Mostrar estado de procesos activos
    printf("\nEstado de los procesos:\n");
    for (int i = 0; i < job_queue_size; i++) {
        printf("Proceso %d: %s (Prioridad: %d)\n",
               job_queue[i].index,
               job_queue[i].is_running ? "Ejecutando" : "Esperando",
               job_queue[i].priority);
    }
    printf("=============================\n");
}

// Función auxiliar para crear nueva colmena
void create_new_beehive(int index) {
    beehives[index] = malloc(sizeof(Beehive));
    if (beehives[index] != NULL) {
        init_beehive(beehives[index], index);
        total_beehives++;
        printf("\n=== ¡Nueva Colmena Creada! ===\n");
        printf("- ID de la nueva colmena: %d\n", index);
        printf("- Total de colmenas activas: %d/%d\n\n", total_beehives, MAX_BEEHIVES);
    } else {
        printf("Error: No se pudo asignar memoria para la nueva colmena\n");
    }
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
    create_new_beehive(0);

    // Inicializar PCB
    ProcessControlBlock pcb = {
        .process_id = 0,
        .arrival_time = time(NULL),
        .iterations = 0,
        .code_stack_progress = 0,
        .io_wait_time = 0,
        .avg_io_wait_time = 0,
        .avg_ready_wait_time = 0,
        .state = READY
    };

    printf("\n=== Simulación de Colmenas Iniciada ===\n");
    printf("- Cada colmena tiene %d cámaras\n", NUM_CHAMBERS);
    printf("- Política inicial: Round Robin (Quantum: %d)\n", scheduler_state.current_quantum);
    printf("- Presione Ctrl+C para finalizar la simulación\n\n");

    time_t last_stats_time = time(NULL);
    
    while (running) {
        // Actualizar cola de trabajo con colmenas actuales
        update_job_queue(beehives, total_beehives);

        // Mostrar estadísticas del planificador cada 5 segundos
        time_t current_time = time(NULL);
        if (difftime(current_time, last_stats_time) >= 5) {
            print_scheduling_info();
            last_stats_time = current_time;
        }

        // Procesar cada colmena en la cola de trabajo
        for (int i = 0; i < job_queue_size && running; i++) {
            if (!running) break;

            ProcessInfo* current_process = &job_queue[i];
            
            if (current_process->hive != NULL && !current_process->hive->should_terminate) {
                // Intentar obtener el semáforo con timeout más corto
                struct timespec timeout;
                clock_gettime(CLOCK_REALTIME, &timeout);
                timeout.tv_nsec += 100000000; // 100ms timeout
                if (timeout.tv_nsec >= 1000000000) {
                    timeout.tv_sec++;
                    timeout.tv_nsec -= 1000000000;
                }
                
                int sem_result = sem_timedwait(current_process->shared_resource_sem, &timeout);
                if (sem_result == 0) {
                    pcb.process_id = current_process->index;
                    
                    // Ejecutar proceso solo si está activo o es su turno
                    if (!current_process->is_running || 
                        scheduler_state.current_policy == ROUND_ROBIN || 
                        scheduler_state.active_process == current_process) {
                        
                        // Actualizar estado y ejecutar
                        current_process->is_running = true;
                        save_beehive_history(current_process->hive);
                        save_pcb(&pcb);
                        schedule_process(&pcb);
                        print_beehive_stats(current_process->hive);

                        // Verificar nueva reina y crear nueva colmena si es necesario
                        if (check_new_queen(current_process->hive) && total_beehives < MAX_BEEHIVES) {
                            for (int j = 0; j < MAX_BEEHIVES; j++) {
                                if (beehives[j] == NULL) {
                                    create_new_beehive(j);
                                    break;
                                }
                            }
                        }
                    }

                    // Liberar el semáforo inmediatamente después de usar el proceso
                    sem_post(current_process->shared_resource_sem);
                }
            }
        }

        // Actualizar tabla de procesos
        save_pcb(&pcb);
        update_process_table(&pcb);

        if (!running) break;

        // Pequeña pausa para no saturar el CPU
        usleep(100000); // Reducido a 100ms
    }

    printf("\nEsperando a que todas las colmenas terminen...\n");
    cleanup_all_beehives();

    printf("\n=== Simulación Finalizada ===\n");
    printf("- Total de colmenas procesadas: %d\n", total_beehives);
    printf("- Todas las colmenas han sido limpiadas\n");
    printf("- Recursos liberados correctamente\n\n");

    return 0;
}