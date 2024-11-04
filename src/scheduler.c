#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/core/scheduler.h"
#include "../include/core/utils.h"

// Static variables for scheduler
static SchedulingPolicy current_policy = ROUND_ROBIN;
static int current_quantum;
static bool sort_by_bees = true;  // Toggle between sorting by bees and honey

// Define job queue
ProcessInfo* job_queue = NULL;
int job_queue_size = 0;

// Initialize scheduler control structure
SchedulerControl scheduler_control = {0};

void init_scheduler(void) {
    current_quantum = random_range(MIN_QUANTUM, MAX_QUANTUM);
    current_policy = ROUND_ROBIN;
    
    // Initialize job queue
    job_queue = malloc(sizeof(ProcessInfo) * MAX_PROCESSES);
    job_queue_size = 0;
    
    // Initialize scheduler control
    pthread_mutex_init(&scheduler_control.policy_mutex, NULL);
    pthread_cond_init(&scheduler_control.policy_change_cond, NULL);
    scheduler_control.running = true;
    scheduler_control.desired_policy = ROUND_ROBIN;
    
    // Start control thread
    start_scheduler_control();
}

void cleanup_scheduler(void) {
    scheduler_control.running = false;
    pthread_cond_signal(&scheduler_control.policy_change_cond);
    pthread_join(scheduler_control.control_thread, NULL);
    pthread_mutex_destroy(&scheduler_control.policy_mutex);
    pthread_cond_destroy(&scheduler_control.policy_change_cond);
    free(job_queue);
}

void* scheduler_control_thread(void* arg) {
    (void)arg; // Unused parameter
    
    while (scheduler_control.running) {
        pthread_mutex_lock(&scheduler_control.policy_mutex);
        pthread_cond_wait(&scheduler_control.policy_change_cond, &scheduler_control.policy_mutex);
        
        if (scheduler_control.running && current_policy != scheduler_control.desired_policy) {
            current_policy = scheduler_control.desired_policy;
            
            if (current_policy == SHORTEST_JOB_FIRST) {
                sort_by_bees = !sort_by_bees;
            }
            
            printf("Switched to %s", current_policy == ROUND_ROBIN ? "Round Robin" : "Shortest Job First");
            if (current_policy != ROUND_ROBIN) {
                printf(" (Sort by: %s)\n", sort_by_bees ? "Bees" : "Honey");
            } else {
                printf("\n");
            }
        }
        
        pthread_mutex_unlock(&scheduler_control.policy_mutex);
        delay_ms(100); // Small delay to prevent busy waiting
    }
    
    return NULL;
}

void start_scheduler_control(void) {
    pthread_create(&scheduler_control.control_thread, NULL, scheduler_control_thread, NULL);
}

void request_policy_change(SchedulingPolicy new_policy) {
    pthread_mutex_lock(&scheduler_control.policy_mutex);
    scheduler_control.desired_policy = new_policy;
    pthread_cond_signal(&scheduler_control.policy_change_cond);
    pthread_mutex_unlock(&scheduler_control.policy_mutex);
}

void switch_scheduling_policy(void) {
    SchedulingPolicy new_policy = (current_policy == ROUND_ROBIN) ? SHORTEST_JOB_FIRST : ROUND_ROBIN;
    request_policy_change(new_policy);
}

void sort_processes_sjf(ProcessInfo* processes, int count, bool by_bees) {
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            bool should_swap = false;
            
            if (by_bees) {
                should_swap = processes[j].hive->bee_count > processes[j + 1].hive->bee_count;
            } else {
                should_swap = processes[j].hive->honey_count > processes[j + 1].hive->honey_count;
            }
            
            if (should_swap) {
                ProcessInfo temp = processes[j];
                processes[j] = processes[j + 1];
                processes[j + 1] = temp;
            }
        }
    }
}

void update_process_info(ProcessInfo* info, ProcessControlBlock* pcb, Beehive* hive, int index) {
    info->pcb = pcb;
    info->hive = hive;
    info->index = index;
}

void update_job_queue(Beehive** beehives, int total_beehives) {
    job_queue_size = 0;
    
    // Add all active beehives to job queue
    for (int i = 0; i < total_beehives; i++) {
        if (beehives[i] != NULL) {
            job_queue[job_queue_size].hive = beehives[i];
            job_queue[job_queue_size].index = i;
            job_queue_size++;
        }
    }
    
    // Sort if using SJF
    if (current_policy == SHORTEST_JOB_FIRST && job_queue_size > 1) {
        sort_processes_sjf(job_queue, job_queue_size, sort_by_bees);
    }
}

void schedule_process(ProcessControlBlock* pcb) {
    static int policy_switch_counter = 0;
    policy_switch_counter++;
    
    // Check if we should switch policies
    if (policy_switch_counter >= POLICY_SWITCH_THRESHOLD) {
        switch_scheduling_policy();
        policy_switch_counter = 0;
    }
    
    if (current_policy == ROUND_ROBIN) {
        static int quantum_counter = 0;
        quantum_counter++;
        if (quantum_counter >= current_quantum) {
            update_quantum();
            quantum_counter = 0;
        }
    }
    
    pcb->iterations++;
}

void update_quantum(void) {
    current_quantum = random_range(MIN_QUANTUM, MAX_QUANTUM);
    printf("New quantum: %d\n", current_quantum);
}

SchedulingPolicy get_current_policy(void) {
    return current_policy;
}

int get_current_quantum(void) {
    return current_quantum;
}