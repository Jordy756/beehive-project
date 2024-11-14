#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "../types/file_manager_types.h"
#include "../types/beehive_types.h"

// Inicialización del sistema de archivos
void init_file_manager(void);

// Funciones para PCB
void write_to_pcb_file(FILE* file, ProcessControlBlock* pcb);
void save_pcb(ProcessControlBlock* pcb);
void init_pcb(ProcessControlBlock* pcb, int process_id);
void update_pcb_state(ProcessControlBlock* pcb, ProcessState new_state);

// Funciones para tabla de procesos
void save_process_table(ProcessTable* table);
ProcessTable* load_process_table(void);
void update_process_table(ProcessControlBlock* pcb);

// Funciones para historial de colmena
void save_beehive_history(Beehive* hive);

#endif