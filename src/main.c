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
}

void cleanup_all_beehives() {
    for (int i = 0; i < total_beehives; i++) {
        if (beehives[i] != NULL) {
            cleanup_beehive(beehives[i]);
            free(beehives[i]);
            beehives[i] = NULL;
        }
    }
}

int main() {
    signal(SIGINT, handle_signal);

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

    // Variables para tracking de tiempos
    time_t* process_start_times = calloc(MAX_BEEHIVES, sizeof(time_t));
    time_t* io_start_times = calloc(MAX_BEEHIVES, sizeof(time_t));
    time_t* ready_start_times = calloc(MAX_BEEHIVES, sizeof(time_t));
    int* ready_wait_counts = calloc(MAX_BEEHIVES, sizeof(int));
    int* io_wait_counts = calloc(MAX_BEEHIVES, sizeof(int));

    ProcessControlBlock pcb = {
        .process_id = 0,
        .arrival_time = time(NULL),
        .iterations = 0,
        .code_stack_progress = 0,
        .io_wait_time = 0,
        .avg_io_wait_time = 0.0,
        .avg_ready_wait_time = 0.0,
        .state = READY
    };

    while (running) {
        update_job_queue(beehives, total_beehives);
        
        for (int i = 0; i < job_queue_size; i++) {
            int current_index = job_queue[i].index;
            Beehive* current_hive = beehives[current_index];
            
            // Actualizar PCB para el proceso actual
            pcb.process_id = current_index;
            
            // Si es un proceso nuevo, establecer su tiempo de llegada
            if (process_start_times[current_index] == 0) {
                process_start_times[current_index] = time(NULL);
                pcb.arrival_time = process_start_times[current_index];
            }

            // Actualizar iteraciones cuando el proceso entra en ejecución
            pcb.iterations++;
            
            // Actualizar progreso del código basado en cámaras y recursos
            int total_resources = current_hive->honey_count + current_hive->egg_count;
            int max_resources = (MAX_HONEY + MAX_EGGS) * current_hive->chamber_count;
            pcb.code_stack_progress = (total_resources * 100) / max_resources;
            
            // Manejar tiempo de E/S cuando las abejas están recolectando polen
            if (current_hive->state == WAITING) {
                if (io_start_times[current_index] == 0) {
                    io_start_times[current_index] = time(NULL);
                }
            } else if (io_start_times[current_index] != 0) {
                pcb.io_wait_time += time(NULL) - io_start_times[current_index];
                io_wait_counts[current_index]++;
                io_start_times[current_index] = 0;
            }
            
            // Manejar tiempo en estado listo
            if (current_hive->state == READY) {
                if (ready_start_times[current_index] == 0) {
                    ready_start_times[current_index] = time(NULL);
                }
            } else if (ready_start_times[current_index] != 0) {
                time_t ready_time = time(NULL) - ready_start_times[current_index];
                pcb.avg_ready_wait_time = ((pcb.avg_ready_wait_time * ready_wait_counts[current_index]) + ready_time) /
                                        (ready_wait_counts[current_index] + 1);
                ready_wait_counts[current_index]++;
                ready_start_times[current_index] = 0;
            }
            
            // Calcular promedio de tiempo de E/S
            if (io_wait_counts[current_index] > 0) {
                pcb.avg_io_wait_time = (double)pcb.io_wait_time / io_wait_counts[current_index];
            }
            
            schedule_process(&pcb);
            print_beehive_stats(current_hive);
            
            if (check_new_queen(current_hive) && total_beehives < MAX_BEEHIVES) {
                // Buscar siguiente índice disponible
                int new_id = -1;
                for (int j = 0; j < MAX_BEEHIVES; j++) {
                    if (beehives[j] == NULL) {
                        new_id = j;
                        break;
                    }
                }
                
                if (new_id != -1) {
                    beehives[new_id] = malloc(sizeof(Beehive));
                    init_beehive(beehives[new_id], new_id);
                    total_beehives++;
                    printf("Nueva colmena creada (#%d)! Total de colmenas: %d\n", new_id, total_beehives);
                }
            }
            
            save_pcb_to_file(&pcb);
            update_process_table(&pcb);
        }
        
        // Actualizar archivo de historial de colmenas
        update_beehive_history(beehives, total_beehives);
        
        delay_ms(100);
    }

    // Cleanup
    cleanup_all_beehives();
    cleanup_scheduler();
    free(process_start_times);
    free(io_start_times);
    free(ready_start_times);
    free(ready_wait_counts);
    free(io_wait_counts);
    
    return 0;
}