#ifndef SCHEDULER_TYPES_H
#define SCHEDULER_TYPES_H

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include "beehive_types.h"

#define MIN_QUANTUM 2
#define MAX_QUANTUM 10
#define QUANTUM_UPDATE_INTERVAL 5
#define POLICY_SWITCH_THRESHOLD 10
#define MAX_PROCESSES 40
#define PROCESS_TIME_SLICE 100  // Tiempo base en ms para cada proceso

typedef enum {
    ROUND_ROBIN,
    SHORTEST_JOB_FIRST
} SchedulingPolicy;

typedef struct {
    Beehive* hive;
    int index;
    sem_t* shared_resource_sem;
    time_t last_quantum_start;
    int remaining_time_slice;  // Nuevo: tiempo restante en su quantum
    bool is_running;          // Nuevo: indica si el proceso está en ejecución
    int priority;            // Nuevo: prioridad del proceso
} ProcessInfo;

typedef struct {
    SchedulingPolicy current_policy;
    int current_quantum;
    int quantum_counter;
    int policy_switch_counter;
    bool sort_by_bees;
    bool running;
    pthread_t policy_control_thread;
    sem_t scheduler_sem;
    sem_t queue_sem;         // Nuevo: semáforo específico para la cola
    ProcessInfo* active_process; // Nuevo: proceso actualmente en ejecución
} SchedulerState;

extern SchedulerState scheduler_state;
extern ProcessInfo* job_queue;
extern int job_queue_size;

#endif