#ifndef PROCESS_MANAGER_TYPES_H
#define PROCESS_MANAGER_TYPES_H

#include <time.h>
#include <stdbool.h>

// Constants
#define PCB_FILE "data/pcb.txt"
#define PROCESS_TABLE_FILE "data/process_table.txt"
#define MAX_FILENAME_LENGTH 100

typedef enum {
    READY,
    RUNNING,
    WAITING,
    TERMINATED
} ProcessState;

typedef struct {
    time_t arrival_time;
    int iterations;
    int code_stack_progress;
    int io_wait_time;
    double avg_io_wait_time;
    double avg_ready_wait_time;
    int bee_count;
    int honey_count;
    int egg_count;
    int process_id;
    int priority;
    ProcessState state;
    char status[20];
} ProcessControlBlock;

typedef struct {
    double avg_arrival_time;
    double avg_iterations;
    double avg_code_progress;
    double avg_io_wait_time;
    double total_io_wait_time;
    double avg_ready_wait_time;
    double total_ready_wait_time;
    int total_processes;
    time_t last_update;
} ProcessTable;

#endif