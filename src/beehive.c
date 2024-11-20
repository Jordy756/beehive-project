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
    for (int i = 0; i < MAX_CHAMBER_SIZE; i++) {
        for (int j = 0; j < MAX_CHAMBER_SIZE; j++) {
            if (is_egg_position(i, j) && !chamber->cells[i][j].has_egg) {
                *x = i;
                *y = j;
                return true;
            }
        }
    }
    return false;
}

bool find_empty_cell_for_honey(Chamber* chamber, int* x, int* y) {
    for (int i = 0; i < MAX_CHAMBER_SIZE; i++) {
        for (int j = 0; j < MAX_CHAMBER_SIZE; j++) {
            if (!is_egg_position(i, j) && !chamber->cells[i][j].has_honey) {
                *x = i;
                *y = j;
                return true;
            }
        }
    }
    return false;
}

void init_chambers(ProcessInfo* process_info) {
    Beehive* hive = process_info->hive;
    time_t current_time = time(NULL);

    for (int c = 0; c < NUM_CHAMBERS; c++) {
        Chamber* chamber = &hive->chambers[c];
        memset(chamber, 0, sizeof(Chamber));
        
        // Inicializar todas las celdas
        for (int i = 0; i < MAX_CHAMBER_SIZE; i++) {
            for (int j = 0; j < MAX_CHAMBER_SIZE; j++) {
                chamber->cells[i][j].has_honey = false;
                chamber->cells[i][j].has_egg = false;
                chamber->cells[i][j].egg_lay_time = current_time;
            }
        }
    }

    // Distribuir recursos iniciales
    int honey_remaining = hive->honey_count;
    int eggs_remaining = hive->egg_count;

    for (int c = 0; c < NUM_CHAMBERS && (honey_remaining > 0 || eggs_remaining > 0); c++) {
        Chamber* chamber = &hive->chambers[c];
        
        // Distribuir huevos
        while (eggs_remaining > 0 && chamber->egg_count < MAX_EGGS_PER_CHAMBER) {
            int x, y;
            if (find_empty_cell_for_egg(chamber, &x, &y)) {
                chamber->cells[x][y].has_egg = true;
                chamber->cells[x][y].egg_lay_time = current_time;
                chamber->egg_count++;
                eggs_remaining--;
            } else {
                break;
            }
        }

        // Distribuir miel
        while (honey_remaining > 0 && chamber->honey_count < MAX_HONEY_PER_CHAMBER) {
            int x, y;
            if (find_empty_cell_for_honey(chamber, &x, &y)) {
                chamber->cells[x][y].has_honey = true;
                chamber->honey_count++;
                honey_remaining--;
            } else {
                break;
            }
        }
    }
}

void update_bees_and_honey_count(Beehive* hive) {
    hive->bees_and_honey_count = hive->bee_count + hive->honey_count;
}

void init_beehive_process(ProcessInfo* process_info, int id) {
    // Asignar e inicializar la colmena
    process_info->hive = malloc(sizeof(Beehive));
    Beehive* hive = process_info->hive;

    // Inicializar datos básicos
    hive->id = id;
    hive->bee_count = random_range(MIN_BEES, MAX_BEES);
    hive->honey_count = random_range(MIN_HONEY, MAX_HONEY);
    hive->egg_count = random_range(MIN_EGGS, MAX_EGGS);
    hive->hatched_eggs = 0;
    hive->dead_bees = 0;
    hive->born_bees = 0;
    hive->produced_honey = 0;
    hive->should_terminate = 0;
    hive->should_create_new_hive = false;

    // Actualizar contador de abejas + miel
    update_bees_and_honey_count(hive);

    // Inicializar mutexes
    pthread_mutex_init(&hive->chamber_mutex, NULL);
    pthread_mutex_init(&hive->resources.polen_mutex, NULL);

    // Inicializar recursos
    hive->resources.total_polen = 0;
    hive->resources.polen_for_honey = 0;
    hive->resources.total_polen_collected = 0;

    // Inicializar abejas
    hive->bees = malloc(sizeof(Bee) * hive->bee_count);
    int queen_index = random_range(0, hive->bee_count - 1);
    time_t current_time = time(NULL);

    for (int i = 0; i < hive->bee_count; i++) {
        hive->bees[i].id = i;
        hive->bees[i].type = (i == queen_index) ? QUEEN : WORKER;
        hive->bees[i].polen_collected = 0;
        hive->bees[i].is_alive = true;
        hive->bees[i].last_collection_time = current_time;
        hive->bees[i].last_egg_laying_time = current_time;
    }

    // Inicializar cámaras
    init_chambers(process_info);

    // Inicializar semáforos del proceso
    init_process_semaphores(process_info);

    // Asignar memoria e inicializar el PCB
    process_info->pcb = malloc(sizeof(ProcessControlBlock));

    // Crear entrada PCB en el archivo
    create_pcb_for_beehive(process_info);

    // Iniciar el proceso
    start_process_thread(process_info);

    printf("\nColmena #%d creada exitosamente:\n", id);
    printf("├─ Población inicial: %d abejas\n", hive->bee_count);
    printf("├─ Reservas de miel: %d unidades\n", hive->honey_count);
    printf("└─ Huevos iniciales: %d\n", hive->egg_count);
}

void cleanup_beehive_process(ProcessInfo* process_info) {
    if (!process_info || !process_info->hive) return;
    
    Beehive* hive = process_info->hive;

    // Detener el hilo
    stop_process_thread(process_info);
    
    // Liberar recursos
    free(hive->bees);
    pthread_mutex_destroy(&hive->chamber_mutex);
    pthread_mutex_destroy(&hive->resources.polen_mutex);
    
    // Liberar PCB
    free(process_info->pcb);
    process_info->pcb = NULL;
    
    // Liberar la colmena
    free(hive);
    process_info->hive = NULL;
}

void start_process_thread(ProcessInfo* process_info) {
    update_process_state(process_info, READY);
    pthread_create(&process_info->thread_id, NULL, process_main_thread, process_info);
}

void stop_process_thread(ProcessInfo* process_info) {
    if (!process_info || !process_info->pcb) return;

    if (process_info->pcb->state != READY) {
        update_process_state(process_info, READY);
        pthread_join(process_info->thread_id, NULL);
    }
}

void* process_main_thread(void* arg) {
    ProcessInfo* process_info = (ProcessInfo*)arg;
    Beehive* hive = process_info->hive;

    while (!hive->should_terminate) {
        sem_wait(process_info->shared_resource_sem);
        
        if (process_info->pcb->state == RUNNING) {
            manage_honey_production(process_info);
            manage_polen_collection(process_info);
            manage_bee_lifecycle(process_info);
            print_beehive_stats(process_info);
        }

        sem_post(process_info->shared_resource_sem);
        delay_ms(1000);
    }
    return NULL;
}

void manage_honey_production(ProcessInfo* process_info) {
    Beehive* hive = process_info->hive;
    pthread_mutex_lock(&hive->resources.polen_mutex);

    if (hive->resources.polen_for_honey >= POLEN_TO_HONEY_RATIO) {
        int honey_to_produce = hive->resources.polen_for_honey / POLEN_TO_HONEY_RATIO;
        hive->resources.polen_for_honey %= POLEN_TO_HONEY_RATIO;

        printf("\nColmena #%d - Iniciando producción de miel:\n", hive->id);
        printf("├─ Polen disponible: %d unidades\n", hive->resources.polen_for_honey);
        printf("└─ Miel a producir: %d unidades\n", honey_to_produce);

        pthread_mutex_lock(&hive->chamber_mutex);
        int honey_produced = 0;

        for (int c = 0; c < NUM_CHAMBERS && honey_to_produce > 0; c++) {
            Chamber* chamber = &hive->chambers[c];
            if (chamber->honey_count >= MAX_HONEY_PER_CHAMBER) continue;

            int x, y;
            while (honey_to_produce > 0 && chamber->honey_count < MAX_HONEY_PER_CHAMBER && find_empty_cell_for_honey(chamber, &x, &y)) {
                chamber->cells[x][y].has_honey = true;
                chamber->honey_count++;
                hive->honey_count++;
                honey_to_produce--;
                honey_produced++;
            }
        }

        if (honey_produced > 0) {
            hive->produced_honey += honey_produced;
            update_bees_and_honey_count(hive);
            printf("\nColmena #%d - Producción completada:\n", hive->id);
            printf("├─ Miel producida: %d unidades\n", honey_produced);
            printf("└─ Total de miel en la colmena: %d/%d\n", hive->honey_count, MAX_HONEY_PER_HIVE);
        }

        pthread_mutex_unlock(&hive->chamber_mutex);
    }
    pthread_mutex_unlock(&hive->resources.polen_mutex);
}

void manage_polen_collection(ProcessInfo* process_info) {
    Beehive* hive = process_info->hive;
    pthread_mutex_lock(&hive->chamber_mutex);
    time_t current_time = time(NULL);
    int active_workers = 0;
    int total_polen_collected_this_round = 0;

    printf("\nColmena #%d - Recolección de polen:\n", hive->id);

    for (int i = 0; i < hive->bee_count; i++) {
        if (hive->bees[i].type == WORKER && hive->bees[i].is_alive) {
            active_workers++;
            int polen = random_range(MIN_POLEN_PER_TRIP, MAX_POLEN_PER_TRIP);
            
            pthread_mutex_lock(&hive->resources.polen_mutex);
            hive->resources.total_polen += polen;
            hive->resources.polen_for_honey += polen;
            hive->resources.total_polen_collected += polen;
            hive->bees[i].polen_collected += polen;
            total_polen_collected_this_round += polen;
            
            printf("├─ Abeja #%d: %d polen (Total: %d/%d)\n", i, polen, hive->bees[i].polen_collected, random_range(MIN_POLEN_LIFETIME, MAX_POLEN_LIFETIME));
            
            pthread_mutex_unlock(&hive->resources.polen_mutex);
            hive->bees[i].last_collection_time = current_time;

            // Verificar muerte de abeja
            if (hive->bees[i].polen_collected >= random_range(MIN_POLEN_LIFETIME, MAX_POLEN_LIFETIME)) {
                handle_bee_death(process_info, i);
            }
        }
    }

    printf("└─ Resumen de recolección:\n");
    printf("    ├─ Polen recolectado: %d unidades\n", total_polen_collected_this_round);
    printf("    └─ Polen total acumulado: %d unidades\n", hive->resources.total_polen_collected);

    pthread_mutex_unlock(&hive->chamber_mutex);
}

void manage_bee_lifecycle(ProcessInfo* process_info) {
    pthread_mutex_lock(&process_info->hive->chamber_mutex);
    
    // Procesar reina y puesta de huevos
    process_queen_egg_laying(process_info);
    
    // Procesar eclosión de huevos
    process_eggs_hatching(process_info);
    
    pthread_mutex_unlock(&process_info->hive->chamber_mutex);
}

void handle_bee_death(ProcessInfo* process_info, int bee_index) {
    Beehive* hive = process_info->hive;
    hive->bees[bee_index].is_alive = false;
    hive->dead_bees++;
    hive->bee_count--;
    update_bees_and_honey_count(hive);
    
    printf("\nColmena #%d - Muerte de abeja:\n", hive->id);
    printf("├─ Abeja #%d (%s) ha muerto\n", bee_index, hive->bees[bee_index].type == QUEEN ? "REINA" : "OBRERA");
    printf("├─ Polen recolectado en su vida: %d unidades\n", hive->bees[bee_index].polen_collected);
    printf("└─ Total de abejas muertas: %d\n", hive->dead_bees);
}

void create_new_bee(ProcessInfo* process_info, BeeType type) {
    Beehive* hive = process_info->hive;
    if (hive->bee_count >= MAX_BEES) return;

    int new_bee_index = hive->bee_count++;
    hive->bees = realloc(hive->bees, sizeof(Bee) * hive->bee_count);
    
    Bee* new_bee = &hive->bees[new_bee_index];
    new_bee->id = new_bee_index;
    new_bee->type = type;
    new_bee->polen_collected = 0;
    new_bee->is_alive = true;
    new_bee->last_collection_time = time(NULL);
    new_bee->last_egg_laying_time = time(NULL);
    
    hive->born_bees++;
    update_bees_and_honey_count(hive);
}

void process_eggs_hatching(ProcessInfo* process_info) {
    Beehive* hive = process_info->hive;
    time_t current_time = time(NULL);
    int queen_count = count_queen_bees(process_info);
    int eggs_hatched = 0;

    printf("\nColmena #%d - Procesando eclosión de huevos:\n", hive->id);

    for (int c = 0; c < NUM_CHAMBERS; c++) {
        Chamber* chamber = &hive->chambers[c];
        for (int x = 0; x < MAX_CHAMBER_SIZE; x++) {
            for (int y = 0; y < MAX_CHAMBER_SIZE; y++) {
                if (chamber->cells[x][y].has_egg) {
                    double elapsed_time = difftime(current_time, chamber->cells[x][y].egg_lay_time) * 1000;
                    if (elapsed_time >= MAX_EGG_HATCH_TIME) {
                        chamber->cells[x][y].has_egg = false;
                        chamber->egg_count--;
                        hive->egg_count--;
                        hive->hatched_eggs++;
                        eggs_hatched++;

                        if (hive->bee_count < MAX_BEES) {
                            bool will_be_queen = (random_range(1, 100) <= QUEEN_BIRTH_PROBABILITY);
                            if (will_be_queen && queen_count == 1) {
                                hive->should_create_new_hive = true;
                                printf("├─ ¡Nueva reina nacerá! Se creará una nueva colmena\n");
                            } else {
                                create_new_bee(process_info, WORKER);
                            }
                        }
                    }
                }
            }
        }
    }

    if (eggs_hatched > 0) {
        printf("└─ Resumen de eclosiones:\n");
        printf("    ├─ Huevos eclosionados en este ciclo: %d\n", eggs_hatched);
        printf("    ├─ Total de huevos eclosionados: %d\n", hive->hatched_eggs);
        printf("    └─ Huevos restantes: %d\n", hive->egg_count);
    } else {
        printf("└─ No hay huevos listos para eclosionar\n");
    }
}

void process_queen_egg_laying(ProcessInfo* process_info) {
    Beehive* hive = process_info->hive;
    printf("\nColmena #%d - Actividad de la reina:\n", hive->id);

    // Encontrar la reina
    for (int i = 0; i < hive->bee_count; i++) {
        if (hive->bees[i].type == QUEEN && hive->bees[i].is_alive) {
            int eggs_to_lay = random_range(MIN_EGGS_PER_LAYING, MAX_EGGS_PER_LAYING);
            printf("├─ Reina #%d intentará poner %d huevos\n", i, eggs_to_lay);
            
            int eggs_laid = 0;
            // Intentar poner huevos en cámaras disponibles
            for (int c = 0; c < NUM_CHAMBERS && eggs_to_lay > 0; c++) {
                Chamber* chamber = &hive->chambers[c];
                if (chamber->egg_count >= MAX_EGGS_PER_CHAMBER ||
                    hive->egg_count >= MAX_EGGS_PER_HIVE) continue;

                int x, y;
                while (eggs_to_lay > 0 && chamber->egg_count < MAX_EGGS_PER_CHAMBER && find_empty_cell_for_egg(chamber, &x, &y)) {
                    chamber->cells[x][y].has_egg = true;
                    chamber->cells[x][y].egg_lay_time = time(NULL);
                    chamber->egg_count++;
                    hive->egg_count++;
                    eggs_to_lay--;
                    eggs_laid++;
                }
            }
            
            printf("└─ Resultado de puesta:\n");
            printf("    ├─ Huevos puestos: %d\n", eggs_laid);
            printf("    ├─ Total de huevos en la colmena: %d/%d\n", hive->egg_count, MAX_EGGS_PER_HIVE);
            printf("    └─ Capacidad restante: %d huevos\n", MAX_EGGS_PER_HIVE - hive->egg_count);
            
            break; // Solo procesar una reina
        }
    }
}

int count_queen_bees(ProcessInfo* process_info) {
    Beehive* hive = process_info->hive;
    int count = 0;
    for (int i = 0; i < hive->bee_count; i++) {
        if (hive->bees[i].is_alive && hive->bees[i].type == QUEEN) {
            count++;
        }
    }
    return count;
}

void print_chamber_row(ProcessInfo* process_info, int start_index, int end_index) {
    Beehive* hive = process_info->hive;
    
    // Imprimir números de cámara
    for (int c = start_index; c < end_index; c++) {
        printf("Cámara #%d:", c);
        printf("\t\t\t\t\t");
    }
    printf("\n");

    // Imprimir matrices de cámaras
    for (int i = 0; i < MAX_CHAMBER_SIZE; i++) {
        for (int c = start_index; c < end_index; c++) {
            Chamber* chamber = &hive->chambers[c];
            for (int j = 0; j < MAX_CHAMBER_SIZE; j++) {
                Cell* cell = &chamber->cells[i][j];
                if (is_egg_position(i, j)) {
                    printf("H%d ", cell->has_egg ? 1 : 0);
                } else {
                    printf("M%d ", cell->has_honey ? 1 : 0);
                }
            }
            printf("\t\t\t");
        }
        printf("\n");
    }

    // Imprimir estadísticas de cámaras
    for (int c = start_index; c < end_index; c++) {
        Chamber* chamber = &hive->chambers[c];
        printf("Miel: %d/%d, Huevos: %d/%d\t\t\t", chamber->honey_count, MAX_HONEY_PER_CHAMBER, chamber->egg_count, MAX_EGGS_PER_CHAMBER);
    }
    printf("\n\n");
}

void print_chamber_matrix(ProcessInfo* process_info) {
    printf("\nColmena #%d - Estado de las cámaras:\n\n", process_info->hive->id);
    print_chamber_row(process_info, 0, 5);
    print_chamber_row(process_info, 5, 10);
}

void print_detailed_bee_status(ProcessInfo* process_info) {
    Beehive* hive = process_info->hive;
    int alive_workers = 0, dead_workers = 0;
    
    for (int i = 0; i < hive->bee_count; i++) {
        if (hive->bees[i].type == WORKER) {
            if (hive->bees[i].is_alive) {
                alive_workers++;
            } else {
                dead_workers++;
            }
        }
    }
    
    printf("├─ Total de abejas: %d/%d\n", alive_workers + 1, MAX_BEES);
    printf("    ├─ Obreras vivas: %d\n", alive_workers);
    printf("    ├─ Obreras muertas: %d\n", dead_workers);
    printf("    ├─ Abejas nacidas: %d\n", hive->born_bees);
    printf("    └─ Total de muertes: %d\n", hive->dead_bees);
}

void print_beehive_stats(ProcessInfo* process_info) {
    Beehive* hive = process_info->hive;

    printf("\nColmena #%d - Estadísticas Generales:\n", hive->id);
    print_detailed_bee_status(process_info);
    printf("├─ Total de miel: %d/%d\n", hive->honey_count, MAX_HONEY_PER_HIVE);
    printf("├─ Total de huevos: %d/%d\n", hive->egg_count, MAX_EGGS_PER_HIVE);
    printf("├─ Huevos eclosionados: %d\n", hive->hatched_eggs);
    printf("├─ Abejas muertas: %d\n", hive->dead_bees);
    printf("├─ Abejas nacidas: %d\n", hive->born_bees);
    printf("├─ Miel producida: %d\n", hive->produced_honey);
    printf("├─ Polen total recolectado: %d\n", hive->resources.total_polen_collected);
    printf("└─ Recursos para FSJ (abejas + miel): %d\n", hive->bees_and_honey_count);
    print_chamber_matrix(process_info);
}

bool check_new_queen(ProcessInfo* process_info) {
    Beehive* hive = process_info->hive;
    
    pthread_mutex_lock(&hive->chamber_mutex);
    bool needs_new_hive = hive->should_create_new_hive;
    if (needs_new_hive) {
        printf("\nColmena #%d - Nueva reina detectada: Se iniciará una nueva colmena\n", hive->id);
        hive->should_create_new_hive = false;
    }
    pthread_mutex_unlock(&hive->chamber_mutex);
    
    return needs_new_hive;
}