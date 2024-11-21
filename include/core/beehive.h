#ifndef BEEHIVE_H
#define BEEHIVE_H
#include "../types/beehive_types.h"
#include "../types/scheduler_types.h"

// Inicialización y limpieza
void init_beehive_process(ProcessInfo* process_info, int id);// Inicializar el proceso de la apicultura de abejas
void cleanup_beehive_process(ProcessInfo* process_info);// Limpiar el proceso de la apicultura de abejas

// Control de hilos del proceso
void start_process_thread(ProcessInfo* process_info);// Iniciar el hilo del proceso
void stop_process_thread(ProcessInfo* process_info);// Detener el hilo del proceso
void* process_main_thread(void* arg);// El hilo del proceso principal (se ejecuta en un nuevo proceso)

// Gestión de cámaras y celdas
void init_chambers(ProcessInfo* process_info);/// Inicializar las cámaras
bool is_egg_position(int i, int j);// Comprobar si una posición es válida para la extracción de huevos
bool find_empty_cell_for_egg(Chamber* chamber, int* x, int* y);// Buscar una celda vacía para la extracción de huevos
bool find_empty_cell_for_honey(Chamber* chamber, int* x, int* y);// Buscar una celda vacía para la extracción de miel

// Gestión de recursos y producción
void manage_honey_production(ProcessInfo* process_info);// Gestionar la producción de miel
void manage_polen_collection(ProcessInfo* process_info);// Gestionar la recolección de polen
void manage_bee_lifecycle(ProcessInfo* process_info);// Gestionar la vida de las abejas

// Gestión de abejas
void handle_bee_death(ProcessInfo* process_info, int bee_index);// Manejar la muerte de una abeja
void create_new_bee(ProcessInfo* process_info, BeeType type);// Crear una abeja nueva
void process_eggs_hatching(ProcessInfo* process_info);// Procesar los huevos que están embarazados
void process_queen_egg_laying(ProcessInfo* process_info);// Procesar los huevos que están embarazados

// Monitoreo y estadísticas principales
void print_detailed_bee_status(ProcessInfo* process_info);// Imprimir el estado detallado de las abejas
void print_beehive_stats(ProcessInfo* process_info);// Imprimir las estadísticas del apiario
void print_chamber_matrix(ProcessInfo* process_info);// Imprimir la matriz de cámaras
void print_chamber_row(ProcessInfo* process_info, int start_index, int end_index);// Imprimir una fila de cámaras

// Utilidades de conteo
int count_queen_bees(ProcessInfo* process_info);// Contar las abejas reinas
bool check_new_queen(ProcessInfo* process_info);// Comprobar si hay una abeja reina

// Actualizar el contador de abejas + miel
void update_bees_and_honey_count(Beehive* hive);// Actualizar el contador de abejas y miel

#endif