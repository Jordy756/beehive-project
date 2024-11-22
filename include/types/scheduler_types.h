#ifndef SCHEDULER_TYPES_H
#define SCHEDULER_TYPES_H

#include <pthread.h> // Biblioteca de hilos
#include <semaphore.h> // Biblioteca de semáforos
#include <stdbool.h> // Biblioteca de tipos de datos
#include <time.h> // Biblioteca de tiempo
#include "beehive_types.h" // Tipos de colmenas
#include "file_manager_types.h" // Tipos de gestión de archivos

// Constantes de planificación
#define MIN_QUANTUM 2 // Tiempo mínimo de quantum
#define MAX_QUANTUM 10 // Tiempo máximo de quantum
#define QUANTUM_UPDATE_INTERVAL 10 // Intervalo de actualización de quantum
#define POLICY_SWITCH_THRESHOLD 30 // Límite de cambio de política
#define MAX_PROCESSES 40 // Número máximo de procesos
#define PROCESS_TIME_SLICE 100 // Límite de tiempo de proceso

// Constantes para E/S
#define IO_PROBABILITY 5 // Probabilidad de E/S
#define MIN_IO_WAIT 30 // Tiempo mínimo de espera de E/S
#define MAX_IO_WAIT 50 // Tiempo máximo de espera de E/S
#define MAX_IO_QUEUE_SIZE 40 // Tamaño máximo de la cola de E/S

// Tipos de políticas de planificación
typedef enum {
    ROUND_ROBIN, // Round Robin
    SHORTEST_JOB_FIRST // Shortest Job First
} SchedulingPolicy;

// Información de proceso
typedef struct ProcessInfo {
    Beehive* hive; // Colmena del proceso
    int index; // Índice del proceso en la colmena
    pthread_t thread_id; // ID de la thread del proceso
    sem_t* shared_resource_sem; // Semáforo para el acceso a recursos compartidos
    time_t last_quantum_start; // Último momento de inicio de quantum
    ProcessControlBlock* pcb; // Bloque de control del proceso
} ProcessInfo;

// Entrada en la cola de E/S
typedef struct {
    ProcessInfo* process; // Proceso asociado a la entrada
    int wait_time; // Tiempo de espera
    time_t start_time; // Tiempo de inicio de la entrada
} IOQueueEntry;

// Cola de E/S
typedef struct {
    IOQueueEntry entries[MAX_IO_QUEUE_SIZE]; // Entradas de la cola
    int size; // Tamaño de la cola
    pthread_mutex_t mutex; // Mutex para el acceso a la cola
    pthread_cond_t condition; // Condición para espera de E/S
} IOQueue;

// Cola de procesos listos
typedef struct {
    ProcessInfo* processes[MAX_PROCESSES]; // Procesos listos
    int size; // Tamaño de la cola
    pthread_mutex_t mutex; // Mutex para el acceso a la cola
} ReadyQueue;

// Estado del planificador
typedef struct {
    SchedulingPolicy current_policy; // Política actual
    int current_quantum; // Quantum actual
    time_t last_quantum_update; // Último momento de actualización de quantum
    time_t last_policy_switch; // Último momento de cambio de política
    bool running; // Indica si el planificador está en ejecución
    pthread_t policy_control_thread; // Thread para control de política
    pthread_t io_thread; // Thread para E/S
    sem_t scheduler_sem; // Semáforo para el acceso al planificador
    ProcessInfo* active_process; // Proceso activo
    IOQueue* io_queue; // Cola de E/S
    ReadyQueue* ready_queue; // Cola de procesos listos
    ProcessTable* process_table; // Tabla de control de procesos
    pthread_mutex_t scheduler_mutex; // Mutex para el acceso al planificador
} SchedulerState;

// Variables globales externas
extern SchedulerState scheduler_state; // Estado del planificador

#endif