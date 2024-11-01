#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include "../types/process_manager_types.h"

// Funciones existentes
void save_pcb_to_file(ProcessControlBlock* pcb);
ProcessControlBlock* load_pcb_from_file(int process_id);
void save_process_table(ProcessTable* table);
ProcessTable* load_process_table(void);
void update_process_table(ProcessControlBlock* pcb);
void init_process_manager(void);

// Nuevas funciones para el historial
void update_beehive_history(int beehive_id, int bees, int honey, int eggs);
BeehiveHistoryEntry* load_beehive_history(int beehive_id, int* entry_count);
void get_beehive_stats(int beehive_id, int* total_bees, int* total_honey, int* total_eggs);
void clear_beehive_history(void);

#endif