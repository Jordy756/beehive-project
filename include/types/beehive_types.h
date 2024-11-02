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
#define QUEEN_BIRTH_PROBABILITY 5

typedef enum {
    WORKER,
    QUEEN,
    SCOUT
} BeeType;

typedef struct Cell {
    int honey;
    int eggs;
} Cell;

typedef struct Chamber {
    Cell cells[MAX_CHAMBER_SIZE][MAX_CHAMBER_SIZE];
} Chamber;

// Definición completa de Beehive
typedef struct Beehive {
    int id;
    int bee_count;
    int max_bees;
    int honey_count;
    int egg_count;
    int max_eggs;
    struct Bee* bees;
    Chamber honey_chamber;
    Chamber brood_chamber;
    pthread_mutex_t chamber_mutex;
    sem_t resource_sem;
    ProcessState state;
} Beehive;

// Ahora podemos definir Bee con la referencia completa a Beehive
typedef struct Bee {
    int id;
    BeeType type;
    int polen_collected;
    int max_polen_capacity;
    bool is_alive;
    pthread_t thread;
    struct Beehive* hive;
} Bee;

typedef struct {
    Beehive* hive;
    int cell_x;
    int cell_y;
} EggHatchingArgs;

typedef struct {
    Bee* bee;
    Beehive* hive;
} BeeThreadArgs;

#endif