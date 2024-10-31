#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include "beehive_types.h"

void save_pcb_to_file(ProcessControlBlock* pcb);
ProcessControlBlock* load_pcb_from_file(int process_id);
void save_process_table(ProcessTable* table);
ProcessTable* load_process_table(void);
void update_process_table(ProcessControlBlock* pcb);
void init_process_manager(void);

#endif