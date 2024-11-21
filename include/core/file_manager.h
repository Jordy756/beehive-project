#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "../types/file_manager_types.h"
#include "../types/beehive_types.h"
#include "../types/scheduler_types.h"


void init_file_manager(void);// Inicializar el gestor de archivos


void save_pcb(ProcessControlBlock* pcb);// Guardar el PCB de un proceso en el archivo
void update_pcb_state(ProcessControlBlock* pcb, ProcessState new_state, Beehive* hive);// Actualizar el estado del PCB de un proceso
void init_pcb(ProcessControlBlock* pcb, int process_id);// Inicializar el PCB de un proceso
void create_pcb_for_beehive(ProcessInfo* process_info);// Crear el PCB de un proceso para la apicultura de abejas


void init_process_table(ProcessTable* table);// Inicializar la tabla de procesos
void save_process_table(ProcessTable* table);// Guardar la tabla de procesos en el archivo
void update_process_table(ProcessControlBlock* pcb);// Actualizar la tabla de procesos


void save_beehive_history(Beehive* hive);// Guardar el historial de la apicultura de abejas en el archivo


extern pthread_mutex_t pcb_mutex;// Mutex para el acceso a la tabla de procesos
extern pthread_mutex_t process_table_mutex;// Mutex para el acceso a la tabla de procesos
extern pthread_mutex_t history_mutex;// Mutex para el acceso al historial de la apicultura de abejas

#endif