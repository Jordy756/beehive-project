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
    int process_id;
    time_t arrival_time;         // Arrival time
    int iterations;              // Number of iterations
    int code_stack_progress;     // Code stack progress
    int io_wait_time;           // I/O wait time
    double avg_io_wait_time;    // Average I/O wait time
    double avg_ready_wait_time; // Average ready wait time
    ProcessState state;         // Needed for process state management
} ProcessControlBlock;

typedef struct {
    double avg_arrival_time;
    double avg_iterations;
    double avg_code_progress;
    double avg_io_wait_time;
    double avg_ready_wait_time;
    int total_processes;
} ProcessTable;

#endif