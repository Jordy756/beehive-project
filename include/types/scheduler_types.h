#ifndef SCHEDULER_TYPES_H
#define SCHEDULER_TYPES_H

#include "beehive_types.h"
#include <stdbool.h>
#include <semaphore.h>
#include <pthread.h>

// Constants
#define MAX_PROCESSES 40
#define MIN_QUANTUM 2
#define MAX_QUANTUM 10
#define POLICY_SWITCH_THRESHOLD 10  // Switch policy every 10 iterations
#define QUANTUM_UPDATE_INTERVAL 5   // Update quantum every 5 iterations in RR

typedef enum {
    ROUND_ROBIN,
    SHORTEST_JOB_FIRST
} SchedulingPolicy;

typedef struct {
    ProcessControlBlock* pcb;
    Beehive* hive;
    int index;  // Index in beehives array
    sem_t* shared_resource_sem;  // Semáforo para recursos compartidos
    time_t last_quantum_start;   // Para control de quantum en RR
} ProcessInfo;

typedef struct {
    SchedulingPolicy current_policy;
    int current_quantum;
    int quantum_counter;
    int policy_switch_counter;
    bool sort_by_bees;
    pthread_t policy_control_thread;  // Hilo de control para cambio de política
    bool running;
    sem_t scheduler_sem;  // Semáforo general del planificador
} SchedulerState;

// Variables globales del scheduler que necesitan ser accedidas desde diferentes archivos
extern ProcessInfo* job_queue;
extern int job_queue_size;
extern SchedulerState scheduler_state;

#endif