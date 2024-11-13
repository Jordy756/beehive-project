#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/core/file_manager.h"

void init_file_manager(void) {
    system("mkdir -p data");
}

void write_to_pcb_file(FILE* file, ProcessControlBlock* pcb) {
    fprintf(file, "============= PCB Colmena %d =============\n", pcb->process_id);
    fprintf(file, "ID Proceso: %d\n", pcb->process_id);
    fprintf(file, "Tiempo de llegada: %ld\n", pcb->arrival_time);
    fprintf(file, "Iteraciones totales: %d\n", pcb->iterations);
    fprintf(file, "Progreso en pila de código: %d\n", pcb->code_stack_progress);
    fprintf(file, "Tiempo de espera en E/S: %d\n", pcb->io_wait_time);
    fprintf(file, "Promedio tiempo espera E/S: %.2f\n", pcb->avg_io_wait_time);
    fprintf(file, "Promedio tiempo espera Ready: %.2f\n", pcb->avg_ready_wait_time);
    fprintf(file, "Estado actual: %s\n",
        pcb->state == READY ? "READY" :
        pcb->state == RUNNING ? "RUNNING" :
        pcb->state == WAITING ? "WAITING" : "TERMINATED");
    fprintf(file, "=========================================\n\n");
}

void save_pcb(ProcessControlBlock* pcb) {
    // Primero verificar si el proceso ya existe
    FILE* file = fopen(PCB_FILE, "r");
    bool exists = false;
    char line[1024];
    
    if (file) {
        while (fgets(line, sizeof(line), file)) {
            int id;
            if (sscanf(line, "============= PCB Colmena %d =============", &id) == 1) {
                if (id == pcb->process_id) {
                    exists = true;
                    break;
                }
            }
        }
        fclose(file);
    }
    
    if (!exists) {
        // Si no existe, simplemente agregar al final
        file = fopen(PCB_FILE, "a");
        if (file) {
            write_to_pcb_file(file, pcb);
            fflush(file);
            fclose(file);
        }
    } else {
        // Si existe, crear archivo temporal y reescribir
        FILE* temp = fopen("data/pcb_temp.txt", "w");
        file = fopen(PCB_FILE, "r");
        
        if (temp && file) {
            bool skipping = false;
            while (fgets(line, sizeof(line), file)) {
                int id;
                if (sscanf(line, "============= PCB Colmena %d =============", &id) == 1) {
                    if (id == pcb->process_id) {
                        write_to_pcb_file(temp, pcb);
                        skipping = true;
                    } else {
                        skipping = false;
                    }
                }
                
                if (!skipping) {
                    fputs(line, temp);
                }
            }
            
            fclose(file);
            fclose(temp);
            
            // Reemplazar el archivo original con el temporal
            remove(PCB_FILE);
            rename("data/pcb_temp.txt", PCB_FILE);
        } else {
            if (temp) fclose(temp);
            if (file) fclose(file);
        }
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