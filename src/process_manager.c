#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/core/process_manager.h"

void init_process_manager(void) {
    system("mkdir -p data");
}

void save_pcb_to_file(ProcessControlBlock* pcb) {
    FILE* file = fopen(PCB_FILE, "r");
    FILE* temp = fopen("data/pcb_temp.txt", "w");
    char line[256];
    bool found = false;
    
    // Si el archivo original existe, buscar y actualizar el registro
    if (file) {
        while (fgets(line, sizeof(line), file)) {
            int current_id;
            sscanf(line, "%d,", &current_id);
            
            if (current_id == pcb->process_id) {
                // Actualizar el registro existente
                fprintf(temp, "%d,%ld,%d,%d,%d,%.2f,%.2f\n",
                        pcb->process_id,
                        pcb->arrival_time,
                        pcb->iterations,
                        pcb->code_stack_progress,
                        pcb->io_wait_time,
                        pcb->avg_io_wait_time,
                        pcb->avg_ready_wait_time);
                found = true;
            } else {
                // Copiar el registro existente sin cambios
                fprintf(temp, "%s", line);
            }
        }
        fclose(file);
    }
    
    // Si no se encontró el registro, añadir uno nuevo
    if (!found) {
        fprintf(temp, "%d,%ld,%d,%d,%d,%.2f,%.2f\n",
                pcb->process_id,
                pcb->arrival_time,
                pcb->iterations,
                pcb->code_stack_progress,
                pcb->io_wait_time,
                pcb->avg_io_wait_time,
                pcb->avg_ready_wait_time);
    }
    
    fclose(temp);
    
    // Reemplazar el archivo original con el temporal
    remove(PCB_FILE);
    rename("data/pcb_temp.txt", PCB_FILE);
}

void save_process_table(ProcessTable* table) {
    FILE* file = fopen(PROCESS_TABLE_FILE, "w");
    if (file) {
        fprintf(file, "%.2f,%.2f,%.2f,%.2f,%.2f,%d\n",
                table->avg_arrival_time,
                table->avg_iterations,
                table->avg_code_progress,
                table->avg_io_wait_time,
                table->avg_ready_wait_time,
                table->total_processes);
        fclose(file);
    }
}

ProcessTable* load_process_table(void) {
    FILE* file = fopen(PROCESS_TABLE_FILE, "r");
    ProcessTable* table = malloc(sizeof(ProcessTable));
    
    if (file) {
        fscanf(file, "%lf,%lf,%lf,%lf,%lf,%d",
               &table->avg_arrival_time,
               &table->avg_iterations,
               &table->avg_code_progress,
               &table->avg_io_wait_time,
               &table->avg_ready_wait_time,
               &table->total_processes);
        fclose(file);
    } else {
        memset(table, 0, sizeof(ProcessTable));
    }
    
    return table;
}

void update_process_table(ProcessControlBlock* pcb) {
    ProcessTable* table = load_process_table();
    
    table->total_processes++;
    table->avg_arrival_time = (table->avg_arrival_time * (table->total_processes - 1) +
                              pcb->arrival_time) / table->total_processes;
    table->avg_iterations = (table->avg_iterations * (table->total_processes - 1) +
                           pcb->iterations) / table->total_processes;
    table->avg_code_progress = (table->avg_code_progress * (table->total_processes - 1) +
                               pcb->code_stack_progress) / table->total_processes;
    table->avg_io_wait_time = (table->avg_io_wait_time * (table->total_processes - 1) +
                              pcb->io_wait_time) / table->total_processes;
    table->avg_ready_wait_time = (table->avg_ready_wait_time * (table->total_processes - 1) +
                                 pcb->avg_ready_wait_time) / table->total_processes;
    
    save_process_table(table);
    free(table);
}

void update_beehive_history(Beehive** beehives, int total_beehives) {
    FILE* file = fopen(BEEHIVE_HISTORY_FILE, "w");
    if (file) {
        for (int i = 0; i < total_beehives; i++) {
            if (beehives[i] != NULL) {
                int queens = 0, workers = 0, scouts = 0;
                for (int j = 0; j < beehives[i]->bee_count; j++) {
                    if (beehives[i]->bees[j].is_alive) {
                        switch (beehives[i]->bees[j].role.type) {
                            case QUEEN: queens++; break;
                            case WORKER: workers++; break;
                            case SCOUT: scouts++; break;
                        }
                    }
                }
                fprintf(file, "%d,%d,%d,%d,%d,%d,%d\n",
                        beehives[i]->id,
                        queens, workers, scouts,
                        beehives[i]->honey_count,
                        beehives[i]->egg_count,
                        beehives[i]->chamber_count);
            }
        }
        fclose(file);
    }
}

void save_beehive_history(BeehiveHistory* history) {
    FILE* file = fopen(BEEHIVE_HISTORY_FILE, "a");
    if (file) {
        fprintf(file, "%d,%d,%d,%d,%d,%d,%d\n",
                history->id,
                history->queens,
                history->workers,
                history->scouts,
                history->honey_count,
                history->egg_count,
                history->chamber_count);
        fclose(file);
    }
}

BeehiveHistory* load_beehive_history(int beehive_id) {
    FILE* file = fopen(BEEHIVE_HISTORY_FILE, "r");
    BeehiveHistory* history = malloc(sizeof(BeehiveHistory));
    char line[256];
    bool found = false;
    
    if (file) {
        while (fgets(line, sizeof(line), file)) {
            int id;
            sscanf(line, "%d,", &id);
            if (id == beehive_id) {
                sscanf(line, "%d,%d,%d,%d,%d,%d,%d",
                       &history->id,
                       &history->queens,
                       &history->workers,
                       &history->scouts,
                       &history->honey_count,
                       &history->egg_count,
                       &history->chamber_count);
                found = true;
                break;
            }
        }
        fclose(file);
    }
    
    if (!found) {
        memset(history, 0, sizeof(BeehiveHistory));
        history->id = beehive_id;
    }
    
    return history;
}