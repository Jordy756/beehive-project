#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "../include/core/beehive.h"
#include "../include/core/scheduler.h"
#include "../include/core/process_manager.h"
#include "../include/core/utils.h"
#include "../include/types/beehive_types.h"
#include "../include/types/process_manager_types.h"
#include "../include/types/scheduler_types.h"

volatile sig_atomic_t running = 1;
Beehive* beehives[MAX_PROCESSES] = {NULL};  // Inicializar array con NULL
int total_beehives = 0;

void handle_signal(int sig) {
    (void)sig;  // Evitar warning de variable no utilizada
    running = 0;
    log_message("Received shutdown signal, cleaning up...");
}

void cleanup_all_beehives() {
    log_message("Starting cleanup of all beehives");
    for (int i = 0; i < total_beehives; i++) {
        if (beehives[i] != NULL) {
            cleanup_beehive(beehives[i]);
            free(beehives[i]);
            beehives[i] = NULL;
        }
    }
    log_message("Cleanup completed");
}

void initialize_first_beehive() {
    beehives[0] = malloc(sizeof(Beehive));
    if (beehives[0] == NULL) {
        log_message("Error: Failed to allocate memory for first beehive");
        exit(1);
    }
    init_beehive(beehives[0], 0);
    total_beehives = 1;
    log_message("First beehive initialized successfully");
}

void initialize_system() {
    // Configurar el manejador de señales
    signal(SIGINT, handle_signal);
    
    // Inicializar componentes del sistema
    init_random();
    init_scheduler();
    init_process_manager();
    
    log_message("System components initialized");
    
    // Inicializar la primera colmena
    initialize_first_beehive();
}

// void update_process_control_block(ProcessControlBlock* pcb, Beehive* hive) {
//     pcb->bee_count = hive->bee_count;
//     pcb->honey_count = hive->honey_count;
//     pcb->egg_count = hive->egg_count;
    
//     // Actualizar el historial de la colmena
//     update_beehive_history(hive->id, hive->bee_count, hive->honey_count, hive->egg_count);
// }

int main() {
    initialize_system();
    
    ProcessControlBlock pcb = {
        .arrival_time = time(NULL),
        .iterations = 0,
        .code_stack_progress = 0,
        .io_wait_time = 0,
        .avg_io_wait_time = 0,
        .avg_ready_wait_time = 0,
        .process_id = 0,
        .priority = 1,
        .state = READY
    };
    strncpy(pcb.status, "READY", sizeof(pcb.status) - 1);
    
    // Inicializar tabla de procesos
    ProcessTable process_table = {
        .avg_arrival_time = 0,
        .avg_iterations = 0,
        .avg_code_progress = 0,
        .avg_io_wait_time = 0,
        .total_io_wait_time = 0,
        .avg_ready_wait_time = 0,
        .total_ready_wait_time = 0,
        .total_processes = 0,
        .last_update = time(NULL)
    };
    
    save_process_table(&process_table);
    
    while (running) {
        update_job_queue(beehives, total_beehives);
        
        for (int i = 0; i < job_queue_size && running; i++) {
            int current_index = job_queue[i].index;
            Beehive* current_hive = beehives[current_index];
            
            if (current_hive == NULL) {
                continue;
            }
            
            pcb.process_id = current_index;
            
            if (get_current_policy() == ROUND_ROBIN && total_beehives >= 2) {
                sem_wait(&current_hive->resource_sem);
            }
            
            schedule_process(&pcb);
            process_egg_hatching(current_hive);
            print_beehive_stats(current_hive);
            
            if (get_current_policy() == ROUND_ROBIN && total_beehives >= 2) {
                sem_post(&current_hive->resource_sem);
            }
            
            if (check_new_queen(current_hive) && total_beehives < MAX_PROCESSES) {
                int new_id = total_beehives;
                beehives[new_id] = malloc(sizeof(Beehive));
                if (beehives[new_id] == NULL) {
                    log_message("Error: Failed to allocate memory for new beehive");
                    continue;
                }
                init_beehive(beehives[new_id], new_id);
                total_beehives++;
                process_table.total_processes++;
                
                char message[100];
                snprintf(message, sizeof(message), 
                        "New beehive created! ID: %d, Total beehives: %d", 
                        new_id, total_beehives);
                log_message(message);
            }
            
            // Actualizar PCB y tabla de procesos
            save_pcb_to_file(&pcb);
            update_process_table(&pcb);
            pcb.iterations++;
        }
        
        delay_ms(100);
    }
    
    cleanup_all_beehives();
    free(job_queue);
    log_message("Program terminated successfully");
    return 0;
}