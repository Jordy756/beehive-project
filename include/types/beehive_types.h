#ifndef BEEHIVE_TYPES_H
#define BEEHIVE_TYPES_H

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <time.h>
#include "process_manager_types.h"

// Constantes relacionadas con las colmenas
#define MAX_CHAMBER_SIZE 10
#define MAX_CHAMBERS 20
#define INITIAL_BEEHIVES 1
#define MAX_BEEHIVES 40
#define MIN_BEES 20
#define MAX_BEES 40
#define MIN_HONEY 20
#define MAX_HONEY 40
#define MIN_EGGS 20
#define MAX_EGGS 40
#define MAX_TOTAL_HONEY 1400  // Nuevo: límite máximo de miel por colmena
#define MAX_TOTAL_EGGS 600    // Nuevo: límite máximo de huevos por colmena
#define POLEN_TO_HONEY_RATIO 10
#define QUEEN_BIRTH_PROBABILITY 5
#define EGG_LAYING_INTERVAL 5  // Nuevo: intervalo en segundos para poner huevos
#define HONEY_PRODUCTION_RATE 2 // Nuevo: tasa de producción de miel

// Direcciones para la estructura hexagonal
typedef enum {
    HEX_TOP_RIGHT,
    HEX_RIGHT,
    HEX_BOTTOM_RIGHT,
    HEX_BOTTOM_LEFT,
    HEX_LEFT,
    HEX_TOP_LEFT,
    HEX_DIRECTIONS
} HexDirection;

// Tipos de abejas con roles específicos
typedef enum {
    QUEEN,   // Pone huevos y mantiene la cohesión de la colmena
    WORKER,  // Recolecta polen y produce miel
    SCOUT    // Explora y encuentra nuevas fuentes de polen
} BeeType;

// Roles específicos para cada tipo de abeja
typedef struct {
    BeeType type;
    union {
        struct {  // Para QUEEN
            int eggs_laid;
            time_t last_egg_time;
            int egg_laying_rate;  // Nuevo: tasa de puesta de huevos
        } queen;
        struct {  // Para WORKER
            int honey_produced;
            int polen_carried;
            time_t last_collection_time;  // Nuevo: tiempo de última recolección
        } worker;
        struct {  // Para SCOUT
            bool found_food_source;
            int discovered_locations;
            time_t last_search_time;  // Nuevo: tiempo de última búsqueda
        } scout;
    } role_data;
} BeeRole;

typedef struct {
    int id;
    BeeRole role;
    int polen_collected;
    int max_polen_capacity;
    bool is_alive;
    pthread_t thread;
    struct Beehive* hive;  // Reference to parent hive
    time_t creation_time;  // Nuevo: tiempo de creación de la abeja
} Bee;

// Estructura hexagonal para las celdas
typedef struct {
    int honey;
    int eggs;
    struct {
        int x;
        int y;
    } hex_coords;  // Coordenadas en el espacio hexagonal
    HexDirection adjacent[HEX_DIRECTIONS];  // Conexiones a celdas adyacentes
} Cell;

typedef struct {
    Cell cells[MAX_CHAMBER_SIZE][MAX_CHAMBER_SIZE];
    bool is_brood_chamber;  // true para cámara de cría, false para cámara de miel
} Chamber;

// Forward declaration for EggData
struct EggData;

typedef struct Beehive {
    int id;
    int bee_count;
    int max_bees;
    int honey_count;
    int egg_count;
    int max_eggs;
    int chamber_count;  // Número actual de cámaras (máx 20)
    Bee* bees;
    struct EggData* egg_data;
    Chamber chambers[MAX_CHAMBERS];  // Array de cámaras (máximo 20)
    pthread_mutex_t chamber_mutex;
    sem_t resource_sem;
    ProcessState state;
    time_t last_activity_time;  // Nuevo: tiempo de última actividad
} Beehive;

typedef struct {
    Beehive* hive;
    int chamber_index;
    int cell_x;
    int cell_y;
} EggHatchingArgs;

typedef struct {
    Bee* bee;
    Beehive* hive;
} BeeThreadArgs;

#endif