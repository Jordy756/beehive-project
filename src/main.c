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
    
    while (running) {
        // Update job queue with current beehives
        update_job_queue(beehives, total_beehives);
        
        // Process beehives in the determined order
        for (int i = 0; i < job_queue_size; i++) {
            int current_index = job_queue[i].index;
            Beehive* current_hive = beehives[current_index];
            
            pcb.process_id = current_index;
            
            schedule_process(&pcb);
            print_beehive_stats(current_hive);
            
            // Check for new queen and create new beehive if possible
            if (check_new_queen(current_hive) && total_beehives < MAX_BEEHIVES) {
                int new_id = total_beehives;
                beehives[new_id] = malloc(sizeof(Beehive));
                init_beehive(beehives[new_id], new_id);
                total_beehives++;
                
                printf("New beehive created! Total beehives: %d\n", total_beehives);
            }
        }
        
        save_pcb_to_file(&pcb);
        update_process_table(&pcb);
        delay_ms(100);
    }
    
    cleanup_all_beehives();
    free(job_queue);  // Clean up job queue
    return 0;
}