#ifndef SCHEDULER_TYPES_H
#define SCHEDULER_TYPES_H

#include "process_manager_types.h"
#include "beehive_types.h"

#define MAX_PROCESSES 40
#define MIN_QUANTUM 2
#define MAX_QUANTUM 10
#define POLICY_SWITCH_THRESHOLD 10  // Switch policy every 10 iterations

typedef enum {
    ROUND_ROBIN,
    SHORTEST_JOB_FIRST
} SchedulingPolicy;

typedef struct {
    ProcessControlBlock* pcb;
    Beehive* hive;
    int index;  // Index in beehives array
} ProcessInfo;

// Variables for scheduler state
typedef struct {
    SchedulingPolicy current_policy;
    int current_quantum;
    int quantum_counter;
    int policy_switch_counter;
    bool sort_by_bees;
    ProcessInfo* job_queue;
    int job_queue_size;
} SchedulerState;

#endif