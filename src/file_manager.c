#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/core/file_manager.h"

void init_file_manager(void) {
    system("mkdir -p data");
}

void init_pcb(ProcessControlBlock* pcb, int process_id) {
    pcb->process_id = process_id;
    pcb->arrival_time = time(NULL);  // Solo se establece UNA VEZ al crear el PCB
    pcb->iterations = 0;
    pcb->avg_io_wait_time = 0.0;
    pcb->avg_ready_wait_time = 0.0;
    pcb->state = READY;
    pcb->total_io_waits = 0;
    pcb->total_io_wait_time = 0.0;
    pcb->total_ready_wait_time = 0.0;
    pcb->last_ready_time = time(NULL);
    pcb->last_state_change = time(NULL);
}

void update_pcb_state(ProcessControlBlock* pcb, ProcessState new_state) {
    if (pcb == NULL) return;
    
    time_t current_time = time(NULL);
    double elapsed_time = difftime(current_time, pcb->last_state_change);

    // Imprimir el estado actual del proceso.
    printf("\n\n=========================================\n");
    printf("[Proceso %d] Cambio de estado: %s -> %s\n", pcb->process_id,
            pcb->state == READY ? "READY" :
            pcb->state == RUNNING ? "RUNNING" : "WAITING",
            new_state == READY ? "READY" :
            new_state == RUNNING ? "RUNNING" : "WAITING");

    // Actualizar estadísticas basadas en el estado anterior
    switch (pcb->state) {
        case READY:
            if (new_state == RUNNING) {
                pcb->iterations++;
            }
            pcb->total_ready_wait_time += elapsed_time;
            if (pcb->iterations > 0) {
                pcb->avg_ready_wait_time = pcb->total_ready_wait_time / pcb->iterations;
            }
            break;
        case WAITING:
            // Cuando sale de WAITING, actualizar el tiempo total y promedio de E/S
            if (new_state != WAITING) {
                pcb->total_io_wait_time += (pcb->current_io_wait_time / 1000.0); // ms a segundos
                pcb->avg_io_wait_time = pcb->total_io_wait_time / pcb->total_io_waits;
            }
            break;
        case RUNNING:
            break;
    }

    // Solo incrementar contador de E/S cuando entra a WAITING
    if (new_state == WAITING && pcb->state != WAITING) {
        pcb->total_io_waits++;
    }

    // Actualizar estado y timestamp
    pcb->state = new_state;
    pcb->last_state_change = current_time;

    // Imprimir estadísticas actualizadas
    printf("[Proceso %d] Iteraciones: %d\n", pcb->process_id, pcb->iterations);
    printf("[Proceso %d] Promedio tiempo espera E/S: %.2f segundos\n", pcb->process_id, pcb->avg_io_wait_time);
    printf("[Proceso %d] Promedio tiempo espera Ready: %.2f segundos\n", pcb->process_id, pcb->avg_ready_wait_time);
    printf("[Proceso %d] Total de tiempo en E/S: %.2f segundos\n", pcb->process_id, pcb->total_io_wait_time);
    printf("[Proceso %d] Total de tiempo en Ready: %.2f segundos\n", pcb->process_id, pcb->total_ready_wait_time);
    printf("[Proceso %d] Estado actual: %s\n", pcb->process_id, pcb->state == READY ? "READY" : pcb->state == RUNNING ? "RUNNING" : "WAITING");
    printf("[Proceso %d] Total E/S realizadas: %d\n", pcb->process_id, pcb->total_io_waits);
    printf("=========================================\n\n");
}

void write_to_pcb_file(FILE* file, ProcessControlBlock* pcb) {
    if (!file || !pcb) return;

    char arrival_time_str[100];
    strftime(arrival_time_str, sizeof(arrival_time_str), "%Y-%m-%d %H:%M:%S", localtime(&pcb->arrival_time));
    
    fprintf(file, "============= PCB Colmena %d =============\n", pcb->process_id);
    fprintf(file, "ID Proceso: %d\n", pcb->process_id);
    fprintf(file, "Tiempo de llegada: %s\n", arrival_time_str);
    fprintf(file, "Iteraciones totales: %d\n", pcb->iterations);
    fprintf(file, "Promedio tiempo espera E/S: %.2f segundos\n", pcb->avg_io_wait_time);
    fprintf(file, "Promedio tiempo espera Ready: %.2f segundos\n", pcb->avg_ready_wait_time);
    fprintf(file, "Estado actual: %s\n", pcb->state == READY ? "READY" : pcb->state == RUNNING ? "RUNNING" : "WAITING");
    fprintf(file, "Total E/S realizadas: %d\n", pcb->total_io_waits);  // Agregado para debug
    fprintf(file, "=========================================\n\n");
}

void save_pcb(ProcessControlBlock* pcb) {
    // Primero verificar si el proceso ya existe
    FILE* file = fopen(PCB_FILE, "r");
    bool exists = false;
    char line[1024];
    char temp_file[1024];

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
        sprintf(temp_file, "%s.tmp", PCB_FILE);
        FILE* temp = fopen(temp_file, "w");
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
            rename(temp_file, PCB_FILE);
        } else {
            if (temp) fclose(temp);
            if (file) fclose(file);
        }
    }
}

void save_process_table(ProcessTable* table) {
    FILE* file = fopen(PROCESS_TABLE_FILE, "w");
    if (file) {
        fprintf(file, "Promedio tiempo de llegada: %.2f segundos\n", table->avg_arrival_time);
        fprintf(file, "Promedio iteraciones: %.2f\n", table->avg_iterations);
        fprintf(file, "Promedio tiempo de espera en E/S: %.2f segundos\n", table->avg_io_wait_time);
        fprintf(file, "Promedio tiempo de espera Ready: %.2f segundos\n", table->avg_ready_wait_time);
        fprintf(file, "Total de procesos: %d\n", table->total_processes);
        fflush(file);
        fclose(file);
    }
}

ProcessTable* load_process_table(void) {
    ProcessTable* table = malloc(sizeof(ProcessTable));
    memset(table, 0, sizeof(ProcessTable));  // Inicializar todo a 0

    FILE* file = fopen(PROCESS_TABLE_FILE, "r");
    if (file) {
        char line[256];
        while (fgets(line, sizeof(line), file)) {
            double value;
            int total_proc;  // Nueva variable para el total de procesos
            if (sscanf(line, "Promedio tiempo de llegada: %lf", &value) == 1)
                table->avg_arrival_time = value;
            else if (sscanf(line, "Promedio iteraciones: %lf", &value) == 1)
                table->avg_iterations = value;
            else if (sscanf(line, "Promedio tiempo de espera en E/S: %lf", &value) == 1)
                table->avg_io_wait_time = value;
            else if (sscanf(line, "Promedio tiempo de espera Ready: %lf", &value) == 1)
                table->avg_ready_wait_time = value;
            else if (sscanf(line, "Total de procesos: %d", &total_proc) == 1)
                table->total_processes = total_proc;
        }
        fclose(file);
    }

    return table;
}

void update_process_table(ProcessControlBlock* pcb) {
    ProcessTable* table = load_process_table();

    // Calcular nuevos promedios
    double old_weight = (double)(table->total_processes) / (table->total_processes + 1);
    double new_weight = 1.0 / (table->total_processes + 1);

    table->avg_arrival_time = (table->avg_arrival_time * old_weight) + 
                             (difftime(time(NULL), pcb->arrival_time) * new_weight);
    table->avg_iterations = (table->avg_iterations * old_weight) + 
                           (pcb->iterations * new_weight);
    table->avg_io_wait_time = (table->avg_io_wait_time * old_weight) + 
                             (pcb->avg_io_wait_time * new_weight);
    table->avg_ready_wait_time = (table->avg_ready_wait_time * old_weight) + 
                                (pcb->avg_ready_wait_time * new_weight);
    table->total_processes++;

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
        fflush(file);
        fclose(file);
    }
}