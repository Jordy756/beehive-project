#ifndef FILE_MANAGER_TYPES_H
#define FILE_MANAGER_TYPES_H

#include <time.h>
#include <stdbool.h>
#include <json-c/json.h>

// Constants
#define PCB_FILE "data/pcb.json"
#define PROCESS_TABLE_FILE "data/process_table.json"
#define BEEHIVE_HISTORY_FILE "data/beehive_history.json"
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
    int total_io_waits;
    time_t last_ready_time;
    time_t last_state_change;
    double total_io_wait_time;
    double total_ready_wait_time;
    int current_io_wait_time;
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

typedef struct {
    int beehive_id;
    time_t timestamp;
    int current_eggs;
    int hatched_eggs;
    int laid_eggs;
    int dead_bees;
    int born_bees;
    int current_bees;
    int total_polen_collected;
    int polen_for_honey;
    int produced_honey;
    int total_honey;
} BeehiveHistory;

#endif