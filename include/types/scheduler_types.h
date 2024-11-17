#ifndef SCHEDULER_TYPES_H
#define SCHEDULER_TYPES_H

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include "beehive_types.h"
#include "file_manager_types.h"

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
    int total_resources;        // Suma de abejas y miel
    int bee_count;             // Número de abejas
    int honey_count;           // Cantidad de miel
    time_t last_update;        // Último momento en que se actualizaron los recursos
} ProcessResources;

typedef struct {
    Beehive* hive;                    // Puntero a la colmena asociada
    int index;                        // Índice en la cola de procesos
    sem_t* shared_resource_sem;       // Semáforo para recursos compartidos
    time_t last_quantum_start;        // Inicio del quantum actual
    int remaining_time_slice;         // Tiempo restante en el slice actual
    bool is_running;                  // Estado de ejecución
    int priority;                     // Prioridad del proceso
    bool in_io;                       // Indica si está en E/S
    ProcessResources resources;       // Recursos del proceso
    bool preempted;                   // Indica si fue interrumpido
    time_t preemption_time;          // Momento de la última interrupción
    ProcessControlBlock pcb;          // Bloque de control de proceso asociado
} ProcessInfo;

typedef struct {
    ProcessInfo* process;
    int wait_time;
    time_t start_time;
} IOQueueEntry;

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
    pthread_t io_thread;
    sem_t scheduler_sem;
    sem_t queue_sem;
    ProcessInfo* active_process;
    IOQueue* io_queue;
    pthread_mutex_t preemption_mutex;
} SchedulerState;

extern SchedulerState scheduler_state;
extern ProcessInfo* job_queue;
extern int job_queue_size;

#endif