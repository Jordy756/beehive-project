#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include "../types/process_manager_types.h"

void init_process_manager(void);
void save_pcb_to_file(ProcessControlBlock* pcb);
void save_process_table(ProcessTable* table);
ProcessTable* load_process_table(void);
void update_process_table(ProcessControlBlock* pcb);

#endif