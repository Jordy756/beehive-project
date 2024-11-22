#ifndef FILE_MANAGER_TYPES_H
#define FILE_MANAGER_TYPES_H

#include <time.h>
#include <stdbool.h>
#include <json-c/json.h> // Biblioteca de JSON

// Constantes de rutas de los archivos
#define PCB_FILE "data/pcb.json" // Archivo de control de procesos
#define PROCESS_TABLE_FILE "data/process_table.json" // Archivo de tabla de procesos
#define BEEHIVE_HISTORY_FILE "data/beehive_history.json" // Archivo de historial de colmenas
#define MAX_FILENAME_LENGTH 100 // Longitud máxima de un nombre de archivo

// Estados del proceso
typedef enum {
   READY, // Proceso listo para ejecutarse
   RUNNING, // Proceso en ejecución
   WAITING // Proceso en espera
} ProcessState;

// Estructura para el bloque de control de procesos
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

// Estructura para la tabla de control de procesos
typedef struct {
   double avg_arrival_time; // Tiempo promedio de llegada a cola
   double avg_iterations; // Tiempo promedio de iteraciones
   double avg_io_wait_time; // Tiempo promedio en espera de E/S
   double avg_ready_wait_time; // Tiempo promedio en cola de listos
   int total_processes; // Número total de procesos
   int ready_processes; // Número de procesos listos
   int io_waiting_processes; // Número de procesos en espera de E/S
} ProcessTable;

#endif