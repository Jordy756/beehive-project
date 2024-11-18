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
#define QUANTUM_UPDATE_INTERVAL 5
#define POLICY_SWITCH_THRESHOLD 10
#define MAX_PROCESSES 40
#define PROCESS_TIME_SLICE 100 // Tiempo base en ms para cada proceso


// Constantes para E/S
#define IO_PROBABILITY 5 // 5% de probabilidad
#define MIN_IO_WAIT 30 // 30ms mínimo
#define MAX_IO_WAIT 50 // 50ms máximo
#define MAX_IO_QUEUE_SIZE 40


// Tipos de políticas de planificación
typedef enum {
   ROUND_ROBIN,
   SHORTEST_JOB_FIRST
} SchedulingPolicy;


// Recursos del proceso
typedef struct {
   int total_resources;  // Suma de abejas y miel
   int bee_count;       // Número de abejas
   int honey_count;     // Cantidad de miel
   time_t last_update;  // Último momento de actualización
} ProcessResources;


// Información de proceso
typedef struct ProcessInfo {
   Beehive* hive;                    // Puntero a la colmena asociada
   int index;                        // Índice en la cola de procesos
   pthread_t thread_id;              // ID del hilo del proceso
   sem_t* shared_resource_sem;       // Semáforo para recursos compartidos
   time_t last_quantum_start;        // Inicio del quantum actual
   int remaining_time_slice;         // Tiempo restante en el slice actual
   bool is_running;                  // Estado de ejecución
   int priority;                     // Prioridad del proceso
   bool in_io;                       // Indica si está en E/S
   ProcessResources resources;       // Recursos del proceso
   bool preempted;                   // Indica si fue interrumpido
   time_t preemption_time;          // Momento de la última interrupción
   ProcessControlBlock pcb;          // Bloque de control de proceso
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


// Estado del planificador
typedef struct {
   SchedulingPolicy current_policy;    // Política actual
   int current_quantum;               // Quantum actual para RR
   int quantum_counter;               // Contador para actualización de quantum
   int policy_switch_counter;         // Contador para cambio de política
   bool running;                      // Estado de ejecución del planificador
   pthread_t policy_control_thread;   // Hilo de control de política
   pthread_t io_thread;              // Hilo de E/S
   sem_t scheduler_sem;              // Semáforo del planificador
   sem_t queue_sem;                  // Semáforo de la cola
   ProcessInfo* active_process;      // Proceso actualmente en ejecución
   IOQueue* io_queue;                // Cola de E/S
   pthread_mutex_t preemption_mutex; // Mutex para preempción
} SchedulerState;


// Variables globales externas
extern SchedulerState scheduler_state;
extern ProcessInfo* job_queue;
extern int job_queue_size;


#endif
