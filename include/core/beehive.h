#ifndef BEEHIVE_H
#define BEEHIVE_H

#include "../types/beehive_types.h"

// Funciones principales
void init_beehive(Beehive* hive, int id);
void* bee_lifecycle(void* arg);
void process_honey_production(Beehive* hive);
void process_egg_hatching(Beehive* hive);
void cleanup_beehive(Beehive* hive);
void print_beehive_stats(Beehive* hive);
bool check_new_queen(Beehive* hive);

// Funciones específicas para tipos de abejas
void queen_behavior(Bee* bee);
void worker_behavior(Bee* bee);
void scout_behavior(Bee* bee);

// Funciones para manejo de estructura hexagonal
void init_hex_structure(Chamber* chamber);
void connect_adjacent_cells(Chamber* chamber);
void deposit_polen(Beehive* hive, int polen_amount);
void* egg_hatching_thread(void* arg);
void print_chamber_matrix(Beehive* hive);

#endif