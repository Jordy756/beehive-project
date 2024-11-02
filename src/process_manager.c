#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/core/process_manager.h"

void init_process_manager(void) {
    // Create data directory if it doesn't exist
    system("mkdir -p data");
}

void save_pcb_to_file(ProcessControlBlock* pcb) {
    FILE* file = fopen(PCB_FILE, "a");
    if (file) {
        fprintf(file, "%d,%ld,%d,%d,%d,%.2f,%.2f,%d,%d,%d,%d,%d,%s\n",
                pcb->process_id,
                pcb->arrival_time,
                pcb->iterations,
                pcb->code_stack_progress,
                pcb->io_wait_time,
                pcb->avg_io_wait_time,
                pcb->avg_ready_wait_time,
                pcb->bee_count,
                pcb->honey_count,
                pcb->egg_count,
                pcb->priority,
                pcb->state,
                pcb->status);
        fclose(file);
    }
}

ProcessControlBlock* load_pcb_from_file(int process_id) {
    FILE* file = fopen(PCB_FILE, "r");
    ProcessControlBlock* pcb = malloc(sizeof(ProcessControlBlock));
    char line[256];
    
    while (fgets(line, sizeof(line), file)) {
        int pid;
        sscanf(line, "%d,", &pid);
        if (pid == process_id) {
            // Parse the rest of the line and populate PCB
            sscanf(line, "%d,%ld,%d,%d,%d,%lf,%lf,%d,%d,%d,%d,%d,%s",
                   &pcb->process_id,
                   &pcb->arrival_time,
                   &pcb->iterations,
                   &pcb->code_stack_progress,
                   &pcb->io_wait_time,
                   &pcb->avg_io_wait_time,
                   &pcb->avg_ready_wait_time,
                   &pcb->bee_count,
                   &pcb->honey_count,
                   &pcb->egg_count,
                   &pcb->priority,
                   (int*)&pcb->state,
                   pcb->status);
            break;
        }
    }
    
    fclose(file);
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
    }
}

ProcessTable* load_process_table(void) {
    FILE* file = fopen(PROCESS_TABLE_FILE, "r");
    ProcessTable* table = malloc(sizeof(ProcessTable));
    
    if (file) {
        fscanf(file, "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%d,%ld",
               &table->avg_arrival_time,
               &table->avg_iterations,
               &table->avg_code_progress,
               &table->avg_io_wait_time,
               &table->total_io_wait_time,
               &table->avg_ready_wait_time,
               &table->total_ready_wait_time,
               &table->total_processes,
               &table->last_update);
        fclose(file);
    } else {
        // Initialize with default values if file doesn't exist
        memset(table, 0, sizeof(ProcessTable));
        table->last_update = time(NULL);
    }
    
    return table;
}

void update_process_table(ProcessControlBlock* pcb) {
    ProcessTable* table = load_process_table();
    
    // Update averages and totals
    table->total_processes++;
    table->avg_arrival_time = (table->avg_arrival_time * (table->total_processes - 1) + 
                              pcb->arrival_time) / table->total_processes;
    table->avg_iterations = (table->avg_iterations * (table->total_processes - 1) + 
                           pcb->iterations) / table->total_processes;
    table->avg_code_progress = (table->avg_code_progress * (table->total_processes - 1) + 
                               pcb->code_stack_progress) / table->total_processes;
    
    table->total_io_wait_time += pcb->io_wait_time;
    table->avg_io_wait_time = table->total_io_wait_time / table->total_processes;
    
    table->total_ready_wait_time += pcb->avg_ready_wait_time;
    table->avg_ready_wait_time = table->total_ready_wait_time / table->total_processes;
    
    table->last_update = time(NULL);
    
    save_process_table(table);
    free(table);
}