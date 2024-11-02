#ifndef BEEHIVE_H
#define BEEHIVE_H

#include "../types/beehive_types.h"

void init_beehive(Beehive* hive, int id);
void* bee_lifecycle(void* arg);
void process_honey_production(Beehive* hive);
void process_egg_hatching(Beehive* hive);
void cleanup_beehive(Beehive* hive);
void print_beehive_stats(Beehive* hive);
bool check_new_queen(Beehive* hive);
void deposit_polen(Beehive* hive, int polen_amount);
void print_chamber_matrix(Beehive* hive);
void* egg_hatching_thread(void* arg);

#endif