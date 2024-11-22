#ifndef BEEHIVE_TYPES_H
#define BEEHIVE_TYPES_H

#include <pthread.h> // Biblioteca de hilos
#include <semaphore.h> // Biblioteca de semáforos
#include <stdbool.h> // Biblioteca de tipos de datos
#include <time.h> // Biblioteca de tiempo
#include "file_manager_types.h" // Tipos de gestión de archivos
#include <signal.h> // Biblioteca de señales

// Constantes relacionadas con las colmenas
#define MAX_CHAMBER_SIZE 10 // Tamaño de la cámara 
#define NUM_CHAMBERS 10 // Número de cámaras
#define INITIAL_BEEHIVES 5 // Número inicial de colmenas
#define MAX_BEEHIVES 40 // Número máximo de colmenas
#define MIN_BEES 20 // Número mínimo de abejas
#define MAX_BEES 40 // Número máximo de abejas
#define MIN_HONEY 20 // Número mínimo de miel
#define MAX_HONEY 40 // Número máximo de miel
#define MIN_EGGS 20 // Número mínimo de huevos
#define MAX_EGGS 40 // Número máximo de huevos
#define POLEN_TO_HONEY_RATIO 10 // Tasa de conversión de polen a miel
#define QUEEN_BIRTH_PROBABILITY 25 // Probabilidad de nacimiento de la reina

// Constantes para polen y tiempo de vida
#define MIN_POLEN_PER_TRIP 1 // Polen mínimo por colmena
#define MAX_POLEN_PER_TRIP 5 // Polen máximo por colmena
#define MIN_POLEN_LIFETIME 100 // Tiempo mínimo de vida de polen
#define MAX_POLEN_LIFETIME 150 // Tiempo máximo de vida de polen
#define MAX_EGG_HATCH_TIME 10 // Tiempo máximo de vida de huevos

// Constantes para la reina
#define MIN_EGGS_PER_LAYING 120 // Número mínimo de huevos por colmena
#define MAX_EGGS_PER_LAYING 150 // Número máximo de huevos por colmena

// Límites
#define MAX_EGGS_PER_HIVE 400 // Número máximo de huevos por colmena
#define MAX_EGGS_PER_CHAMBER 40 // Número máximo de huevos por cámara
#define MAX_HONEY_PER_HIVE 600 // Número máximo de miel por colmena
#define MAX_HONEY_PER_CHAMBER 60 // Número máximo de miel por cámara

// Tipos de abejas
typedef enum {
    QUEEN, // Reina
    WORKER // Trabajadora
} BeeType;

// Estructura de celda
typedef struct {
    bool has_honey; // Indica si la celda tiene miel
    bool has_egg; // Indica si la celda tiene huevos
    time_t egg_lay_time; // Tiempo de vida de huevos
} Cell;

// Estructura de cámara
typedef struct {
    Cell cells[MAX_CHAMBER_SIZE][MAX_CHAMBER_SIZE]; // Matriz de celdas
    int honey_count; // Número de miel en la cámara
    int egg_count; // Número de huevos en la cámara
} Chamber;

// Estructura de abeja
typedef struct {
    int id; // ID de la abeja
    BeeType type; // Tipo de abeja
    int polen_collected; // Polen recolectado
    bool is_alive; // Indica si la abeja está viva
    time_t last_collection_time; // Tiempo de vida de polen
    time_t last_egg_laying_time; // Tiempo de vida de huevos
} Bee;

// Recursos de producción
typedef struct {
    int total_polen; // Polen total
    int polen_for_honey; // Polen para convertir en miel
    int total_polen_collected; // Polen recolectado
    pthread_mutex_t polen_mutex; // Mutex para el acceso a polen
} ProductionResources;

// Estructura base de colmena
typedef struct {
    int id; // ID de la colmena
    int bee_count; // Número de abejas en la colmena
    int honey_count; // Número de miel en la colmena
    int egg_count; // Número de huevos en la colmena
    int hatched_eggs; // Número de huevos que han nacido
    int dead_bees; // Número de abejas muertas
    int born_bees; // Número de abejas nacidas
    int produced_honey; // Suma de honey_count de todos los bees
    int bees_and_honey_count;  // Suma de bee_count + honey_count para el FSJ
    Bee* bees; // Array de bees
    Chamber chambers[NUM_CHAMBERS]; // Array de chambers
    pthread_mutex_t chamber_mutex; // Mutex para el acceso a chambers
    ProductionResources resources; // Recursos de producción
    volatile sig_atomic_t should_terminate; // Indica si se debe terminar
    bool should_create_new_hive; // Indica si se debe crear una nueva colmena
} Beehive;

#endif