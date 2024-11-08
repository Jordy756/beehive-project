#ifndef BEEHIVE_H
#define BEEHIVE_H

#include "../types/beehive_types.h"

// Funciones principales de la colmena
void init_beehive(Beehive* hive, int id);
void cleanup_beehive(Beehive* hive);
void print_beehive_stats(Beehive* hive);
bool check_new_queen(Beehive* hive);
void print_chamber_matrix(Beehive* hive);

// Funciones de los hilos principales
void* honey_production_thread(void* arg);
void* polen_collection_thread(void* arg);
void* egg_hatching_thread(void* arg);

// Control de hilos
void start_hive_threads(Beehive* hive);
void stop_hive_threads(Beehive* hive);

#endif