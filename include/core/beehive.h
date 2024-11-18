#ifndef BEEHIVE_H
#define BEEHIVE_H


#include "../types/beehive_types.h"
#include "../types/scheduler_types.h"


// Inicialización y limpieza
void init_beehive_process(ProcessInfo* process_info, int id);
void cleanup_beehive_process(ProcessInfo* process_info);


// Control de hilos del proceso
void start_process_thread(ProcessInfo* process_info);
void stop_process_thread(ProcessInfo* process_info);
void* process_main_thread(void* arg);


// Gestión de cámaras y celdas
void init_chambers(ProcessInfo* process_info);
bool is_egg_position(int i, int j);
bool find_empty_cell_for_egg(Chamber* chamber, int* x, int* y);
bool find_empty_cell_for_honey(Chamber* chamber, int* x, int* y);


// Gestión de recursos y producción
void manage_honey_production(ProcessInfo* process_info);
void manage_polen_collection(ProcessInfo* process_info);
void manage_bee_lifecycle(ProcessInfo* process_info);


// Gestión de abejas
void handle_bee_death(ProcessInfo* process_info, int bee_index);
void create_new_bee(ProcessInfo* process_info, BeeType type);
void process_eggs_hatching(ProcessInfo* process_info);
void process_queen_egg_laying(ProcessInfo* process_info);


// Monitoreo y estadísticas
void print_beehive_stats(ProcessInfo* process_info);
void print_chamber_matrix(ProcessInfo* process_info);
void print_chamber_row(ProcessInfo* process_info, int start_index, int end_index);
bool check_new_queen(ProcessInfo* process_info);


// Utilidades de conteo
int count_alive_bees(ProcessInfo* process_info);
int count_queen_bees(ProcessInfo* process_info);
void update_bee_counts(ProcessInfo* process_info, int* alive_count, int* queen_count, int* worker_count);


#endif
