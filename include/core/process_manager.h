#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include "../types/process_manager_types.h"
#include "../types/beehive_types.h"

void init_process_manager(void);
void save_pcb_to_file(ProcessControlBlock* pcb);
void save_process_table(ProcessTable* table);
ProcessTable* load_process_table(void);
void update_process_table(ProcessControlBlock* pcb);

// Nuevas funciones para el manejo del historial de colmenas
void update_beehive_history(Beehive** beehives, int total_beehives);
void save_beehive_history(BeehiveHistory* history);

#endif
