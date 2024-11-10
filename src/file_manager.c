#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/core/file_manager.h"

void init_file_manager(void) {
    system("mkdir -p data");
}

void save_pcb(ProcessControlBlock* pcb) {
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
                // Actualizar el registro existente con formato más legible
                fprintf(temp, "============= PCB Colmena %d =============\n", pcb->process_id);
                fprintf(temp, "ID Proceso: %d\n", pcb->process_id);
                fprintf(temp, "Tiempo de llegada: %ld\n", pcb->arrival_time);
                fprintf(temp, "Iteraciones totales: %d\n", pcb->iterations);
                fprintf(temp, "Progreso en pila de código: %d\n", pcb->code_stack_progress);
                fprintf(temp, "Tiempo de espera en E/S: %d\n", pcb->io_wait_time);
                fprintf(temp, "Promedio tiempo espera E/S: %.2f\n", pcb->avg_io_wait_time);
                fprintf(temp, "Promedio tiempo espera Ready: %.2f\n", pcb->avg_ready_wait_time);
                fprintf(temp, "Estado actual: %s\n", 
                    pcb->state == READY ? "READY" :
                    pcb->state == RUNNING ? "RUNNING" :
                    pcb->state == WAITING ? "WAITING" : "TERMINATED");
                fprintf(temp, "=========================================\n\n");
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
        fprintf(temp, "============= PCB Colmena %d =============\n", pcb->process_id);
        fprintf(temp, "ID Proceso: %d\n", pcb->process_id);
        fprintf(temp, "Tiempo de llegada: %ld\n", pcb->arrival_time);
        fprintf(temp, "Iteraciones totales: %d\n", pcb->iterations);
        fprintf(temp, "Progreso en pila de código: %d\n", pcb->code_stack_progress);
        fprintf(temp, "Tiempo de espera en E/S: %d\n", pcb->io_wait_time);
        fprintf(temp, "Promedio tiempo espera E/S: %.2f\n", pcb->avg_io_wait_time);
        fprintf(temp, "Promedio tiempo espera Ready: %.2f\n", pcb->avg_ready_wait_time);
        fprintf(temp, "Estado actual: %s\n", 
            pcb->state == READY ? "READY" :
            pcb->state == RUNNING ? "RUNNING" :
            pcb->state == WAITING ? "WAITING" : "TERMINATED");
        fprintf(temp, "=========================================\n\n");
    }
    
    fclose(temp);
    
    // Reemplazar el archivo original con el temporal
    remove(PCB_FILE);
    rename("data/pcb_temp.txt", PCB_FILE);
    
    // Asegurar que los datos se escriban inmediatamente
    temp = fopen(PCB_FILE, "r+");
    if (temp) {
        fflush(temp);
        fclose(temp);
    }
}

void save_process_table(ProcessTable* table) {
    FILE* file = fopen(PROCESS_TABLE_FILE, "w");
    if (file) {
        fprintf(file, "Promedio tiempo de llegada: %.2f\n", table->avg_arrival_time);
        fprintf(file, "Promedio iteraciones: %.2f\n", table->avg_iterations);
        fprintf(file, "Promedio progreso en pila de código: %.2f\n", table->avg_code_progress);
        fprintf(file, "Promedio tiempo de espera en E/S: %.2f\n", table->avg_io_wait_time);
        fprintf(file, "Promedio tiempo de espera Ready: %.2f\n", table->avg_ready_wait_time);
        fprintf(file, "Total de procesos: %d\n", table->total_processes);
        fflush(file);  // Asegurar que los datos se escriban inmediatamente
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
        // Initialize with default values if file doesn't exist
        memset(table, 0, sizeof(ProcessTable));
    }
    
    return table;
}

void update_process_table(ProcessControlBlock* pcb) {
    ProcessTable* table = load_process_table();
    
    // Update averages and totals
    table->total_processes++;
    table->avg_arrival_time = (table->avg_arrival_time * (table->total_processes - 1) + pcb->arrival_time) / table->total_processes;
    table->avg_iterations = (table->avg_iterations * (table->total_processes - 1) + pcb->iterations) / table->total_processes;
    table->avg_code_progress = (table->avg_code_progress * (table->total_processes - 1) + pcb->code_stack_progress) / table->total_processes;
    
    save_process_table(table);
    free(table);
}

void save_beehive_history(Beehive* hive) {
    FILE* file = fopen(BEEHIVE_HISTORY_FILE, "a");
    if (file) {
        time_t current_time;
        time(&current_time);
        
        fprintf(file, "============= Registro de Colmena =============\n");
        fprintf(file, "Timestamp: %s", ctime(&current_time));
        fprintf(file, "ID Colmena: %d\n", hive->id);
        fprintf(file, "Huevos:\n");
        fprintf(file, "  - Total actual: %d\n", hive->egg_count);
        fprintf(file, "  - Eclosionados: %d\n", hive->hatched_eggs);
        fprintf(file, "Abejas:\n");
        fprintf(file, "  - Muertas: %d\n", hive->dead_bees);
        fprintf(file, "  - Nacidas: %d\n", hive->born_bees);
        fprintf(file, "  - Total actual: %d\n", hive->bee_count);
        fprintf(file, "Polen:\n");
        fprintf(file, "  - Total recolectado: %d\n", hive->threads.resources.total_polen_collected);
        fprintf(file, "  - Disponible para miel: %d\n", hive->threads.resources.polen_for_honey);
        fprintf(file, "Miel:\n");
        fprintf(file, "  - Producida: %d\n", hive->produced_honey);
        fprintf(file, "  - Total actual: %d\n", hive->honey_count);
        fprintf(file, "===============================================\n\n");
        fflush(file);  // Asegurar que los datos se escriban inmediatamente
        fclose(file);
    }
}