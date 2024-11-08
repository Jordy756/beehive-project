#ifndef BEEHIVE_TYPES_H
#define BEEHIVE_TYPES_H

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <time.h>
#include "process_manager_types.h"

// Constantes relacionadas con las colmenas
#define MAX_CHAMBER_SIZE 10
#define NUM_CHAMBERS 10
#define INITIAL_BEEHIVES 1
#define MAX_BEEHIVES 40
#define MIN_BEES 20
#define MAX_BEES 40
#define MIN_HONEY 20
#define MAX_HONEY 40
#define MIN_EGGS 20
#define MAX_EGGS 40
#define POLEN_TO_HONEY_RATIO 10
#define QUEEN_BIRTH_PROBABILITY 5

// Límites
#define MAX_EGGS_PER_HIVE 400
#define MAX_EGGS_PER_CHAMBER 40
#define MAX_HONEY_PER_HIVE 600

typedef enum {
    QUEEN,
    WORKER
} BeeType;

typedef struct {
    int id;
    BeeType type;
    int polen_collected;
    int max_polen_capacity;
    bool is_alive;
    struct Beehive* hive;
} Bee;

typedef struct {
    bool has_honey;     // true si hay miel
    bool has_egg;       // true si hay huevo
} Cell;

typedef struct {
    Cell cells[MAX_CHAMBER_SIZE][MAX_CHAMBER_SIZE];
    int honey_count;    // cantidad de miel en esta cámara
    int egg_count;      // cantidad de huevos en esta cámara
} Chamber;

typedef struct {
    pthread_t honey_production;
    pthread_t polen_collection;
    pthread_t egg_hatching;
    bool threads_running;
} HiveThreads;

typedef struct Beehive {
    int id;
    int bee_count;
    int max_bees;
    int honey_count;
    int egg_count;
    int max_eggs;
    Bee* bees;
    Chamber chambers[NUM_CHAMBERS];
    pthread_mutex_t chamber_mutex;
    sem_t resource_sem;
    ProcessState state;
    HiveThreads threads;
    bool has_queen;
} Beehive;

#endif