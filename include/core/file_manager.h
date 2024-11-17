#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "../types/file_manager_types.h"
#include "../types/beehive_types.h"

// System initialization
void init_file_manager(void);

// PCB operations
void save_pcb(ProcessControlBlock* pcb);
void update_pcb_state(ProcessControlBlock* pcb, ProcessState new_state);
void init_pcb(ProcessControlBlock* pcb, int process_id);

// Process table operations
void save_process_table(ProcessTable* table);
ProcessTable* load_process_table(void);
void update_process_table(ProcessControlBlock* pcb);

// Beehive history operations
void save_beehive_history(Beehive* hive);

// Thread safety
extern pthread_mutex_t pcb_mutex;
extern pthread_mutex_t process_table_mutex;
extern pthread_mutex_t history_mutex;

#endif