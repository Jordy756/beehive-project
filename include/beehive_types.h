#ifndef BEEHIVE_TYPES_H
#define BEEHIVE_TYPES_H

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <time.h>

#define MAX_CHAMBER_SIZE 10
#define INITIAL_BEEHIVES 1
#define MIN_BEES 20
#define MAX_BEES 40
#define MIN_HONEY 20
#define MAX_HONEY 40
#define MIN_EGGS 20
#define MAX_EGGS 40
#define POLEN_TO_HONEY_RATIO 10
#define MIN_QUANTUM 2
#define MAX_QUANTUM 10
#define MAX_FILENAME_LENGTH 100

typedef enum {
    WORKER,
    QUEEN,
    SCOUT
} BeeType;

typedef enum {
    READY,
    RUNNING,
    WAITING,
    TERMINATED
} ProcessState;

struct Beehive;  // Forward declaration

typedef struct {
    int id;
    BeeType type;
    int polen_collected;
    int max_polen_capacity;
    bool is_alive;
    pthread_t thread;
    struct Beehive* hive;  // Reference to parent hive
} Bee;

typedef struct {
    int honey;
    int eggs;
} Cell;

typedef struct {
    Cell cells[MAX_CHAMBER_SIZE][MAX_CHAMBER_SIZE];
} Chamber;

// Forward declaration for EggData
struct EggData;

typedef struct Beehive {
    int id;
    int bee_count;
    int max_bees;    // Maximum number of bees the hive can hold
    int honey_count;
    int egg_count;
    int max_eggs;    // Maximum number of eggs the hive can hold
    Bee* bees;
    struct EggData* egg_data;  // Array to track egg threads
    Chamber honey_chamber;
    Chamber brood_chamber;
    pthread_mutex_t chamber_mutex;
    sem_t resource_sem;
    ProcessState state;
} Beehive;

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