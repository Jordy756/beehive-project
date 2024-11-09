#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

// Includes de tipos
#include "../include/types/beehive_types.h"
#include "../include/types/scheduler_types.h"
#include "../include/types/process_manager_types.h"

// Includes de core
#include "../include/core/beehive.h"
#include "../include/core/scheduler.h"
#include "../include/core/process_manager.h"
#include "../include/core/utils.h"

volatile sig_atomic_t running = 1;
Beehive* beehives[MAX_BEEHIVES];
int total_beehives = 0;

void handle_signal(int sig) {
    (void)sig;  // Avoid unused parameter warning
    running = 0;
    printf("\nFinalizando la simulación de colmenas...\n");
}

void cleanup_all_beehives() {
    printf("\nLimpiando todas las colmenas...\n");
    for (int i = 0; i < total_beehives; i++) {
        if (beehives[i] != NULL) {
            printf("Limpiando colmena #%d...\n", i);
            cleanup_beehive(beehives[i]);
            free(beehives[i]);
            beehives[i] = NULL;
        }
    }
    printf("Limpieza completada.\n");
}

int main() {
    // Configurar el manejador de señales para SIGINT (Ctrl+C)
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    
    init_random();
    init_scheduler();
    init_process_manager();
    
    // Initialize beehives array
    for (int i = 0; i < MAX_BEEHIVES; i++) {
        beehives[i] = NULL;
    }
    
    // Create initial beehive
    beehives[0] = malloc(sizeof(Beehive));
    init_beehive(beehives[0], 0);
    total_beehives = 1;
    
    ProcessControlBlock pcb = {
        .arrival_time = time(NULL),
        .iterations = 0,
        .code_stack_progress = 0,
        .io_wait_time = 0,
        .avg_io_wait_time = 0,
        .avg_ready_wait_time = 0,
        .process_id = 0,
        .state = READY
    };
    
    printf("\n=== Simulación de Colmenas Iniciada ===\n");
    printf("- Cada colmena tiene %d cámaras (alternando entre miel y cría)\n", NUM_CHAMBERS);
    printf("- Presione Ctrl+C para finalizar la simulación\n\n");
    
    while (running) {
        // Update job queue with current beehives
        update_job_queue(beehives, total_beehives);
        
        // Process beehives in the determined order
        for (int i = 0; i < job_queue_size && running; i++) {
            int current_index = job_queue[i].index;
            Beehive* current_hive = beehives[current_index];
            
            if (current_hive != NULL) {
                pcb.process_id = current_index;
                
                schedule_process(&pcb);
                print_beehive_stats(current_hive);
                
                // Check for new queen and create new beehive if possible
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
                        printf("- Total de colmenas activas: %d/%d\n\n", total_beehives, MAX_BEEHIVES);
                    }
                }
            }
        }
        
        save_pcb_to_file(&pcb);
        update_process_table(&pcb);
        
        delay_ms(100);
    }
    
    // Limpieza final
    cleanup_all_beehives();
    free(job_queue);
    
    printf("\n=== Simulación Finalizada ===\n");
    printf("- Total de colmenas procesadas: %d\n", total_beehives);
    printf("- Todas las colmenas han sido limpiadas\n");
    printf("- Recursos liberados correctamente\n\n");
    
    return 0;
}