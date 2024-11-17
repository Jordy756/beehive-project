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

// Variables globales del programa
static volatile sig_atomic_t running = 1;
static Beehive* beehives[MAX_BEEHIVES];
static int total_beehives = 0;

// Funciones privadas para manejo de señales
static void handle_signal(int sig) {
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
        printf(" - Abejas: %d\n", scheduler_state.active_process->resources.bee_count);
        printf(" - Miel: %d\n", scheduler_state.active_process->resources.honey_count);
        printf(" - Total: %d\n", scheduler_state.active_process->resources.total_resources);
        printf(" - Iteraciones: %d\n", scheduler_state.active_process->pcb.iterations);
    }

    print_io_stats();
    printf("=============================\n");
}

// Funciones privadas para manejo de colmenas
static void init_beehives(void) {
    for (int i = 0; i < MAX_BEEHIVES; i++) {
        beehives[i] = NULL;
    }
    
    // Crear colmena inicial
    beehives[0] = malloc(sizeof(Beehive));
    init_beehive(beehives[0], 0);
    total_beehives = 1;
}

static void cleanup_beehives(void) {
    printf("\nLimpiando todas las colmenas...\n");
    for (int i = 0; i < MAX_BEEHIVES; i++) {
        if (beehives[i] != NULL) {
            printf("Limpiando colmena #%d...\n", i);
            beehives[i]->should_terminate = 1;
            beehives[i]->threads.thread_running = false;
            usleep(100000); // 100ms
            cleanup_beehive(beehives[i]);
            free(beehives[i]);
            beehives[i] = NULL;
        }
    }
    
    cleanup_scheduler();
    printf("Limpieza completada.\n");
}

static void handle_new_beehive(Beehive* current_hive) {
    if (check_new_queen(current_hive) && total_beehives < MAX_BEEHIVES) {
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
            printf("- Total de colmenas activas: %d/%d\n\n", total_beehives, MAX_BEEHIVES);
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

// Control de estadísticas y monitoreo
static void update_statistics(time_t* last_stats_time) {
    time_t current_time = time(NULL);
    if (difftime(current_time, *last_stats_time) >= 5) {
        print_scheduling_info();
        *last_stats_time = current_time;
    }
}

// Manejo de preempción FSJ
static void handle_scheduling_policy(void) {
    if (scheduler_state.current_policy == SHORTEST_JOB_FIRST && scheduler_state.active_process != NULL) {
        handle_fsj_preemption();
    }
}

// Procesa una colmena individual
static void process_beehive(ProcessInfo* process_info) {
    Beehive* current_hive = beehives[process_info->index];
    
    if (current_hive != NULL && !current_hive->should_terminate) {
        sem_wait(process_info->shared_resource_sem);
        
        // Planificar el proceso
        schedule_process(&process_info->pcb);
        
        // Verificar preempción FSJ si es necesario
        if (scheduler_state.current_policy == SHORTEST_JOB_FIRST) {
            check_fsj_preemption(process_info);
        }
        
        // Manejar creación de nuevas colmenas
        handle_new_beehive(current_hive);
        
        sem_post(process_info->shared_resource_sem);
    }
}

// Procesa todas las colmenas en la cola
static void process_job_queue(void) {
    for (int i = 0; i < job_queue_size && running; i++) {
        if (!running) break;
        process_beehive(&job_queue[i]);
    }
    
    // Actualizar tabla de procesos si hay proceso activo
    if (scheduler_state.active_process != NULL) {
        update_process_table(&scheduler_state.active_process->pcb);
    }
}

// Función principal de simulación
static void run_simulation(void) {
    time_t last_stats_time = time(NULL);
    
    while (running) {
        // 1. Actualizar cola de trabajos
        update_job_queue(beehives, total_beehives);
        
        // 2. Manejar política de planificación
        handle_scheduling_policy();
        
        // 3. Actualizar estadísticas
        update_statistics(&last_stats_time);
        
        // 4. Procesar colmenas
        process_job_queue();
        
        // 5. Verificar terminación
        if (!running) {
            printf("\nIniciando proceso de terminación...\n");
            break;
        }
        
        // 6. Esperar antes del siguiente ciclo
        delay_ms(5000);
    }
}

int main() {
    // Configurar manejador de señales
    setup_signal_handlers();
    
    // Inicializaciones
    init_random();
    init_scheduler();
    init_file_manager();
    init_beehives();
    
    // Mostrar estado inicial
    print_initial_state();
    
    // Ejecutar simulación
    run_simulation();
    
    // Limpieza final
    printf("\nEsperando a que todas las colmenas terminen...\n");
    cleanup_beehives();
    
    printf("\n=== Simulación Finalizada ===\n");
    printf("- Total de colmenas procesadas: %d\n", total_beehives);
    printf("- Todas las colmenas han sido limpiadas\n");
    printf("- Recursos liberados correctamente\n\n");
    
    return 0;
}