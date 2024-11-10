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
    printf("Limpieza completada.\n");
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
    printf("- Cada colmena tiene %d cámaras (alternando entre miel y cría)\n", NUM_CHAMBERS);
    printf("- Presione Ctrl+C para finalizar la simulación\n\n");

    while (running) {
        // Actualizar cola de trabajo con colmenas actuales
        update_job_queue(beehives, total_beehives);

        for (int i = 0; i < job_queue_size && running; i++) {
            if (!running) break;  // Verificación adicional
            
            int current_index = job_queue[i].index;
            Beehive* current_hive = beehives[current_index];

            if (current_hive != NULL && !current_hive->should_terminate) {
                pcb.process_id = current_index;

                // Forzar flush después de cada escritura
                save_beehive_history(current_hive);
                fflush(NULL);  // Forzar flush de todos los streams
                
                save_pcb(&pcb);
                fflush(NULL);
                
                schedule_process(&pcb);
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
            }
        }

        save_pcb(&pcb);
        update_process_table(&pcb);
        fflush(NULL);  // Forzar flush de todos los streams

        if (!running) {
            printf("\nIniciando proceso de terminación...\n");
            break;
        }

        delay_ms(100);
    }

    printf("\nEsperando a que todas las colmenas terminen...\n");
    cleanup_all_beehives();
    
    if (job_queue != NULL) {
        free(job_queue);
        job_queue = NULL;
    }

    printf("\n=== Simulación Finalizada ===\n");
    printf("- Total de colmenas procesadas: %d\n", total_beehives);
    printf("- Todas las colmenas han sido limpiadas\n");
    printf("- Recursos liberados correctamente\n\n");

    return 0;
}