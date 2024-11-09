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
#define QUEEN_BIRTH_PROBABILITY 90
#define EGG_HATCH_PROBABILITY 1

// Constantes para polen y tiempo de vida
#define MIN_POLEN_PER_TRIP 1
#define MAX_POLEN_PER_TRIP 5
#define MIN_POLEN_LIFETIME 100
#define MAX_POLEN_LIFETIME 150
#define MIN_EGG_HATCH_TIME 1
#define MAX_EGG_HATCH_TIME 10

// Constantes para la reina
#define MIN_EGGS_PER_LAYING 100
#define MAX_EGGS_PER_LAYING 120

// Límites
#define MAX_EGGS_PER_HIVE 400
#define MAX_EGGS_PER_CHAMBER 40
#define MAX_HONEY_PER_HIVE 600
#define MAX_HONEY_PER_CHAMBER 60

typedef enum {
   QUEEN,
   WORKER
} BeeType;

typedef struct {
   int id;
   BeeType type;
   int polen_collected;
   bool is_alive;
   struct Beehive* hive;
   time_t last_collection_time;  // Para rastrear cuándo recolectó polen por última vez
   time_t last_egg_laying_time;  // Para rastrear cuándo la reina puso huevos por última vez
} Bee;

typedef struct {
   bool has_honey;     // true si hay miel
   bool has_egg;       // true si hay huevo
   time_t egg_lay_time; // tiempo cuando se puso el huevo
} Cell;

typedef struct {
   Cell cells[MAX_CHAMBER_SIZE][MAX_CHAMBER_SIZE];
   int honey_count;    // cantidad de miel en esta cámara
   int egg_count;      // cantidad de huevos en esta cámara
} Chamber;

// Estructura para gestionar los recursos de producción
typedef struct {
   int total_polen;        // Polen total recolectado
   int polen_for_honey;    // Polen pendiente de convertir en miel
   pthread_mutex_t polen_mutex;
} ProductionResources;

typedef struct {
   pthread_t honey_production;
   pthread_t polen_collection;
   pthread_t egg_hatching;
   bool threads_running;
   ProductionResources resources;  // Nuevo: recursos compartidos entre hilos
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