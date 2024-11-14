#ifndef FILE_MANAGER_TYPES_H
#define FILE_MANAGER_TYPES_H

#include <time.h>
#include <stdbool.h>

// Constants
#define PCB_FILE "data/pcb.txt"
#define PROCESS_TABLE_FILE "data/process_table.txt"
#define BEEHIVE_HISTORY_FILE "data/beehive_history.txt"
#define MAX_FILENAME_LENGTH 100

typedef enum {
    READY,
    RUNNING,
    WAITING
} ProcessState;

typedef struct {
    int process_id;
    time_t arrival_time;
    int iterations;
    double avg_io_wait_time;
    double avg_ready_wait_time;
    ProcessState state;
    
    // Variables de control para cálculos de promedios
    int total_io_waits;
    time_t last_ready_time;
    time_t last_state_change;
    double total_io_wait_time;
    double total_ready_wait_time;
    int current_io_wait_time;  // Movido aquí desde ProcessInfo
} ProcessControlBlock;

typedef struct {
    double avg_arrival_time;
    double avg_iterations;
    double avg_io_wait_time;
    double avg_ready_wait_time;
    int total_processes;
} ProcessTable;

typedef struct {
    int beehive_id;
    int egg_count;
    int hatched_eggs;
    int dead_bees;
    int born_bees;
    int total_bees;
    int produced_honey;
    int total_honey;
    time_t timestamp;
} BeehiveHistory;

#endif