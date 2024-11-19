#ifndef SCHEDULER_TYPES_H
#define SCHEDULER_TYPES_H

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <time.h>
#include "beehive_types.h"
#include "file_manager_types.h"

// Constantes de planificación
#define MIN_QUANTUM 2
#define MAX_QUANTUM 10
#define QUANTUM_UPDATE_INTERVAL 10
#define POLICY_SWITCH_THRESHOLD 30
#define MAX_PROCESSES 40
#define PROCESS_TIME_SLICE 100

// Constantes para E/S
#define IO_PROBABILITY 5
#define MIN_IO_WAIT 30
#define MAX_IO_WAIT 50
#define MAX_IO_QUEUE_SIZE 40

// Tipos de políticas de planificación
typedef enum {
    ROUND_ROBIN,
    SHORTEST_JOB_FIRST
} SchedulingPolicy;

// Información de proceso
typedef struct ProcessInfo {
    Beehive* hive;
    int index;
    pthread_t thread_id;
    sem_t* shared_resource_sem;
    time_t last_quantum_start;
    ProcessControlBlock* pcb;
} ProcessInfo;

// Entrada en la cola de E/S
typedef struct {
    ProcessInfo* process;
    int wait_time;
    time_t start_time;
} IOQueueEntry;

// Cola de E/S
typedef struct {
    IOQueueEntry entries[MAX_IO_QUEUE_SIZE];
    int size;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
} IOQueue;

// Cola de procesos listos
typedef struct {
    ProcessInfo* processes[MAX_PROCESSES];
    int size;
    pthread_mutex_t mutex;
} ReadyQueue;

// Estado del planificador
typedef struct {
    SchedulingPolicy current_policy;
    int current_quantum;
    time_t last_quantum_update;
    time_t last_policy_switch;
    bool running;
    pthread_t policy_control_thread;
    pthread_t io_thread;
    sem_t scheduler_sem;
    ProcessInfo* active_process;
    IOQueue* io_queue;
    ReadyQueue* ready_queue;
    pthread_mutex_t scheduler_mutex;
} SchedulerState;

// Variables globales externas
extern SchedulerState scheduler_state;

#endif