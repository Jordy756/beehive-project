#include "../include/core/file_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <json-c/json.h>
#include <pthread.h>
#include <unistd.h>     // Para access()
#include <sys/stat.h>   // Para mkdir() y struct stat
#include <sys/types.h>  // Tipos adicionales necesarios
#include <errno.h>      // Para manejo de errores

// Initialize mutex for thread-safe file operations
pthread_mutex_t pcb_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t process_table_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t history_mutex = PTHREAD_MUTEX_INITIALIZER;

// Private functions declarations
static const char* process_state_to_string(ProcessState state);
static ProcessState string_to_process_state(const char* state_str);
static json_object* read_json_file(const char* filename);
static void write_json_file(const char* filename, json_object* json);
static void ensure_directory_exists(void);
static char* get_formatted_time(time_t t);
static json_object* pcb_to_json(ProcessControlBlock* pcb);
static json_object* beehive_history_to_json(BeehiveHistory* history);

// Private function implementations
static void ensure_directory_exists(void) {
    struct stat st = {0};
    if (stat("data", &st) == -1) {
        #ifdef _WIN32
            mkdir("data");
        #else
            mkdir("data", 0700);
        #endif
    }
}

static json_object* read_json_file(const char* filename) {
    json_object* root = json_object_from_file(filename);
    if (!root) {
        root = json_object_new_array();
    } else if (!json_object_is_type(root, json_type_array)) {
        json_object_put(root);
        root = json_object_new_array();
    }
    return root;
}

static void write_json_file(const char* filename, json_object* json) {
    if (!filename || !json) return;
    
    FILE* fp = fopen(filename, "w");
    if (fp) {
        fputs(json_object_to_json_string_ext(json, JSON_C_TO_STRING_PRETTY), fp);
        fclose(fp);
    }
}

static const char* process_state_to_string(ProcessState state) {
    switch (state) {
        case RUNNING:
            return "En ejecución";
        case READY:
            return "Listo";
        case WAITING:
            return "Espera E/S";
        default:
            return "Desconocido";
    }
}

static ProcessState string_to_process_state(const char* state_str) {
    if (strcmp(state_str, "En ejecución") == 0) return RUNNING;
    if (strcmp(state_str, "Listo") == 0) return READY;
    if (strcmp(state_str, "Espera E/S") == 0) return WAITING;
    return READY; // Default state
}

static char* get_formatted_time(time_t t) {
    static char buffer[26];
    struct tm* tm_info = localtime(&t);
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    return buffer;
}

void init_file_manager(void) {
    ensure_directory_exists();
    
    if (access(PCB_FILE, F_OK) != 0) {
        json_object* array = json_object_new_array();
        write_json_file(PCB_FILE, array);
        json_object_put(array);
    }
    
    if (access(PROCESS_TABLE_FILE, F_OK) != 0) {
        json_object* obj = json_object_new_object();
        write_json_file(PROCESS_TABLE_FILE, obj);
        json_object_put(obj);
    }
    
    if (access(BEEHIVE_HISTORY_FILE, F_OK) != 0) {
        json_object* array = json_object_new_array();
        write_json_file(BEEHIVE_HISTORY_FILE, array);
        json_object_put(array);
    }
}

void init_pcb(ProcessControlBlock* pcb, int process_id) {
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

void update_pcb_state(ProcessControlBlock* pcb, ProcessState new_state) {
    if (pcb == NULL) return;
    
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
            if (new_state != WAITING) {
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

    pcb->state = new_state;
    pcb->last_state_change = current_time;
    save_pcb(pcb);
}

void save_pcb(ProcessControlBlock* pcb) {
    pthread_mutex_lock(&pcb_mutex);
    
    json_object* array = read_json_file(PCB_FILE);
    
    // Create new PCB entry
    json_object* pcb_obj = pcb_to_json(pcb);

    // Check if process already exists and update if it does
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
    
    // If process doesn't exist, add it
    if (!found) {
        json_object_array_add(array, pcb_obj);
    }
    
    write_json_file(PCB_FILE, array);
    json_object_put(array);
    
    pthread_mutex_unlock(&pcb_mutex);
}

ProcessTable* load_process_table(void) {
    pthread_mutex_lock(&process_table_mutex);
    
    ProcessTable* table = malloc(sizeof(ProcessTable));
    memset(table, 0, sizeof(ProcessTable));
    
    json_object* root = read_json_file(PROCESS_TABLE_FILE);
    
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
    
    // Actualizar contadores de procesos por estado
    if (pcb->state == READY) table->ready_processes++;
    else if (pcb->state == WAITING) table->io_waiting_processes++;
    
    save_process_table(table);
    free(table);
}

void save_beehive_history(Beehive* hive) {
    if (!hive) return;

    pthread_mutex_lock(&history_mutex);
    
    // Leer el historial existente o crear uno nuevo
    json_object* array = read_json_file(BEEHIVE_HISTORY_FILE);
    
    // Añadir nueva entrada al array de históricos
    json_object_array_add(array, beehive_history_to_json(hive));
    
    write_json_file(BEEHIVE_HISTORY_FILE, array);
    json_object_put(array);
    
    pthread_mutex_unlock(&history_mutex);
}

static json_object* beehive_history_to_json(BeehiveHistory* history) {
    json_object* obj = json_object_new_object();
    
    json_object_object_add(obj, "timestamp", json_object_new_int64(history->timestamp));
    json_object_object_add(obj, "beehive_id", json_object_new_int(history->beehive_id));
    
    // Huevos
    json_object* eggs = json_object_new_object();
    json_object_object_add(eggs, "current", json_object_new_int(history->current_eggs));
    json_object_object_add(eggs, "hatched", json_object_new_int(history->hatched_eggs));
    json_object_object_add(eggs, "laid", json_object_new_int(history->laid_eggs));
    json_object_object_add(obj, "eggs", eggs);
    
    // Abejas
    json_object* bees = json_object_new_object();
    json_object_object_add(bees, "dead", json_object_new_int(history->dead_bees));
    json_object_object_add(bees, "born", json_object_new_int(history->born_bees));
    json_object_object_add(bees, "current", json_object_new_int(history->current_bees));
    json_object_object_add(obj, "bees", bees);
    
    // Polen
    json_object* polen = json_object_new_object();
    json_object_object_add(polen, "total_collected", json_object_new_int(history->total_polen_collected));
    json_object_object_add(polen, "available", json_object_new_int(history->polen_for_honey));
    json_object_object_add(obj, "polen", polen);
    
    // Miel
    json_object* honey = json_object_new_object();
    json_object_object_add(honey, "produced", json_object_new_int(history->produced_honey));
    json_object_object_add(honey, "total", json_object_new_int(history->total_honey));
    json_object_object_add(obj, "honey", honey);
    
    return obj;
}

static json_object* pcb_to_json(ProcessControlBlock* pcb) {
    json_object* obj = json_object_new_object();
    
    json_object_object_add(obj, "process_id", json_object_new_int(pcb->process_id));
    json_object_object_add(obj, "arrival_time", json_object_new_int64(pcb->arrival_time));
    json_object_object_add(obj, "iterations", json_object_new_int(pcb->iterations));
    json_object_object_add(obj, "avg_io_wait_time", json_object_new_double(pcb->avg_io_wait_time));
    json_object_object_add(obj, "avg_ready_wait_time", json_object_new_double(pcb->avg_ready_wait_time));
    json_object_object_add(obj, "state", json_object_new_string(process_state_to_string(pcb->state)));
    json_object_object_add(obj, "total_io_waits", json_object_new_int(pcb->total_io_waits));
    json_object_object_add(obj, "total_io_wait_time", json_object_new_double(pcb->total_io_wait_time));
    json_object_object_add(obj, "total_ready_wait_time", json_object_new_double(pcb->total_ready_wait_time));
    
    return obj;
}