#ifndef SCHEDULER_TYPES_H
#define SCHEDULER_TYPES_H

#include "beehive_types.h"
#include <stdbool.h>
#include <pthread.h>

// Constants
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

// Nueva estructura para el control del planificador
typedef struct {
    pthread_t control_thread;
    pthread_mutex_t policy_mutex;
    pthread_cond_t policy_change_cond;
    volatile bool running;
    SchedulingPolicy desired_policy;
} SchedulerControl;

// Variables globales del scheduler que necesitan ser accedidas desde diferentes archivos
extern ProcessInfo* job_queue;
extern int job_queue_size;
extern SchedulerControl scheduler_control;

#endif