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

// Sincronización de hilos
pthread_mutex_t pcb_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex para el acceso a PCB
pthread_mutex_t process_table_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex para el acceso a la tabla de procesos
pthread_mutex_t history_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex para el acceso a la historia de colmenas

static const char* process_state_to_string(ProcessState state); // Convierte el estado de un proceso a una cadena legible
static json_object* pcb_to_json(ProcessControlBlock* pcb); // Convierte un bloque de control de procesos a un objeto JSON
static json_object* beehive_to_json(Beehive* hive); // Convierte una colmena a un objeto JSON

// Convierte el estado de un proceso a una cadena legible
static const char* process_state_to_string(ProcessState state) {
    switch (state) { // Convierte el estado de un proceso a una cadena legible
        case RUNNING: return "En ejecución"; // Proceso en ejecución
        case READY: return "Listo"; // Proceso listo
        case WAITING: return "Espera E/S"; // Proceso en espera
        default: return "Desconocido"; // Estado desconocido
    }
}

// Convierte un bloque de control de procesos a un objeto JSON
static json_object* pcb_to_json(ProcessControlBlock* pcb) {
    if (!pcb) return NULL; // Si no hay bloque de control de procesos, devuelve NULL
    
    json_object* obj = json_object_new_object();
    json_object_object_add(obj, "process_id", json_object_new_int(pcb->process_id)); // ID del proceso
    json_object_object_add(obj, "arrival_time", json_object_new_string(format_time(pcb->arrival_time))); // Hora de llegada
    json_object_object_add(obj, "iterations", json_object_new_int(pcb->iterations)); // Número de iteraciones
    json_object_object_add(obj, "avg_io_wait_time", json_object_new_double(pcb->avg_io_wait_time)); // Tiempo promedio en espera de E/S
    json_object_object_add(obj, "avg_ready_wait_time", json_object_new_double(pcb->avg_ready_wait_time)); // Tiempo promedio en cola de listos
    json_object_object_add(obj, "state", json_object_new_string(process_state_to_string(pcb->state))); // Estado actual del proceso
    json_object_object_add(obj, "total_io_waits", json_object_new_int(pcb->total_io_waits)); // Número total de operaciones E/S
    json_object_object_add(obj, "total_io_wait_time", json_object_new_double(pcb->total_io_wait_time)); // Tiempo total en espera de E/S
    json_object_object_add(obj, "total_ready_wait_time", json_object_new_double(pcb->total_ready_wait_time)); // Tiempo total en cola de listos
    
    return obj;
}

// Convierte una colmena a un objeto JSON
static json_object* beehive_to_json(Beehive* hive) {
    if (!hive) return NULL; // Si no hay colmena, devuelve NULL
    
    json_object* obj = json_object_new_object(); // Crea un objeto JSON
    
    // Marca de tiempo y ID
    json_object_object_add(obj, "timestamp", json_object_new_string(format_time(time(NULL)))); // Marca de tiempo
    json_object_object_add(obj, "beehive_id", json_object_new_int(hive->id)); // ID de la colmena
    
    // Huevos
    json_object* eggs = json_object_new_object(); // Crea un objeto JSON para los huevos
    json_object_object_add(eggs, "current", json_object_new_int(hive->egg_count)); // Número de huevos actuales
    json_object_object_add(eggs, "hatched", json_object_new_int(hive->hatched_eggs)); // Número de huevos nacidos
    json_object_object_add(eggs, "laid", json_object_new_int(hive->egg_count + hive->hatched_eggs)); // Número de huevos nacidos
    json_object_object_add(obj, "eggs", eggs); // Añade el objeto JSON para los huevos
    
    // Abejas
    json_object* bees = json_object_new_object(); // Crea un objeto JSON para las abejas
    json_object_object_add(bees, "dead", json_object_new_int(hive->dead_bees)); // Número de abejas muertas
    json_object_object_add(bees, "born", json_object_new_int(hive->born_bees)); // Número de abejas nacidas
    json_object_object_add(bees, "current", json_object_new_int(hive->bee_count)); // Número de abejas actuales
    json_object_object_add(obj, "bees", bees); // Añade el objeto JSON para las abejas
    
    // Polen
    json_object* polen = json_object_new_object(); // Crea un objeto JSON para el polen
    json_object_object_add(polen, "total_collected", json_object_new_int(hive->resources.total_polen_collected)); // Polen total recogido
    json_object_object_add(polen, "available", json_object_new_int(hive->resources.polen_for_honey)); // Polen disponible
    json_object_object_add(obj, "polen", polen); // Añade el objeto JSON para el polen
    
    // Miel
    json_object* honey = json_object_new_object(); // Crea un objeto JSON para el miel
    json_object_object_add(honey, "produced", json_object_new_int(hive->produced_honey)); // Miel producido
    json_object_object_add(honey, "total", json_object_new_int(hive->honey_count)); // Miel total
    json_object_object_add(obj, "honey", honey); // Añade el objeto JSON para el miel
    
    return obj;
}

// Inicialización del sistema de manejo de archivos
void init_file_manager(void) {
    if (!directory_exists("data")) { // Si no existe el directorio data, lo crea
        create_directory("data"); // Crea el directorio data
    }
    
    if (!file_exists(PCB_FILE)) { // Si no existe el archivo PCB, lo crea
        json_object* array = json_object_new_array(); // Crea un array de objetos JSON
        write_json_file(PCB_FILE, array); // Escribe el archivo PCB
        json_object_put(array); // Libera el array de objetos JSON
    }
    
    if (!file_exists(PROCESS_TABLE_FILE)) { // Si no existe el archivo de tabla de procesos, lo crea
        json_object* obj = json_object_new_object(); // Crea un objeto JSON
        write_json_file(PROCESS_TABLE_FILE, obj); // Escribe el archivo de tabla de procesos
        json_object_put(obj); // Libera el objeto JSON
    }
    
    if (!file_exists(BEEHIVE_HISTORY_FILE)) { // Si no existe el archivo de historial de colmenas, lo crea
        json_object* array = json_object_new_array(); // Crea un array de objetos JSON
        write_json_file(BEEHIVE_HISTORY_FILE, array); // Escribe el archivo de historial de colmenas
        json_object_put(array); // Libera el array de objetos JSON
    }
}

//Inicialización de la Tabla de procesos
void init_process_table(ProcessTable* table) {
    if (!table) return; // Si no hay tabla de procesos, devuelve
    
    table->avg_arrival_time = 0.0; // Tiempo promedio de llegada a cola
    table->avg_iterations = 0.0; // Tiempo promedio de iteraciones
    table->avg_io_wait_time = 0.0; // Tiempo promedio en espera de E/S
    table->avg_ready_wait_time = 0.0; // Tiempo promedio en cola de listos
    table->total_processes = INITIAL_BEEHIVES; // Número total de procesos
    table->ready_processes = INITIAL_BEEHIVES - 1; // Número de procesos listos
    table->io_waiting_processes = 0; // Número de procesos en espera de E/S
    
    save_process_table(table); // Guarda la tabla de procesos
}

//Inicialización del bloque de control de procesos
void init_pcb(ProcessControlBlock* pcb, int process_id) {
    if (!pcb) return; // Si no hay bloque de control de procesos, devuelve
    
    pcb->process_id = process_id; // ID del proceso
    pcb->arrival_time = time(NULL); // Hora de llegada
    pcb->iterations = 0; // Número de iteraciones
    pcb->avg_io_wait_time = 0.0; // Tiempo promedio en espera de E/S
    pcb->avg_ready_wait_time = 0.0; // Tiempo promedio en cola de listos
    pcb->state = READY; // Estado actual del proceso
    pcb->total_io_waits = 0; // Número total de operaciones E/S
    pcb->total_io_wait_time = 0.0; // Tiempo total en espera de E/S
    pcb->total_ready_wait_time = 0.0; // Tiempo total en cola de listos
    pcb->last_ready_time = time(NULL); // Hora de última vez que entró en cola de listos
    pcb->last_state_change = time(NULL); // Hora de última vez que cambió de estado
}

// Crea un bloque de control de procesos (PCB) para una colmena específica
void create_pcb_for_beehive(ProcessInfo* process_info) {
    if (!process_info || !process_info->hive || !process_info->pcb) return; // Si no hay bloque de control de procesos, devuelve
    
    pthread_mutex_lock(&pcb_mutex); // Bloquea el mutex para el acceso a PCB
    init_pcb(process_info->pcb, process_info->hive->id); // Inicializa el bloque de control de procesos
    
    // Lee el archivo de PCB
    json_object* array = read_json_array_file(PCB_FILE);
    
    // Convierte el PCB a JSON y lo añade al array
    json_object* pcb_obj = pcb_to_json(process_info->pcb);
    if (pcb_obj) { // Si el objeto JSON no es NULL, lo añade al array
        json_object_array_add(array, pcb_obj); // Añade el objeto JSON al array
    }
    
    // Guarda el archivo actualizado
    write_json_file(PCB_FILE, array);
    json_object_put(array); // Libera el array de objetos JSON
    
    pthread_mutex_unlock(&pcb_mutex); // Desbloquea el mutex para el acceso a PCB
}

// Actualiza el estado del proceso
void update_pcb_state(ProcessControlBlock* pcb, ProcessState new_state, Beehive* hive) {
    if (!pcb || !hive) return; // Si no hay bloque de control de procesos o colmena, devuelve
    
    time_t current_time = time(NULL); // Obtiene la hora actual
    double elapsed_time = difftime(current_time, pcb->last_state_change); // Tiempo transcurrido desde la última vez que cambió de estado
    
    switch (pcb->state) { // Convierte el estado actual del proceso a una cadena legible
        case READY: // Proceso listo
            if (new_state == RUNNING) { // Proceso en ejecución
                pcb->iterations++; // Incrementa el número de iteraciones
            }
            pcb->total_ready_wait_time += elapsed_time; // Suma el tiempo transcurrido en cola de listos
            if (pcb->iterations > 0) { // Si hay iteraciones
                pcb->avg_ready_wait_time = pcb->total_ready_wait_time / pcb->iterations; // Calcula el tiempo promedio en cola de listos
            }
            break;
            
        case WAITING: // Proceso en espera
            if (new_state == READY) { // Proceso listo
                pcb->total_io_wait_time += (pcb->current_io_wait_time / 1000.0); // Suma el tiempo transcurrido en espera de E/S
                pcb->avg_io_wait_time = pcb->total_io_wait_time / pcb->total_io_waits; // Calcula el tiempo promedio en espera de E/S
            }
            break;
            
        case RUNNING: // Proceso en ejecución
            break;
    }
    
    if (new_state == WAITING && pcb->state != WAITING) { // Si el nuevo estado es WAITING y el actual no lo es
        pcb->total_io_waits++; // Incrementa el número de operaciones E/S
    }
    
    if ((pcb->state == WAITING && new_state == READY) || (pcb->state == RUNNING && new_state == READY)) { // Si el estado actual es WAITING y el nuevo es READY o el estado actual es RUNNING y el nuevo es READY
        pcb->state = new_state; // Actualiza el estado del proceso
        save_beehive_history(hive); // Guarda el historial de colmenas
        save_pcb(pcb); // Guarda el bloque de control de procesos
    } else {
        pcb->state = new_state; // Actualiza el estado del proceso
    }
    
    pcb->last_state_change = current_time; // Actualiza la hora de última vez que cambió de estado
}

// Guarda el bloque de control de procesos en el archivo correspondiente
void save_pcb(ProcessControlBlock* pcb) {
    if (!pcb) return; // Si no hay bloque de control de procesos, devuelve
    
    pthread_mutex_lock(&pcb_mutex); // Bloquea el mutex para el acceso a PCB
    json_object* array = read_json_array_file(PCB_FILE); // Lee el archivo de PCB
    json_object* pcb_obj = pcb_to_json(pcb); // Convierte el bloque de control de procesos a JSON
    
    // Busca y actualiza el bloque de control de procesos existente
    bool found = false;
    for (size_t i = 0; i < json_object_array_length(array); i++) { // Recorre el array de bloques de control de procesos
        json_object* existing_pcb = json_object_array_get_idx(array, i); // Obtiene el bloque de control de procesos existente
        json_object* existing_id; // Obtiene el ID del bloque de control de procesos existente
        
        if (json_object_object_get_ex(existing_pcb, "process_id", &existing_id) && 
            json_object_get_int(existing_id) == pcb->process_id) { // Si el ID del bloque de control de procesos es el mismo
            json_object_array_put_idx(array, i, pcb_obj); // Actualiza el bloque de control de procesos
            found = true; // Indica que se ha encontrado el bloque de control de procesos
            break;
        }
    }
    
    // Si no existe, añade uno nuevo
    if (!found) {
        json_object_array_add(array, pcb_obj); // Añade el bloque de control de procesos a la lista
    }
    
    write_json_file(PCB_FILE, array); // Escribe el archivo de PCB actualizado
    json_object_put(array); // Libera el array de objetos JSON
    pthread_mutex_unlock(&pcb_mutex); // Desbloquea el mutex para el acceso a PCB
}

// Guarda el historial de colmenas en el archivo correspondiente
void save_beehive_history(Beehive* hive) {
    if (!hive) return; // Si no hay colmena, devuelve
    
    pthread_mutex_lock(&history_mutex); // Bloquea el mutex para el acceso a la historia de colmenas
    json_object* array = read_json_array_file(BEEHIVE_HISTORY_FILE); // Lee el archivo de historial de colmenas
    json_object* history = beehive_to_json(hive); // Convierte la colmena a JSON
    
    if (history != NULL) { // Si el objeto JSON no es NULL, lo añade al array
        json_object_array_add(array, history); // Añade el objeto JSON al array
        write_json_file(BEEHIVE_HISTORY_FILE, array); // Escribe el archivo de historial de colmenas actualizado
    }
    
    json_object_put(array); // Libera el array de objetos JSON
    pthread_mutex_unlock(&history_mutex); // Desbloquea el mutex para el acceso a la historia de colmenas
}

// Guarda la tabla de procesos en el archivo correspondiente
void save_process_table(ProcessTable* table) {
    if (!table) return; // Si no hay tabla de procesos, devuelve
    
    pthread_mutex_lock(&process_table_mutex); // Bloquea el mutex para el acceso a la tabla de procesos
    json_object* obj = json_object_new_object(); // Crea un objeto JSON
    
    json_object_object_add(obj, "avg_arrival_time", json_object_new_double(table->avg_arrival_time)); // Tiempo promedio de llegada a cola
    json_object_object_add(obj, "avg_iterations", json_object_new_double(table->avg_iterations)); // Tiempo promedio de iteraciones
    json_object_object_add(obj, "avg_io_wait_time", json_object_new_double(table->avg_io_wait_time)); // Tiempo promedio en espera de E/S
    json_object_object_add(obj, "avg_ready_wait_time", json_object_new_double(table->avg_ready_wait_time)); // Tiempo promedio en cola de listos
    json_object_object_add(obj, "total_processes", json_object_new_int(table->total_processes)); // Número total de procesos
    json_object_object_add(obj, "ready_processes", json_object_new_int(table->ready_processes)); // Número de procesos listos
    json_object_object_add(obj, "io_waiting_processes", json_object_new_int(table->io_waiting_processes)); // Número de procesos en espera de E/S
    
    write_json_file(PROCESS_TABLE_FILE, obj); // Escribe el archivo de tabla de procesos actualizado
    json_object_put(obj); // Libera el objeto JSON
    pthread_mutex_unlock(&process_table_mutex); // Desbloquea el mutex para el acceso a la tabla de procesos
}

// Actualiza las estadísticas de la tabla de procesos 
void update_process_table(ProcessControlBlock* pcb) {
    if (!pcb) return; // Si no hay bloque de control de procesos, devuelve
    
    ProcessTable* table = scheduler_state.process_table; // Obtiene la tabla de procesos
    
    double old_weight = (double)(table->total_processes) / (table->total_processes + 1); // Tiempo promedio de llegada a cola
    double new_weight = 1.0 / (table->total_processes + 1); // Tiempo promedio de iteraciones

    table->avg_arrival_time = (table->avg_arrival_time * old_weight) + (difftime(time(NULL), pcb->arrival_time) * new_weight); // Tiempo promedio de llegada a cola
    table->avg_iterations = (table->avg_iterations * old_weight) + (pcb->iterations * new_weight); // Tiempo promedio de iteraciones
    table->avg_io_wait_time = (table->avg_io_wait_time * old_weight) + (pcb->avg_io_wait_time * new_weight); // Tiempo promedio en espera de E/S
    table->avg_ready_wait_time = (table->avg_ready_wait_time * old_weight) + (pcb->avg_ready_wait_time * new_weight); // Tiempo promedio en cola de listos
    
    save_process_table(table); // Guarda la tabla de procesos
}