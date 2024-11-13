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

// Nuevas constantes para E/S
#define IO_PROBABILITY 5        // 5% de probabilidad
#define MIN_IO_WAIT 30         // 30ms mínimo
#define MAX_IO_WAIT 50         // 50ms máximo
#define MAX_IO_QUEUE_SIZE 40   // Tamaño máximo de la cola de E/S

typedef enum {
    ROUND_ROBIN,
    SHORTEST_JOB_FIRST
} SchedulingPolicy;

typedef struct {
    Beehive* hive;
    int index;
    sem_t* shared_resource_sem;
    time_t last_quantum_start;
    int remaining_time_slice;
    bool is_running;
    int priority;
    bool in_io;                // Nuevo: indica si está en E/S
} ProcessInfo;

// Nueva estructura para entrada en cola de E/S
typedef struct {
    ProcessInfo* process;       // Proceso en E/S
    int wait_time;             // Tiempo de espera asignado
    time_t start_time;         // Tiempo cuando entró a E/S
} IOQueueEntry;

// Nueva estructura para cola de E/S
typedef struct {
    IOQueueEntry entries[MAX_IO_QUEUE_SIZE];
    int size;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
} IOQueue;

typedef struct {
    SchedulingPolicy current_policy;
    int current_quantum;
    int quantum_counter;
    int policy_switch_counter;
    bool sort_by_bees;
    bool running;
    pthread_t policy_control_thread;
    pthread_t io_thread;       // Nuevo: thread para manejar E/S
    sem_t scheduler_sem;
    sem_t queue_sem;
    ProcessInfo* active_process;
    IOQueue* io_queue;         // Nuevo: cola de E/S
} SchedulerState;

extern SchedulerState scheduler_state;
extern ProcessInfo* job_queue;
extern int job_queue_size;

#endif