#include "../include/core/file_manager.h"
#include "../include/core/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <json-c/json.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

// Initialize mutex for thread-safe file operations
pthread_mutex_t pcb_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t process_table_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t history_mutex = PTHREAD_MUTEX_INITIALIZER;

// Private functions declarations
static const char* process_state_to_string(ProcessState state);
static json_object* pcb_to_json(ProcessControlBlock* pcb);
static json_object* beehive_to_json(Beehive* hive);

// Private function implementations
static const char* process_state_to_string(ProcessState state) {
    switch (state) {
        case RUNNING: return "En ejecución";
        case READY: return "Listo";
        case WAITING: return "Espera E/S";
        default: return "Desconocido";
    }
}

static json_object* pcb_to_json(ProcessControlBlock* pcb) {
    if (!pcb) return NULL;
    
    json_object* obj = json_object_new_object();
    json_object_object_add(obj, "process_id", json_object_new_int(pcb->process_id));
    json_object_object_add(obj, "arrival_time", json_object_new_string(format_time(pcb->arrival_time)));
    json_object_object_add(obj, "iterations", json_object_new_int(pcb->iterations));
    json_object_object_add(obj, "avg_io_wait_time", json_object_new_double(pcb->avg_io_wait_time));
    json_object_object_add(obj, "avg_ready_wait_time", json_object_new_double(pcb->avg_ready_wait_time));
    json_object_object_add(obj, "state", json_object_new_string(process_state_to_string(pcb->state)));
    json_object_object_add(obj, "total_io_waits", json_object_new_int(pcb->total_io_waits));
    json_object_object_add(obj, "total_io_wait_time", json_object_new_double(pcb->total_io_wait_time));
    json_object_object_add(obj, "total_ready_wait_time", json_object_new_double(pcb->total_ready_wait_time));
    
    return obj;
}

static json_object* beehive_to_json(Beehive* hive) {
    if (!hive) return NULL;
    
    json_object* obj = json_object_new_object();
    
    // Timestamp y ID
    json_object_object_add(obj, "timestamp", json_object_new_string(format_time(time(NULL))));
    json_object_object_add(obj, "beehive_id", json_object_new_int(hive->id));
    
    // Huevos
    json_object* eggs = json_object_new_object();
    json_object_object_add(eggs, "current", json_object_new_int(hive->egg_count));
    json_object_object_add(eggs, "hatched", json_object_new_int(hive->hatched_eggs));
    json_object_object_add(eggs, "laid", json_object_new_int(hive->egg_count + hive->hatched_eggs));
    json_object_object_add(obj, "eggs", eggs);
    
    // Abejas
    json_object* bees = json_object_new_object();
    json_object_object_add(bees, "dead", json_object_new_int(hive->dead_bees));
    json_object_object_add(bees, "born", json_object_new_int(hive->born_bees));
    json_object_object_add(bees, "current", json_object_new_int(hive->bee_count));
    json_object_object_add(obj, "bees", bees);
    
    // Polen
    json_object* polen = json_object_new_object();
    json_object_object_add(polen, "total_collected", json_object_new_int(hive->resources.total_polen_collected));
    json_object_object_add(polen, "available", json_object_new_int(hive->resources.polen_for_honey));
    json_object_object_add(obj, "polen", polen);
    
    // Miel
    json_object* honey = json_object_new_object();
    json_object_object_add(honey, "produced", json_object_new_int(hive->produced_honey));
    json_object_object_add(honey, "total", json_object_new_int(hive->honey_count));
    json_object_object_add(obj, "honey", honey);
    
    return obj;
}

// Public functions implementations
void init_file_manager(void) {
    if (!directory_exists("data")) {
        create_directory("data");
    }
    
    if (!file_exists(PCB_FILE)) {
        json_object* array = json_object_new_array();
        write_json_file(PCB_FILE, array);
        json_object_put(array);
    }
    
    if (!file_exists(PROCESS_TABLE_FILE)) {
        json_object* obj = json_object_new_object();
        write_json_file(PROCESS_TABLE_FILE, obj);
        json_object_put(obj);
    }
    
    if (!file_exists(BEEHIVE_HISTORY_FILE)) {
        json_object* array = json_object_new_array();
        write_json_file(BEEHIVE_HISTORY_FILE, array);
        json_object_put(array);
    }
}

void init_pcb(ProcessControlBlock* pcb, int process_id) {
    if (!pcb) return;
    
    pcb->process_id = process_id;
    pcb->arrival_time = time(NULL);
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

void create_pcb_for_beehive(ProcessInfo* process_info) {
    if (!process_info || !process_info->hive || !process_info->pcb) return;
    
    pthread_mutex_lock(&pcb_mutex);
    
    init_pcb(process_info->pcb, process_info->hive->id);
    
    // Leer el archivo de PCBs
    json_object* array = read_json_array_file(PCB_FILE);
    
    // Convertir el PCB a JSON y añadirlo al array
    json_object* pcb_obj = pcb_to_json(process_info->pcb);
    if (pcb_obj) {
        json_object_array_add(array, pcb_obj);
    }
    
    // Guardar el archivo actualizado
    write_json_file(PCB_FILE, array);
    json_object_put(array);
    
    pthread_mutex_unlock(&pcb_mutex);
}

void update_pcb_state(ProcessControlBlock* pcb, ProcessState new_state, Beehive* hive) {
    if (!pcb || !hive) return;
    
    time_t current_time = time(NULL);
    double elapsed_time = difftime(current_time, pcb->last_state_change);

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
            if (new_state == READY) {
                pcb->total_io_wait_time += (pcb->current_io_wait_time / 1000.0);
                pcb->avg_io_wait_time = pcb->total_io_wait_time / pcb->total_io_waits;
            }
            break;
            
        case RUNNING:
            break;
    }

    if (new_state == WAITING && pcb->state != WAITING) {
        pcb->total_io_waits++;
    }

    if((pcb->state == WAITING && new_state == READY) || (pcb->state == RUNNING && new_state == READY)){
        pcb->state = new_state;
        save_beehive_history(hive);
        save_pcb(pcb);
    } else {
        pcb->state = new_state;
    }
    
    pcb->last_state_change = current_time;
}

void save_pcb(ProcessControlBlock* pcb) {
    if (!pcb) return;
    
    pthread_mutex_lock(&pcb_mutex);
    
    json_object* array = read_json_array_file(PCB_FILE);
    json_object* pcb_obj = pcb_to_json(pcb);
    
    // Buscar y actualizar PCB existente
    bool found = false;
    for (size_t i = 0; i < json_object_array_length(array); i++) {
        json_object* existing_pcb = json_object_array_get_idx(array, i);
        json_object* existing_id;
        if (json_object_object_get_ex(existing_pcb, "process_id", &existing_id) &&
            json_object_get_int(existing_id) == pcb->process_id) {
            json_object_array_put_idx(array, i, pcb_obj);
            found = true;
            break;
        }
    }
    
    // Si no existe, añadir nuevo
    if (!found) {
        json_object_array_add(array, pcb_obj);
    }
    
    write_json_file(PCB_FILE, array);
    json_object_put(array);
    
    pthread_mutex_unlock(&pcb_mutex);
}

void save_beehive_history(Beehive* hive) {
    if (!hive) return;
    
    pthread_mutex_lock(&history_mutex);
    
    json_object* array = read_json_array_file(BEEHIVE_HISTORY_FILE);
    json_object* history = beehive_to_json(hive);
    
    if (history != NULL) {
        json_object_array_add(array, history);
        write_json_file(BEEHIVE_HISTORY_FILE, array);
    }
    
    json_object_put(array);
    
    pthread_mutex_unlock(&history_mutex);
}

ProcessTable* load_process_table(void) {
    pthread_mutex_lock(&process_table_mutex);
    
    ProcessTable* table = malloc(sizeof(ProcessTable));
    memset(table, 0, sizeof(ProcessTable));
    
    json_object* root = read_json_array_file(PROCESS_TABLE_FILE);
    json_object* temp;
    
    if (json_object_object_get_ex(root, "avg_arrival_time", &temp))
        table->avg_arrival_time = json_object_get_double(temp);
    
    if (json_object_object_get_ex(root, "avg_iterations", &temp))
        table->avg_iterations = json_object_get_double(temp);
    
    if (json_object_object_get_ex(root, "avg_io_wait_time", &temp))
        table->avg_io_wait_time = json_object_get_double(temp);
    
    if (json_object_object_get_ex(root, "avg_ready_wait_time", &temp))
        table->avg_ready_wait_time = json_object_get_double(temp);
    
    if (json_object_object_get_ex(root, "total_processes", &temp))
        table->total_processes = json_object_get_int(temp);
    
    if (json_object_object_get_ex(root, "ready_processes", &temp))
        table->ready_processes = json_object_get_int(temp);
    
    if (json_object_object_get_ex(root, "io_waiting_processes", &temp))
        table->io_waiting_processes = json_object_get_int(temp);
    
    json_object_put(root);
    
    pthread_mutex_unlock(&process_table_mutex);
    return table;
}

void save_process_table(ProcessTable* table) {
    if (!table) return;
    
    pthread_mutex_lock(&process_table_mutex);
    
    json_object* obj = json_object_new_object();
    json_object_object_add(obj, "avg_arrival_time", json_object_new_double(table->avg_arrival_time));
    json_object_object_add(obj, "avg_iterations", json_object_new_double(table->avg_iterations));
    json_object_object_add(obj, "avg_io_wait_time", json_object_new_double(table->avg_io_wait_time));
    json_object_object_add(obj, "avg_ready_wait_time", json_object_new_double(table->avg_ready_wait_time));
    json_object_object_add(obj, "total_processes", json_object_new_int(table->total_processes));
    json_object_object_add(obj, "ready_processes", json_object_new_int(table->ready_processes));
    json_object_object_add(obj, "io_waiting_processes", json_object_new_int(table->io_waiting_processes));
    
    write_json_file(PROCESS_TABLE_FILE, obj);
    json_object_put(obj);
    
    pthread_mutex_unlock(&process_table_mutex);
}

void update_process_table(ProcessControlBlock* pcb) {
    if (!pcb) return;
    
    ProcessTable* table = load_process_table();
    
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
    
    if (pcb->state == READY) {
        table->ready_processes++;
    } else if (pcb->state == WAITING) {
        table->io_waiting_processes++;
    }
    
    save_process_table(table);
    free(table);
}