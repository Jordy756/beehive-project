#ifndef BEEHIVE_TYPES_H
#define BEEHIVE_TYPES_H

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <time.h>
#include "process_manager_types.h"

#define MAX_CHAMBER_SIZE 10
#define INITIAL_BEEHIVES 1
#define MIN_BEES 20
#define MAX_BEES 40
#define MIN_HONEY 20
#define MAX_HONEY 40
#define MIN_EGGS 20
#define MAX_EGGS 40
#define POLEN_TO_HONEY_RATIO 10
#define QUEEN_BIRTH_PROBABILITY 90  // 5% chance of queen birth

typedef enum {
    WORKER,
    QUEEN,
    SCOUT
} BeeType;

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

// Estructuras para pasar argumentos a los threads
typedef struct {
    struct Beehive* hive;
    int cell_x;
    int cell_y;
} EggHatchingArgs;

typedef struct {
    Bee* bee;
    struct Beehive* hive;
} BeeThreadArgs;

typedef struct Beehive {
    int id;
    int bee_count;
    int max_bees;    // Maximum number of bees the hive can hold
    int honey_count;
    int egg_count;
    int max_eggs;    // Maximum number of eggs the hive can hold
    Bee* bees;
    Chamber honey_chamber;
    Chamber brood_chamber;
    pthread_mutex_t chamber_mutex;
    sem_t resource_sem;
    ProcessState state;
} Beehive;

#endif