#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "../include/core/process_manager.h"
#include "../include/core/utils.h"

void init_process_manager(void) {
    struct stat st = {0};
    if (stat("data", &st) == -1) {
        if (mkdir("data", 0700) == -1) {
            log_message("Error creating data directory");
            exit(1);
        }
    }

    // Inicializar/limpiar archivos
    FILE* pcb_file = fopen(PCB_FILE, "w");
    if (pcb_file) fclose(pcb_file);

    FILE* pt_file = fopen(PROCESS_TABLE_FILE, "w");
    if (pt_file) fclose(pt_file);

    FILE* history_file = fopen(BEEHIVE_HISTORY_FILE, "w");
    if (history_file) fclose(history_file);
}

void save_pcb_to_file(ProcessControlBlock* pcb) {
    FILE* file = fopen(PCB_FILE, "w");
    if (file) {
        fprintf(file, "%d,%ld,%d,%d,%d,%.2f,%.2f,%d,%d,%s\n",
                pcb->process_id,
                pcb->arrival_time,
                pcb->iterations,
                pcb->code_stack_progress,
                pcb->io_wait_time,
                pcb->avg_io_wait_time,
                pcb->avg_ready_wait_time,
                pcb->priority,
                pcb->state,
                pcb->status);
        fclose(file);

        char message[100];
        snprintf(message, sizeof(message), 
                "PCB updated for process %d: iterations=%d, state=%d",
                pcb->process_id, pcb->iterations, pcb->state);
        log_message(message);
    } else {
        log_message("Error saving PCB to file");
    }
}

ProcessControlBlock* load_pcb_from_file(int process_id) {
    FILE* file = fopen(PCB_FILE, "r");
    if (!file) {
        log_message("Error opening PCB file for reading");
        return NULL;
    }

    ProcessControlBlock* pcb = malloc(sizeof(ProcessControlBlock));
    char line[256];
    bool found = false;

    while (fgets(line, sizeof(line), file)) {
        int pid;
        sscanf(line, "%d,", &pid);
        if (pid == process_id) {
            sscanf(line, "%d,%ld,%d,%d,%d,%lf,%lf,%d,%d,%19s",
                   &pcb->process_id,
                   &pcb->arrival_time,
                   &pcb->iterations,
                   &pcb->code_stack_progress,
                   &pcb->io_wait_time,
                   &pcb->avg_io_wait_time,
                   &pcb->avg_ready_wait_time,
                   &pcb->priority,
                   (int*)&pcb->state,
                   pcb->status);
            found = true;
            break;
        }
    }

    fclose(file);
    if (!found) {
        free(pcb);
        return NULL;
    }
    return pcb;
}

void save_process_table(ProcessTable* table) {
    FILE* file = fopen(PROCESS_TABLE_FILE, "w");
    if (file) {
        fprintf(file, "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d,%ld\n",
                table->avg_arrival_time,
                table->avg_iterations,
                table->avg_code_progress,
                table->avg_io_wait_time,
                table->total_io_wait_time,
                table->avg_ready_wait_time,
                table->total_ready_wait_time,
                table->total_processes,
                table->last_update);
        fclose(file);

        char message[100];
        snprintf(message, sizeof(message), 
                "Process Table updated: processes=%d, avg_iterations=%.2f",
                table->total_processes, table->avg_iterations);
        log_message(message);
    } else {
        log_message("Error saving Process Table to file");
    }
}

ProcessTable* load_process_table(void) {
    FILE* file = fopen(PROCESS_TABLE_FILE, "r");
    ProcessTable* table = malloc(sizeof(ProcessTable));

    if (file) {
        if (fscanf(file, "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%d,%ld",
                   &table->avg_arrival_time,
                   &table->avg_iterations,
                   &table->avg_code_progress,
                   &table->avg_io_wait_time,
                   &table->total_io_wait_time,
                   &table->avg_ready_wait_time,
                   &table->total_ready_wait_time,
                   &table->total_processes,
                   &table->last_update) != 9) {
            // If we couldn't read all values, initialize with defaults
            memset(table, 0, sizeof(ProcessTable));
            table->last_update = time(NULL);
        }
        fclose(file);
    } else {
        // Initialize with default values if file doesn't exist
        memset(table, 0, sizeof(ProcessTable));
        table->last_update = time(NULL);
        log_message("Error opening Process Table file for reading, initializing with defaults");
    }

    return table;
}

void update_process_table(ProcessControlBlock* pcb) {
    ProcessTable* table = load_process_table();
    
    // Update averages and totals
    table->total_processes++;
    table->avg_arrival_time = ((table->avg_arrival_time * (table->total_processes - 1)) +
                              pcb->arrival_time) / table->total_processes;
    table->avg_iterations = ((table->avg_iterations * (table->total_processes - 1)) +
                           pcb->iterations) / table->total_processes;
    table->avg_code_progress = ((table->avg_code_progress * (table->total_processes - 1)) +
                               pcb->code_stack_progress) / table->total_processes;
    
    table->total_io_wait_time += pcb->io_wait_time;
    table->avg_io_wait_time = table->total_io_wait_time / table->total_processes;
    
    table->total_ready_wait_time += pcb->avg_ready_wait_time;
    table->avg_ready_wait_time = table->total_ready_wait_time / table->total_processes;
    
    table->last_update = time(NULL);
    
    save_process_table(table);
    free(table);
}

void update_beehive_history(int beehive_id, int bees, int honey, int eggs) {
    FILE* file = fopen(BEEHIVE_HISTORY_FILE, "a");
    if (file) {
        fprintf(file, "%d,%d,%d,%d,%ld\n", 
                beehive_id, bees, honey, eggs, time(NULL));
        fclose(file);
        
        char message[100];
        snprintf(message, sizeof(message), 
                "History updated for beehive %d: Bees=%d, Honey=%d, Eggs=%d",
                beehive_id, bees, honey, eggs);
        log_message(message);
    } else {
        log_message("Error updating Beehive History file");
    }
}

BeehiveHistoryEntry* load_beehive_history(int beehive_id, int* entry_count) {
    FILE* file = fopen(BEEHIVE_HISTORY_FILE, "r");
    if (!file) {
        *entry_count = 0;
        return NULL;
    }

    BeehiveHistoryEntry* entries = malloc(sizeof(BeehiveHistoryEntry) * MAX_HISTORY_ENTRIES);
    *entry_count = 0;
    char line[256];

    while (fgets(line, sizeof(line), file) && *entry_count < MAX_HISTORY_ENTRIES) {
        BeehiveHistoryEntry entry;
        if (sscanf(line, "%d,%d,%d,%d,%ld", 
                   &entry.beehive_id, &entry.bee_count, &entry.honey_count,
                   &entry.egg_count, &entry.timestamp) == 5) {
            if (entry.beehive_id == beehive_id) {
                entries[*entry_count] = entry;
                (*entry_count)++;
            }
        }
    }

    fclose(file);
    if (*entry_count == 0) {
        free(entries);
        return NULL;
    }
    return entries;
}

void get_beehive_stats(int beehive_id, int* total_bees, int* total_honey, int* total_eggs) {
    int entry_count;
    BeehiveHistoryEntry* entries = load_beehive_history(beehive_id, &entry_count);
    
    if (entries && entry_count > 0) {
        BeehiveHistoryEntry latest = entries[entry_count - 1];
        *total_bees = latest.bee_count;
        *total_honey = latest.honey_count;
        *total_eggs = latest.egg_count;
        free(entries);
    } else {
        *total_bees = 0;
        *total_honey = 0;
        *total_eggs = 0;
    }
}

void clear_beehive_history(void) {
    FILE* file = fopen(BEEHIVE_HISTORY_FILE, "w");
    if (file) {
        fclose(file);
        log_message("Beehive history cleared");
    } else {
        log_message("Error clearing beehive history");
    }
}