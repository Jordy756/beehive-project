#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include "../include/core/beehive.h"
#include "../include/core/utils.h"
#include "../include/core/file_manager.h"
#include "../include/core/scheduler.h"

bool is_egg_position(int i, int j) {
    if (i >= 2 && i <= 7) { // Filas 3-8
        if (i == 4 || i == 5) { // Filas 5-6
            return (j >= 1 && j <= 8); // Todo excepto primera y última columna
        } else {
            return (j >= 2 && j <= 7); // Columnas 3-8
        }
    }
    return false;
}

bool find_empty_cell_for_egg(Chamber* chamber, int* x, int* y) {
    for (int i = 0; i < MAX_CHAMBER_SIZE; i++) {// Recorrer todas las filas
        for (int j = 0; j < MAX_CHAMBER_SIZE; j++) {// Recorrer todas las columnas
            if (is_egg_position(i, j) && !chamber->cells[i][j].has_egg) {// Comprobar si la posición está vacía y no tiene huevo
                *x = i;// Guardar la posición
                *y = j;// Guardar la posición
                return true;// Retornar que se encontró una posición vacía
            }
        }
    }
    return false;// Retornar que no se encontró una posición vacía
}

bool find_empty_cell_for_honey(Chamber* chamber, int* x, int* y) {
    for (int i = 0; i < MAX_CHAMBER_SIZE; i++) {// Recorrer todas las filas
        for (int j = 0; j < MAX_CHAMBER_SIZE; j++) {// Recorrer todas las columnas
            if (!is_egg_position(i, j) && !chamber->cells[i][j].has_honey) {// Comprobar si la posición está vacía y no tiene miel
                *x = i;// Guardar la posición
                *y = j;// Guardar la posición
                return true;// Retornar que se encontró una posición vacía
            }
        }
    }
    return false;// Retornar que no se encontró una posición vacía
}

void init_chambers(ProcessInfo* process_info) {
    Beehive* hive = process_info->hive;// Obtener la colmena (para acceder a los recursos)
    time_t current_time = time(NULL);// Obtener la hora actual (para calcular la hora de recolección de polen)

    for (int c = 0; c < NUM_CHAMBERS; c++) {// Recorrer todas las cámaras
        Chamber* chamber = &hive->chambers[c];// Obtener la cámara actual (para inicializar las celdas)
        memset(chamber, 0, sizeof(Chamber));// Inicializar la cámara
        
        // Inicializar todas las celdas
        for (int i = 0; i < MAX_CHAMBER_SIZE; i++) {// Recorrer todas las filas
            for (int j = 0; j < MAX_CHAMBER_SIZE; j++) {// Recorrer todas las columnas
                chamber->cells[i][j].has_honey = false;// Inicializar la celda como vacía
                chamber->cells[i][j].has_egg = false;// Inicializar la celda como vacía
                chamber->cells[i][j].egg_lay_time = current_time;// Guardar la hora de la última puesta de huevos
            }
        }
    }

    // Distribuir recursos iniciales
    int honey_remaining = hive->honey_count;// Obtener la cantidad de miel restante
    int eggs_remaining = hive->egg_count;// Obtener la cantidad de huevos restante

    for (int c = 0; c < NUM_CHAMBERS && (honey_remaining > 0 || eggs_remaining > 0); c++) {// Recorrer todas las cámaras
        Chamber* chamber = &hive->chambers[c];// Obtener la cámara actual (para calcular la posición vacía)
        
        // Distribuir huevos
        while (eggs_remaining > 0 && chamber->egg_count < MAX_EGGS_PER_CHAMBER) {// Mientras haya huevos restantes y la cámara no esté llena
            int x, y;// Posición vacía
            if (find_empty_cell_for_egg(chamber, &x, &y)) {// Comprobar si hay una posición vacía
                chamber->cells[x][y].has_egg = true;// Marcar la celda como conteniendo huevo
                chamber->cells[x][y].egg_lay_time = current_time;// Guardar la hora de la última puesta de huevos
                chamber->egg_count++;// Incrementar el número de huevos
                eggs_remaining--;// Restar el número de huevos
            } else {// Si no hay posición vacía
                break;// Salir del bucle
            }
        }

        // Distribuir miel
        while (honey_remaining > 0 && chamber->honey_count < MAX_HONEY_PER_CHAMBER) {// Mientras haya miel restantes y la cámara no esté llena
            int x, y;// Posición vacía
            if (find_empty_cell_for_honey(chamber, &x, &y)) {// Comprobar si hay una posición vacía
                chamber->cells[x][y].has_honey = true;// Marcar la celda como conteniendo miel
                chamber->honey_count++;// Incrementar el número de miel
                honey_remaining--;// Restar el número de miel
            } else {// Si no hay posición vacía
                break;// Salir del bucle
            }
        }
    }
}

void update_bees_and_honey_count(Beehive* hive) {// Actualizar el contador de abejas + miel
    hive->bees_and_honey_count = hive->bee_count + hive->honey_count;// Actualizar el contador
}

void init_beehive_process(ProcessInfo* process_info, int id) {// Inicializar el proceso de la apicultura de abejas
    // Asignar e inicializar la colmena
    process_info->hive = malloc(sizeof(Beehive));// Crear un objeto de la colmena
    Beehive* hive = process_info->hive;// Obtener el objeto de la colmena

    // Inicializar datos básicos
    hive->id = id;// Asignar el ID de la colmena
    hive->bee_count = random_range(MIN_BEES, MAX_BEES);// Generar el número de abejas
    hive->honey_count = random_range(MIN_HONEY, MAX_HONEY);// Generar el número de miel
    hive->egg_count = random_range(MIN_EGGS, MAX_EGGS);// Generar el número de huevos
    hive->hatched_eggs = 0;// Inicializar el número de huevos eclosionados
    hive->dead_bees = 0;// Inicializar el número de abejas muertas
    hive->born_bees = 0;// Inicializar el número de abejas nacidas
    hive->produced_honey = 0;// Inicializar el número de miel producido
    hive->should_terminate = 0;// Inicializar el indicador de terminación
    hive->should_create_new_hive = false;// Inicializar el indicador de creación de nueva colmena

    // Actualizar contador de abejas + miel
    update_bees_and_honey_count(hive);// Actualizar el contador de abejas + miel

    // Inicializar mutexes
    pthread_mutex_init(&hive->chamber_mutex, NULL);// Inicializar el mutex de la cámara
    pthread_mutex_init(&hive->resources.polen_mutex, NULL);// Inicializar el mutex de polen

    // Inicializar recursos
    hive->resources.total_polen = 0;// Inicializar el total de polen
    hive->resources.polen_for_honey = 0;// Inicializar el polen para miel
    hive->resources.total_polen_collected = 0;// Inicializar el total de polen recolectado

    // Inicializar abejas
    hive->bees = malloc(sizeof(Bee) * hive->bee_count);// Crear un arreglo de abejas
    int queen_index = random_range(0, hive->bee_count - 1);// Obtener la posición de la reina (para asignar el tipo de la abeja)
    time_t current_time = time(NULL);// Obtener la hora actual (para calcular la hora de recolección de polen)

    for (int i = 0; i < hive->bee_count; i++) {// Recorrer todas las abejas
        hive->bees[i].id = i;// Asignar el ID de la abeja
        hive->bees[i].type = (i == queen_index) ? QUEEN : WORKER;// Asignar el tipo de la abeja (reina o obrera)
        hive->bees[i].polen_collected = 0;// Inicializar el polen recolectado
        hive->bees[i].is_alive = true;// Inicializar la vida de la abeja
        hive->bees[i].last_collection_time = current_time;// Guardar la hora de la última recolección de polen
        hive->bees[i].last_egg_laying_time = current_time;// Guardar la hora de la última puesta de huevos
    }

    // Inicializar cámaras
    init_chambers(process_info);// Inicializar las cámaras

    // Inicializar semáforos del proceso
    init_process_semaphores(process_info);// Inicializar los semáforos del proceso

    // Asignar memoria e inicializar el PCB
    process_info->pcb = malloc(sizeof(ProcessControlBlock));// Crear un objeto del PCB

    // Crear entrada PCB en el archivo
    create_pcb_for_beehive(process_info);// Crear la entrada del PCB en el archivo

    // Iniciar el proceso
    start_process_thread(process_info);// Iniciar el hilo del proceso

    printf("\nColmena #%d creada exitosamente:\n", id);// Imprimir el mensaje de creación de colmena
    printf("├─ Población inicial: %d abejas\n", hive->bee_count);// Imprimir la población inicial
    printf("├─ Reservas de miel: %d unidades\n", hive->honey_count);// Imprimir las reservas de miel
    printf("└─ Huevos iniciales: %d\n", hive->egg_count);// Imprimir los huevos iniciales
}

void cleanup_beehive_process(ProcessInfo* process_info) {// Limpiar el proceso de la apicultura de abejas
    if (!process_info || !process_info->hive) return;// Comprobar si se proporcionó un proceso y una colmena
    
    Beehive* hive = process_info->hive;// Obtener la colmena (para liberar recursos)

    // Detener el hilo
    stop_process_thread(process_info);// Detener el hilo del proceso
    
    // Liberar recursos
    free(hive->bees);// Liberar los arreglos de abejas
    pthread_mutex_destroy(&hive->chamber_mutex);// Liberar el mutex de las cámaras
    pthread_mutex_destroy(&hive->resources.polen_mutex);// Liberar el mutex de los recursos
    
    // Liberar PCB
    free(process_info->pcb);// Liberar el PCB
    process_info->pcb = NULL;// Liberar la memoria
    
    // Liberar la colmena
    free(hive);// Liberar la colmena
    process_info->hive = NULL;// Liberar la memoria de la colmena en el PCB
}

void start_process_thread(ProcessInfo* process_info) {// Iniciar el hilo del proceso principal
    update_process_state(process_info, READY);// Actualizar el estado del proceso a READY
    pthread_create(&process_info->thread_id, NULL, process_main_thread, process_info);// Crear el hilo del proceso principal
}

void stop_process_thread(ProcessInfo* process_info) {// Detener el hilo del proceso principal (si está en ejecución)
    if (!process_info || !process_info->pcb) return;// Comprobar si se proporcionó un proceso y un PCB

    if (process_info->pcb->state != READY) {// Comprobar si el estado del PCB no es READY
        update_process_state(process_info, READY);// Actualizar el estado del PCB a READY
        pthread_join(process_info->thread_id, NULL);// Unir al hilo del proceso principal
    }
}

void* process_main_thread(void* arg) {// El hilo del proceso principal (se ejecuta en un nuevo proceso)
    ProcessInfo* process_info = (ProcessInfo*)arg;// Obtener el PCB del proceso principal
    Beehive* hive = process_info->hive;// Obtener la colmena del proceso principal (para acceder a los recursos)

    while (!hive->should_terminate) {// Mientras no se debe terminar el proceso principal
        sem_wait(process_info->shared_resource_sem);// Esperar a que se produzca una operación en el PCB
        
        if (process_info->pcb->state == RUNNING) {// Comprobar si el estado del PCB es RUNNING
            manage_honey_production(process_info);// Gestionar la producción de miel
            manage_polen_collection(process_info);// Gestionar la recolección de polen
            manage_bee_lifecycle(process_info);// Gestionar la vida de las abejas
            print_beehive_stats(process_info);// Imprimir las estadísticas de la colmena
        }

        sem_post(process_info->shared_resource_sem);// Liberar el semáforo del PCB
        delay_ms(1000);// Retrasar el programa por un número de milisegundos
    }
    return NULL;// Devolver NULL para indicar que se ha finalizado el hilo
}

void manage_honey_production(ProcessInfo* process_info) {// Gestionar la producción de miel
    Beehive* hive = process_info->hive;// Obtener la colmena del proceso principal
    pthread_mutex_lock(&hive->resources.polen_mutex);// Bloquear el mutex de los recursos

    if (hive->resources.polen_for_honey >= POLEN_TO_HONEY_RATIO) {// Comprobar si hay polen suficiente para producir miel
        int honey_to_produce = hive->resources.polen_for_honey / POLEN_TO_HONEY_RATIO;// Obtener la cantidad de miel a producir
        hive->resources.polen_for_honey %= POLEN_TO_HONEY_RATIO;// Restar el polen restante

        printf("\nColmena #%d - Iniciando producción de miel:\n", hive->id);// Imprimir el mensaje de inicio de producción de miel
        printf("├─ Polen disponible: %d unidades\n", hive->resources.polen_for_honey);// Imprimir el polen disponible
        printf("└─ Miel a producir: %d unidades\n", honey_to_produce);// Imprimir la cantidad de miel a producir

        pthread_mutex_lock(&hive->chamber_mutex);// Bloquear el mutex de las cámaras
        int honey_produced = 0;// Inicializar el número de miel producido

        for (int c = 0; c < NUM_CHAMBERS && honey_to_produce > 0; c++) {// Recorrer todas las cámaras
            Chamber* chamber = &hive->chambers[c];// Obtener la cámara actual (para calcular la posición vacía)
            if (chamber->honey_count >= MAX_HONEY_PER_CHAMBER) continue;// Comprobar si la cámara está llena

            int x, y;// Posición vacía
            while (honey_to_produce > 0 && chamber->honey_count < MAX_HONEY_PER_CHAMBER && find_empty_cell_for_honey(chamber, &x, &y)) {// Mientras haya polen y la cámara no esté llena
                chamber->cells[x][y].has_honey = true;// Marcar la celda como conteniendo miel
                chamber->honey_count++;// Incrementar el número de miel
                hive->honey_count++;// Incrementar el número de miel
                honey_to_produce--;// Restar el polen restante
                honey_produced++;// Incrementar el número de miel producido
            }
        }

        if (honey_produced > 0) {// Comprobar si se produjo algún miel
            hive->produced_honey += honey_produced;// Incrementar el total de miel producido
            update_bees_and_honey_count(hive);// Actualizar el contador de abejas + miel
            printf("\nColmena #%d - Producción completada:\n", hive->id);// Imprimir el mensaje de producción completada
            printf("├─ Miel producida: %d unidades\n", honey_produced);// Imprimir la cantidad de miel producido
            printf("└─ Total de miel en la colmena: %d/%d\n", hive->honey_count, MAX_HONEY_PER_HIVE);// Imprimir el total de miel en la colmena
        }

        pthread_mutex_unlock(&hive->chamber_mutex);// Desbloquear el mutex de las cámaras
    }
    pthread_mutex_unlock(&hive->resources.polen_mutex);// Desbloquear el mutex de los recursos
}

void manage_polen_collection(ProcessInfo* process_info) {// Gestionar la recolección de polen
    Beehive* hive = process_info->hive;// Obtener la colmena del proceso principal
    pthread_mutex_lock(&hive->chamber_mutex);// Bloquear el mutex de las cámaras
    time_t current_time = time(NULL);// Obtener la hora actual (para calcular el tiempo de recolección)
    int active_workers = 0;// Inicializar el número de abejas activas
    int total_polen_collected_this_round = 0;// Inicializar el total de polen recolectado en esta ronda

    printf("\nColmena #%d - Recolección de polen:\n", hive->id);// Imprimir el mensaje de recolección de polen

    for (int i = 0; i < hive->bee_count; i++) {// Recorrer todas las abejas
        if (hive->bees[i].type == WORKER && hive->bees[i].is_alive) {// Comprobar si la abeja es una obrera y está viva
            active_workers++;// Incrementar el número de abejas activas
            int polen = random_range(MIN_POLEN_PER_TRIP, MAX_POLEN_PER_TRIP);// Obtener el número de polen
            
            pthread_mutex_lock(&hive->resources.polen_mutex);// Bloquear el mutex de los recursos
            hive->resources.total_polen += polen;// Incrementar el total de polen
            hive->resources.polen_for_honey += polen;// Incrementar el polen disponible para miel
            hive->resources.total_polen_collected += polen;// Incrementar el total de polen recolectado
            hive->bees[i].polen_collected += polen;// Incrementar el polen recolectado de la abeja
            total_polen_collected_this_round += polen;// Incrementar el total de polen recolectado en esta ronda
            
            printf("├─ Abeja #%d: %d polen (Total: %d/%d)\n", i, polen, hive->bees[i].polen_collected, random_range(MIN_POLEN_LIFETIME, MAX_POLEN_LIFETIME));// Imprimir el mensaje de recolección de polen
            
            pthread_mutex_unlock(&hive->resources.polen_mutex);// Desbloquear el mutex de los recursos
            hive->bees[i].last_collection_time = current_time;// Guardar la hora de la última recolección de polen

            // Verificar muerte de abeja
            if (hive->bees[i].polen_collected >= random_range(MIN_POLEN_LIFETIME, MAX_POLEN_LIFETIME)) {// Comprobar si la abeja ha muerto
                handle_bee_death(process_info, i);// Manejar la muerte de la abeja
            }
        }
    }

    printf("└─ Resumen de recolección:\n");// Imprimir el resumen de recolección
    printf("    ├─ Polen recolectado: %d unidades\n", total_polen_collected_this_round);// Imprimir el total de polen recolectado en esta ronda
    printf("    └─ Polen total acumulado: %d unidades\n", hive->resources.total_polen_collected);// Imprimir el total de polen acumulado

    pthread_mutex_unlock(&hive->chamber_mutex);// Desbloquear el mutex de las cámaras
}

void manage_bee_lifecycle(ProcessInfo* process_info) {// Gestionar la vida de las abejas
    pthread_mutex_lock(&process_info->hive->chamber_mutex);// Bloquear el mutex de las cámaras
    
    // Procesar reina y puesta de huevos
    process_queen_egg_laying(process_info);// Procesar la puesta de huevos de la reina
    
    // Procesar eclosión de huevos
    process_eggs_hatching(process_info);// Procesar la eclosión de huevos
    
    pthread_mutex_unlock(&process_info->hive->chamber_mutex);// Desbloquear el mutex de las cámaras
}

void handle_bee_death(ProcessInfo* process_info, int bee_index) {// Manejar la muerte de una abeja
    Beehive* hive = process_info->hive;// Obtener la colmena del proceso principal (para acceder a los recursos y a la colmena)
    hive->bees[bee_index].is_alive = false;// Marcar la abeja como muerta
    hive->dead_bees++;// Incrementar el número de abejas muertas
    hive->bee_count--;// Restar el número de abejas
    update_bees_and_honey_count(hive);// Actualizar el contador de abejas + miel
    
    printf("\nColmena #%d - Muerte de abeja:\n", hive->id);// Imprimir el mensaje de muerte de abeja
    printf("├─ Abeja #%d (%s) ha muerto\n", bee_index, hive->bees[bee_index].type == QUEEN ? "REINA" : "OBRERA");// Imprimir el número de abeja y su tipo
    printf("├─ Polen recolectado en su vida: %d unidades\n", hive->bees[bee_index].polen_collected);// Imprimir el polen recolectado en su vida
    printf("└─ Total de abejas muertas: %d\n", hive->dead_bees);// Imprimir el total de abejas muertas
}

void create_new_bee(ProcessInfo* process_info, BeeType type) {// Crear una abeja nueva
    Beehive* hive = process_info->hive;// Obtener la colmena del proceso principal (para acceder a los recursos y a la colmena)
    if (hive->bee_count >= MAX_BEES) return;// Comprobar si se ha alcanzado el límite de abejas

    int new_bee_index = hive->bee_count++;// Obtener el índice de la abeja nueva (para asignar el tipo de la abeja)
    hive->bees = realloc(hive->bees, sizeof(Bee) * hive->bee_count);// utilizar realloc para cambiar el tamaño de la colmena
    
    Bee* new_bee = &hive->bees[new_bee_index];// Obtener la abeja nueva
    new_bee->id = new_bee_index;// Asignar el ID de la abeja
    new_bee->type = type;// Asignar el tipo de la abeja
    new_bee->polen_collected = 0;// Inicializar el polen recolectado
    new_bee->is_alive = true;// Inicializar la vida de la abeja
    new_bee->last_collection_time = time(NULL);// Guardar la hora de la última recolección de polen
    new_bee->last_egg_laying_time = time(NULL);// Guardar la hora de la última puesta de huevos
    
    hive->born_bees++;// Incrementar el número de abejas nacidas
    update_bees_and_honey_count(hive);// Actualizar el contador de abejas + miel
}

void process_eggs_hatching(ProcessInfo* process_info) {// Procesar la eclosión de huevos
    Beehive* hive = process_info->hive;// Obtener la colmena del proceso principal (para acceder a los recursos y a la colmena)
    time_t current_time = time(NULL);// Obtener la hora actual (para calcular el tiempo de puesta de huevos)
    int queen_count = count_queen_bees(process_info);// Obtener el número de reinas
    int eggs_hatched = 0;// Inicializar el número de huevos eclosionados

    printf("\nColmena #%d - Procesando eclosión de huevos:\n", hive->id);// Imprimir el mensaje de eclosión de huevos

    for (int c = 0; c < NUM_CHAMBERS; c++) {// Recorrer todas las cámaras
        Chamber* chamber = &hive->chambers[c];// Obtener la cámara actual (para calcular la posición vacía)
        for (int x = 0; x < MAX_CHAMBER_SIZE; x++) {// Recorrer todas las filas
            for (int y = 0; y < MAX_CHAMBER_SIZE; y++) {// Recorrer todas las columnas
                if (chamber->cells[x][y].has_egg) {// Comprobar si la celda tiene huevo
                    double elapsed_time = difftime(current_time, chamber->cells[x][y].egg_lay_time) * 1000;// Obtener el tiempo transcurrido desde la última puesta de huevos
                    if (elapsed_time >= MAX_EGG_HATCH_TIME) {// Comprobar si el tiempo transcurrido es superior al límite de tiempo de eclosión
                        chamber->cells[x][y].has_egg = false;// Marcar la celda como vacía
                        chamber->egg_count--;// Restar el número de huevos
                        hive->egg_count--;// Restar el número de huevos
                        hive->hatched_eggs++;// Incrementar el número de huevos eclosionados
                        eggs_hatched++;// Incrementar el número de huevos eclosionados

                        if (hive->bee_count < MAX_BEES) {// Comprobar si se ha alcanzado el límite de abejas
                            bool will_be_queen = (random_range(1, 100) <= QUEEN_BIRTH_PROBABILITY);// Comprobar si se va a nacer una reina
                            if (will_be_queen && queen_count == 1) {// Comprobar si hay una reina
                                hive->should_create_new_hive = true;// Indicar que se debe crear una nueva colmena
                                printf("├─ ¡Nueva reina nacerá! Se creará una nueva colmena\n");// Imprimir el mensaje de nacimiento de reina
                            } else {// Si no es una reina
                                create_new_bee(process_info, WORKER);// Crear una abeja nueva
                            }
                        }
                    }
                }
            }
        }
    }

    if (eggs_hatched > 0) {// Comprobar si se produjo algún huevo eclosionado
        printf("└─ Resumen de eclosiones:\n");// Imprimir el resumen de eclosiones
        printf("    ├─ Huevos eclosionados en este ciclo: %d\n", eggs_hatched);// Imprimir el número de huevos eclosionados en este ciclo
        printf("    ├─ Total de huevos eclosionados: %d\n", hive->hatched_eggs);// Imprimir el total de huevos eclosionados
        printf("    └─ Huevos restantes: %d\n", hive->egg_count);// Imprimir el número de huevos restantes
    } else {// Si no se produjo huevo eclosionado
        printf("└─ No hay huevos listos para eclosionar\n");// Imprimir que no hay huevos listos para eclosionar
    }
}

void process_queen_egg_laying(ProcessInfo* process_info) {// Procesar la puesta de huevos de la reina
    Beehive* hive = process_info->hive;// Obtener la colmena del proceso principal (para acceder a los recursos y a la colmena)
    printf("\nColmena #%d - Actividad de la reina:\n", hive->id);// Imprimir el mensaje de puesta de huevos de la reina

    // Encontrar la reina
    for (int i = 0; i < hive->bee_count; i++) {// Recorrer todas las abejas
        if (hive->bees[i].type == QUEEN && hive->bees[i].is_alive) {// Comprobar si la abeja es una reina y está viva
            int eggs_to_lay = random_range(MIN_EGGS_PER_LAYING, MAX_EGGS_PER_LAYING);// Obtener el número de huevos a poner
            printf("├─ Reina #%d intentará poner %d huevos\n", i, eggs_to_lay);// Imprimir el mensaje de puesta de huevos de la reina
            
            int eggs_laid = 0;// Inicializar el número de huevos puestos
            // Intentar poner huevos en cámaras disponibles
            for (int c = 0; c < NUM_CHAMBERS && eggs_to_lay > 0; c++) {// Recorrer todas las cámaras
                Chamber* chamber = &hive->chambers[c];// Obtener la cámara actual (para calcular la posición vacía)
                if (chamber->egg_count >= MAX_EGGS_PER_CHAMBER ||
                    hive->egg_count >= MAX_EGGS_PER_HIVE) continue;// Comprobar si se ha alcanzado el límite de huevos

                int x, y;
                while (eggs_to_lay > 0 && chamber->egg_count < MAX_EGGS_PER_CHAMBER && find_empty_cell_for_egg(chamber, &x, &y)) {// Mientras hay huevos restantes y la cámara no esté llena
                    chamber->cells[x][y].has_egg = true;// Marcar la celda como conteniendo huevo
                    chamber->cells[x][y].egg_lay_time = time(NULL);// Guardar la hora de la última puesta de huevos
                    chamber->egg_count++;// Incrementar el número de huevos
                    hive->egg_count++;// Incrementar el número de huevos
                    eggs_to_lay--;// Restar el número de huevos
                    eggs_laid++;// Incrementar el número de huevos puestos
                }
            }
            
            printf("└─ Resultado de puesta:\n");// Imprimir el resultado de la puesta de huevos
            printf("    ├─ Huevos puestos: %d\n", eggs_laid);// Imprimir el número de huevos puestos
            printf("    ├─ Total de huevos en la colmena: %d/%d\n", hive->egg_count, MAX_EGGS_PER_HIVE);// Imprimir el total de huevos en la colmena
            printf("    └─ Capacidad restante: %d huevos\n", MAX_EGGS_PER_HIVE - hive->egg_count);// Imprimir la capacidad restante de huevos
            
            break; // Solo procesar una reina por vez
        }
    }
}

int count_queen_bees(ProcessInfo* process_info) {// Contar las abejas reinas
    Beehive* hive = process_info->hive;// Obtener la colmena del proceso principal (para acceder a los recursos y a la colmena)
    int count = 0;// Inicializar el contador
    for (int i = 0; i < hive->bee_count; i++) {// Recorrer todas las abejas
        if (hive->bees[i].is_alive && hive->bees[i].type == QUEEN) {// Comprobar si la abeja es una reina y está viva
            count++;// Incrementar el contador
        }
    }
    return count;// Devolver el número de reinas
}

void print_chamber_row(ProcessInfo* process_info, int start_index, int end_index) {// Imprimir una fila de cámaras
    Beehive* hive = process_info->hive;// Obtener la colmena del proceso principal (para acceder a los recursos y a la colmena)
    
    // Imprimir números de cámara
    for (int c = start_index; c < end_index; c++) {// Recorrer todas las cámaras
        printf("Cámara #%d:", c);// Imprimir el número de cámara
        printf("\t\t\t\t\t");// Imprimir 4 espacios para alinear el texto
    }
    printf("\n");// Imprimir un salto de línea

    // Imprimir matrices de cámaras
    for (int i = 0; i < MAX_CHAMBER_SIZE; i++) {// Recorrer todas las filas
        for (int c = start_index; c < end_index; c++) {// Recorrer todas las cámaras
            Chamber* chamber = &hive->chambers[c];// Obtener la cámara actual (para calcular la posición vacía)
            for (int j = 0; j < MAX_CHAMBER_SIZE; j++) {// Recorrer todas las columnas
                Cell* cell = &chamber->cells[i][j];// Obtener la celda actual (para comprobar si tiene huevo)
                if (is_egg_position(i, j)) {// Comprobar si la posición está vacía y tiene huevo
                    printf("H%d ", cell->has_egg ? 1 : 0);// Imprimir el número de huevo (1 si tiene huevo, 0 si no)
                } else {// Si la posición está vacía y no tiene huevo
                    printf("M%d ", cell->has_honey ? 1 : 0);// Imprimir el número de miel (1 si tiene miel, 0 si no)
                }
            }
            printf("\t\t\t");// Imprimir 4 espacios para alinear el texto
        }
        printf("\n");// Imprimir un salto de línea
    }

    // Imprimir estadísticas de cámaras
    for (int c = start_index; c < end_index; c++) {// Recorrer todas las cámaras
        Chamber* chamber = &hive->chambers[c];// Obtener la cámara actual (para calcular la posición vacía)
        printf("Miel: %d/%d, Huevos: %d/%d\t\t\t", chamber->honey_count, MAX_HONEY_PER_CHAMBER, chamber->egg_count, MAX_EGGS_PER_CHAMBER);// Imprimir el número de miel y huevos
    }
    printf("\n\n");// Imprimir un salto de línea
}

void print_chamber_matrix(ProcessInfo* process_info) {// Imprimir la matriz de cámaras
    printf("\nColmena #%d - Estado de las cámaras:\n\n", process_info->hive->id);// Imprimir el mensaje de estado de las cámaras
    print_chamber_row(process_info, 0, 5);// Imprimir la fila de cámaras
    print_chamber_row(process_info, 5, 10);// Imprimir la fila de cámaras
}

void print_detailed_bee_status(ProcessInfo* process_info) {// Imprimir el estado detallado de las abejas
    Beehive* hive = process_info->hive;// Obtener la colmena del proceso principal (para acceder a los recursos y a la colmena)
    int alive_workers = 0, dead_workers = 0;// Inicializar el número de abejas vivas y muertas
    
    for (int i = 0; i < hive->bee_count; i++) {// Recorrer todas las abejas
        if (hive->bees[i].type == WORKER) {// Comprobar si la abeja es un trabajador
            if (hive->bees[i].is_alive) {// Comprobar si la abeja está viva
                alive_workers++;// Incrementar el número de abejas vivas
            } else {// Si la abeja está muerta
                dead_workers++;// Incrementar el número de abejas muertas
            }
        }
    }
    
    printf("├─ Total de abejas: %d/%d\n", alive_workers + 1, MAX_BEES);// Imprimir el número de abejas y su tipo
    printf("    ├─ Obreras vivas: %d\n", alive_workers);// Imprimir el número de abejas vivas
    printf("    ├─ Obreras muertas: %d\n", dead_workers);// Imprimir el número de abejas muertas
    printf("    ├─ Abejas nacidas: %d\n", hive->born_bees);// Imprimir el número de abejas nacidas
    printf("    └─ Total de muertes: %d\n", hive->dead_bees);// Imprimir el número de muertes
}

void print_beehive_stats(ProcessInfo* process_info) {// Imprimir las estadísticas del apiario
    Beehive* hive = process_info->hive;// Obtener la colmena del proceso principal (para acceder a los recursos y a la colmena)

    printf("\nColmena #%d - Estadísticas Generales:\n", hive->id);// Imprimir el mensaje de estadísticas generales
    print_detailed_bee_status(process_info);// Imprimir el estado detallado de las abejas
    printf("├─ Total de miel: %d/%d\n", hive->honey_count, MAX_HONEY_PER_HIVE);// Imprimir el total de miel
    printf("├─ Total de huevos: %d/%d\n", hive->egg_count, MAX_EGGS_PER_HIVE);// Imprimir el total de huevos
    printf("├─ Huevos eclosionados: %d\n", hive->hatched_eggs);// Imprimir el número de huevos eclosionados
    printf("├─ Abejas muertas: %d\n", hive->dead_bees);// Imprimir el número de abejas muertas
    printf("├─ Abejas nacidas: %d\n", hive->born_bees);// Imprimir el número de abejas nacidas
    printf("├─ Miel producida: %d\n", hive->produced_honey);// Imprimir el total de miel producido
    printf("├─ Polen total recolectado: %d\n", hive->resources.total_polen_collected);// Imprimir el total de polen recolectado
    printf("└─ Recursos para FSJ (abejas + miel): %d\n", hive->bees_and_honey_count);// Imprimir el total de abejas y miel
    print_chamber_matrix(process_info);// Imprimir la matriz de cámaras
}

bool check_new_queen(ProcessInfo* process_info) {// Comprobar si hay una reina
    Beehive* hive = process_info->hive;// Obtener la colmena del proceso principal (para acceder a los recursos y a la colmena)
    
    pthread_mutex_lock(&hive->chamber_mutex);// Bloquear el mutex de las cámaras
    bool needs_new_hive = hive->should_create_new_hive;// Comprobar si se debe crear una nueva colmena
    if (needs_new_hive) {// Si se debe crear una nueva colmena
        printf("\nColmena #%d - Nueva reina detectada: Se iniciará una nueva colmena\n", hive->id);// Imprimir el mensaje de nueva reina detectada
        hive->should_create_new_hive = false;// Liberar el mutex de las cámaras
    }
    pthread_mutex_unlock(&hive->chamber_mutex);// Desbloquear el mutex de las cámaras
    
    return needs_new_hive;// Devolver si se debe crear una nueva colmena
}