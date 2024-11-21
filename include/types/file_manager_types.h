#ifndef FILE_MANAGER_TYPES_H
#define FILE_MANAGER_TYPES_H

#include <time.h>
#include <stdbool.h>
#include <json-c/json.h>

// Constants
#define PCB_FILE "data/pcb.json"
#define PROCESS_TABLE_FILE "data/process_table.json"
#define BEEHIVE_HISTORY_FILE "data/beehive_history.json"

typedef enum {
   READY,
   RUNNING,
   WAITING
} ProcessState;

typedef struct {
   int process_id;               // ID único del proceso/colmena
   time_t arrival_time;          // Último tiempo de llegada a cola
   int iterations;               // Número de veces que ha entrado en ejecución
   double avg_io_wait_time;      // Tiempo promedio en espera de E/S
   double avg_ready_wait_time;   // Tiempo promedio en cola de listos
   ProcessState state;           // Estado actual del proceso
   int total_io_waits;          // Número total de operaciones E/S
   time_t last_ready_time;       // Último momento que entró en ready
   time_t last_state_change;     // Último cambio de estado
   double total_io_wait_time;    // Tiempo total en espera de E/S
   double total_ready_wait_time; // Tiempo total en cola de listos
   int current_io_wait_time;     // Tiempo actual de espera de E/S
} ProcessControlBlock;

typedef struct {
   double avg_arrival_time;
   double avg_iterations;
   double avg_io_wait_time;
   double avg_ready_wait_time;
   int total_processes;
   int ready_processes;
   int io_waiting_processes;
} ProcessTable;

#endif