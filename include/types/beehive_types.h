#ifndef BEEHIVE_TYPES_H
#define BEEHIVE_TYPES_H


#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <time.h>
#include "file_manager_types.h"
#include <signal.h>


// Constantes relacionadas con las colmenas
#define MAX_CHAMBER_SIZE 10
#define NUM_CHAMBERS 10
#define MAX_BEEHIVES 40
#define MIN_BEES 20
#define MAX_BEES 40
#define MIN_HONEY 20
#define MAX_HONEY 40
#define MIN_EGGS 20
#define MAX_EGGS 40
#define POLEN_TO_HONEY_RATIO 10
#define QUEEN_BIRTH_PROBABILITY 90


// Constantes para polen y tiempo de vida
#define MIN_POLEN_PER_TRIP 1
#define MAX_POLEN_PER_TRIP 5
#define MIN_POLEN_LIFETIME 100
#define MAX_POLEN_LIFETIME 150
#define MAX_EGG_HATCH_TIME 10


// Constantes para la reina
#define MIN_EGGS_PER_LAYING 5
#define MAX_EGGS_PER_LAYING 10


// Límites
#define MAX_EGGS_PER_HIVE 400
#define MAX_EGGS_PER_CHAMBER 40
#define MAX_HONEY_PER_HIVE 600
#define MAX_HONEY_PER_CHAMBER 60


// Tipos de abejas
typedef enum {
   QUEEN,
   WORKER
} BeeType;


// Estructura de celda
typedef struct {
   bool has_honey;
   bool has_egg;
   time_t egg_lay_time;
} Cell;


// Estructura de cámara
typedef struct {
   Cell cells[MAX_CHAMBER_SIZE][MAX_CHAMBER_SIZE];
   int honey_count;
   int egg_count;
} Chamber;


// Estructura de abeja
typedef struct {
   int id;
   BeeType type;
   int polen_collected;
   bool is_alive;
   time_t last_collection_time;
   time_t last_egg_laying_time;
} Bee;


// Recursos de producción
typedef struct {
   int total_polen;
   int polen_for_honey;
   int total_polen_collected;
   pthread_mutex_t polen_mutex;
} ProductionResources;


// Estructura base de colmena
typedef struct {
   int id;
   int bee_count;
   int honey_count;
   int egg_count;
   int hatched_eggs;
   int dead_bees;
   int born_bees;
   int produced_honey;
   Bee* bees;
   Chamber chambers[NUM_CHAMBERS];
   pthread_mutex_t chamber_mutex;
   ProductionResources resources;
   volatile sig_atomic_t should_terminate;
   bool should_create_new_hive;
} Beehive;


#endif