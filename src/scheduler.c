#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/core/scheduler.h"
#include "../include/core/utils.h"

// Static state variable
static SchedulerState scheduler_state;

// Define job queue from the state
ProcessInfo* job_queue = NULL;
int job_queue_size = 0;

void init_scheduler(void) {
    scheduler_state.current_quantum = random_range(MIN_QUANTUM, MAX_QUANTUM);
    scheduler_state.current_policy = ROUND_ROBIN;
    scheduler_state.quantum_counter = 0;
    scheduler_state.policy_switch_counter = 0;
    scheduler_state.sort_by_bees = true;
    
    // Initialize job queue
    job_queue = malloc(sizeof(ProcessInfo) * MAX_PROCESSES);
    scheduler_state.job_queue = job_queue;
    scheduler_state.job_queue_size = 0;
    job_queue_size = 0;
}

void switch_scheduling_policy(void) {
    scheduler_state.current_policy = (scheduler_state.current_policy == ROUND_ROBIN) ? 
                                   SHORTEST_JOB_FIRST : ROUND_ROBIN;
    
    if (scheduler_state.current_policy == SHORTEST_JOB_FIRST) {
        scheduler_state.sort_by_bees = !scheduler_state.sort_by_bees;
    }
    
    printf("Switched to %s", scheduler_state.current_policy == ROUND_ROBIN ? 
           "Round Robin" : "Shortest Job First");
    if (scheduler_state.current_policy != ROUND_ROBIN) {
        printf(" (Sort by: %s)\n", scheduler_state.sort_by_bees ? "Bees" : "Honey");
    } else {
        printf("\n");
    }
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
    scheduler_state.job_queue_size = 0;
    job_queue_size = 0;
    
    // Add all active beehives to job queue
    for (int i = 0; i < total_beehives; i++) {
        if (beehives[i] != NULL) {
            job_queue[job_queue_size].hive = beehives[i];
            job_queue[job_queue_size].index = i;
            job_queue_size++;
            scheduler_state.job_queue_size++;
        }
    }
    
    // Sort if using SJF
    if (scheduler_state.current_policy == SHORTEST_JOB_FIRST && job_queue_size > 1) {
        sort_processes_sjf(job_queue, job_queue_size, scheduler_state.sort_by_bees);
    }
}

void schedule_process(ProcessControlBlock* pcb) {
    scheduler_state.policy_switch_counter++;
    
    // Check if we should switch policies
    if (scheduler_state.policy_switch_counter >= POLICY_SWITCH_THRESHOLD) {
        switch_scheduling_policy();
        scheduler_state.policy_switch_counter = 0;
    }
    
    if (scheduler_state.current_policy == ROUND_ROBIN) {
        scheduler_state.quantum_counter++;
        if (scheduler_state.quantum_counter >= scheduler_state.current_quantum) {
            update_quantum();
            scheduler_state.quantum_counter = 0;
        }
    }
    
    pcb->iterations++;
}

void update_quantum(void) {
    scheduler_state.current_quantum = random_range(MIN_QUANTUM, MAX_QUANTUM);
    printf("New quantum: %d\n", scheduler_state.current_quantum);
}

SchedulingPolicy get_current_policy(void) {
    return scheduler_state.current_policy;
}

int get_current_quantum(void) {
    return scheduler_state.current_quantum;
}