#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "../include/types/beehive_types.h"
#include "../include/types/scheduler_types.h"
#include "../include/types/file_manager_types.h"
#include "../include/core/beehive.h"
#include "../include/core/scheduler.h"
#include "../include/core/file_manager.h"
#include "../include/core/utils.h"

// Variables globales
static volatile sig_atomic_t running = 1;
static ProcessInfo processes[MAX_PROCESSES];
static int total_processes = 0;

// Manejo de señales
static void handle_signal(int sig) {
    (void)sig;
    printf("\nRecibida señal de terminación (Ctrl+C). Finalizando el programa...\n");
    running = 0;

    // Detener todos los procesos
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

// Gestión de procesos
static void init_processes(void) {
    memset(processes, 0, sizeof(processes));

    for (int i = 0; i < INITIAL_BEEHIVES; i++) {
        ProcessInfo* process = &processes[i];
        process->index = i;
        init_process_semaphores(process);
        init_beehive_process(process, i);
        add_to_ready_queue(process);
    }

    total_processes = INITIAL_BEEHIVES;
}

static void cleanup_processes(void) {
    printf("\nLimpiando todos los procesos...\n");
    for (int i = 0; i < total_processes; i++) {
        if (processes[i].hive != NULL) {
            printf("Limpiando proceso #%d...\n", i);
            cleanup_beehive_process(&processes[i]);
            cleanup_process_semaphores(&processes[i]);
        }
    }
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
            add_to_ready_queue(new_process);
            total_processes++;
            printf("- Total de procesos activos: %d/%d\n\n", total_processes, MAX_PROCESSES);
        }
    }
}

// Impresión de información
static void print_scheduler_stats(void) {
    printf("\n=== Estado del Planificador ===\n");
    printf("Política actual: %s\n", scheduler_state.current_policy == ROUND_ROBIN ? "Round Robin" : "Shortest Job First (FSJ)");

    if (scheduler_state.current_policy == ROUND_ROBIN) {
        printf("Quantum actual: %d segundos\n", scheduler_state.current_quantum);
    }

    if (scheduler_state.active_process != NULL) {
        ProcessInfo* active = scheduler_state.active_process;
        printf("\nProceso en ejecución: %d\n", active->index);
        printf("- Abejas: %d\n", active->hive->bee_count);
        printf("- Miel: %d\n", active->hive->honey_count);
        printf("- Recursos totales: %d\n", active->hive->bees_and_honey_count);
        printf("- Iteraciones: %d\n", active->pcb->iterations);
    } else {
        printf("\nNo hay proceso en ejecución\n");
    }

    printf("\nProcesos en cola de listos: %d\n", scheduler_state.ready_queue->size);
    // Imprimir en un for cada uno de los procesos que esta en la cola de listos:
    for (int i = 0; i < scheduler_state.ready_queue->size; i++) {
        ProcessInfo* process = scheduler_state.ready_queue->processes[i];
        printf("- Proceso #%d: %d abejas, %d miel, %d recursos\n", process->index, process->hive->bee_count, process->hive->honey_count, process->hive->bees_and_honey_count);
    }
    printf("Procesos en cola de E/S: %d\n", scheduler_state.io_queue->size);
    // Imprimir en un for cada uno de los procesos que esta en la cola de E/S:
    for (int i = 0; i < scheduler_state.io_queue->size; i++) {
        ProcessInfo* process = scheduler_state.io_queue->entries[i].process;
        printf("- Proceso #%d: %d abejas, %d miel, %d recursos\n", process->index, process->hive->bee_count, process->hive->honey_count, process->hive->bees_and_honey_count);
    }
    printf("=============================\n");
}

static void print_initial_state(void) {
    printf("\n=== Simulación de Colmenas Iniciada ===\n");
    printf("- Colmenas iniciales: %d\n", INITIAL_BEEHIVES);
    printf("- Máximo de colmenas: %d\n", MAX_PROCESSES);
    printf("- Política inicial: %s\n", scheduler_state.current_policy == ROUND_ROBIN ? "Round Robin" : "FSJ");
    printf("- Quantum inicial: %d segundos\n", scheduler_state.current_quantum);
    printf("- Presione Ctrl+C para finalizar\n\n");
}

// Ciclo principal
static void run_simulation(void) {
    time_t last_stats_time = time(NULL);

    while (running) {
        time_t current_time = time(NULL);

        // Imprimir estadísticas cada 5 segundos
        if (difftime(current_time, last_stats_time) >= 5.0) {
            print_scheduler_stats();
            last_stats_time = current_time;
        }

        // Verificar nuevas colmenas
        if (scheduler_state.active_process) {
            handle_new_process(scheduler_state.active_process);
        }

        // Actualizar archivos de estado
        if (scheduler_state.active_process) {
            update_process_table(scheduler_state.active_process->pcb);
        }

        // Esperar antes del siguiente ciclo
        delay_ms(1000);
    }
}

int main(void) {
    // Configuración inicial
    srand(time(NULL));
    setup_signal_handlers();
    
    // Inicializar componentes
    init_file_manager();
    init_scheduler();
    init_processes();
    
    // Ejecutar simulación
    print_initial_state();
    run_simulation();
    
    // Limpieza
    cleanup_processes();
    cleanup_scheduler();
    
    printf("\n=== Simulación Finalizada ===\n");
    printf("Total de procesos: %d\n", total_processes);
    printf("Recursos liberados correctamente\n\n");
    
    return 0;
}