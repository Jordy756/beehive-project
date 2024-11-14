#ifndef BEEHIVE_H
#define BEEHIVE_H

#include "../types/beehive_types.h"

// Funciones principales de la colmena
void init_beehive(Beehive* hive, int id);
void cleanup_beehive(Beehive* hive);
void print_beehive_stats(Beehive* hive);
bool check_new_queen(Beehive* hive);
void print_chamber_matrix(Beehive* hive);

// Nueva función para imprimir una fila de cámaras
void print_chamber_row(Beehive* hive, int start_index, int end_index);

// Función principal del hilo de la colmena
void* hive_main_thread(void* arg);

// Funciones de gestión de la colmena (llamadas por el hilo principal)
void manage_honey_production(Beehive* hive);
void manage_polen_collection(Beehive* hive);
void manage_bee_lifecycle(Beehive* hive);

// Control de hilos
void start_hive_thread(Beehive* hive);
void stop_hive_thread(Beehive* hive);

// Función auxiliar para inicializar cámaras
void init_chambers(Beehive* hive);

#endif