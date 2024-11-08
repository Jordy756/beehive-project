#ifndef BEEHIVE_TYPES_H
#define BEEHIVE_TYPES_H

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <time.h>
#include "process_manager_types.h"

// Constantes relacionadas con las colmenas
#define MAX_CHAMBER_SIZE 10
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

// Nuevos límites
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
    int honey;
    int eggs;
} Cell;

typedef struct {
    Cell cells[MAX_CHAMBER_SIZE][MAX_CHAMBER_SIZE];
    int total_eggs;
} Chamber;

// Estructura para los 3 hilos principales de la colmena
typedef struct {
    pthread_t honey_production;    // Hilo de producción de miel
    pthread_t polen_collection;    // Hilo de recolección de polen
    pthread_t egg_hatching;        // Hilo de nacimiento de abejas
    bool threads_running;          // Flag para control de hilos
} HiveThreads;

typedef struct Beehive {
    int id;
    int bee_count;
    int max_bees;
    int honey_count;
    int egg_count;
    int max_eggs;
    Bee* bees;
    Chamber honey_chamber;
    Chamber brood_chamber;
    pthread_mutex_t chamber_mutex;
    sem_t resource_sem;
    ProcessState state;
    HiveThreads threads;
    bool has_queen;
} Beehive;

#endif