#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "../include/core/beehive.h"
#include "../include/core/utils.h"

bool is_egg_position(int i, int j) {
    if (i >= 2 && i <= 7) {  // Filas 3-8
        if (i == 4 || i == 5) {  // Filas 5-6
            return (j >= 1 && j <= 8);  // Todo excepto primera y última columna
        } else {
            return (j >= 2 && j <= 7);  // Columnas 3-8
        }
    }
    return false;
}

void init_chambers(Beehive* hive) {
    time_t current_time = time(NULL);

    for (int c = 0; c < NUM_CHAMBERS; c++) {
        Chamber* chamber = &hive->chambers[c];
        memset(chamber, 0, sizeof(Chamber));
        
        // Inicializar todas las celdas como vacías
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
    
    // Distribuir recursos entre las cámaras
    for (int c = 0; c < NUM_CHAMBERS && (honey_remaining > 0 || eggs_remaining > 0); c++) {
        Chamber* chamber = &hive->chambers[c];
        
        // Distribuir huevos solo en posiciones H
        if (eggs_remaining > 0) {
            for (int i = 0; i < MAX_CHAMBER_SIZE && eggs_remaining > 0; i++) {
                for (int j = 0; j < MAX_CHAMBER_SIZE && eggs_remaining > 0; j++) {
                    if (is_egg_position(i, j) && chamber->egg_count < MAX_EGGS_PER_CHAMBER) {
                        chamber->cells[i][j].has_egg = true;
                        chamber->cells[i][j].egg_lay_time = current_time;
                        chamber->egg_count++;
                        eggs_remaining--;
                    }
                }
            }
        }
        
        // Distribuir miel solo en posiciones M
        if (honey_remaining > 0) {
            for (int i = 0; i < MAX_CHAMBER_SIZE && honey_remaining > 0; i++) {
                for (int j = 0; j < MAX_CHAMBER_SIZE && honey_remaining > 0; j++) {
                    if (!is_egg_position(i, j) && chamber->honey_count < MAX_HONEY_PER_CHAMBER) {
                        chamber->cells[i][j].has_honey = true;
                        chamber->honey_count++;
                        honey_remaining--;
                    }
                }
            }
        }
    }
}

void init_beehive(Beehive* hive, int id) {
    hive->id = id;
    hive->bee_count = random_range(MIN_BEES, MAX_BEES);
    hive->honey_count = random_range(MIN_HONEY, MAX_HONEY);
    hive->egg_count = random_range(MIN_EGGS, MAX_EGGS);
    hive->hatched_eggs = 0;
    hive->dead_bees = 0;
    hive->born_bees = 0;
    hive->produced_honey = 0;
    hive->should_terminate = 0;

    pthread_mutex_init(&hive->chamber_mutex, NULL);
    sem_init(&hive->resource_sem, 0, 1);

    // Inicializar recursos de producción
    hive->threads.resources.total_polen = 0;
    hive->threads.resources.polen_for_honey = 0;
    hive->threads.resources.total_polen_collected = 0;
    pthread_mutex_init(&hive->threads.resources.polen_mutex, NULL);

    // Initialize bees
    hive->bees = malloc(sizeof(Bee) * hive->bee_count);
    int queen_index = random_range(0, hive->bee_count - 1);

    time_t current_time = time(NULL);

    for (int i = 0; i < hive->bee_count; i++) {
        hive->bees[i].id = i;
        hive->bees[i].type = (i == queen_index) ? QUEEN : WORKER;
        hive->bees[i].polen_collected = 0;
        hive->bees[i].is_alive = true;
        hive->bees[i].hive = hive;
        hive->bees[i].last_collection_time = current_time;
    }

    init_chambers(hive);
    hive->state = READY;
    hive->threads.threads_running = false;

    start_hive_threads(hive);
}

// Función auxiliar para manejar la eclosión de huevos
static void handle_egg_hatching(Beehive* hive, Chamber* chamber, int x, int y, int queen_count) {
    chamber->cells[x][y].has_egg = false;
    chamber->egg_count--;
    hive->egg_count--;
    hive->hatched_eggs++;
    
    if (hive->bee_count < MAX_BEES) {
        bool will_be_queen = (random_range(1, 100) <= QUEEN_BIRTH_PROBABILITY);
        
        if (will_be_queen && queen_count == 1) {
            hive->should_create_new_hive = true;
            hive->born_bees++;
        } else {
            int new_bee_index = hive->bee_count;
            hive->bee_count++;
            hive->bees = realloc(hive->bees, sizeof(Bee) * hive->bee_count);
            
            if (hive->bees != NULL) {
                Bee* new_bee = &hive->bees[new_bee_index];
                new_bee->id = new_bee_index;
                new_bee->type = WORKER;
                new_bee->polen_collected = 0;
                new_bee->is_alive = true;
                new_bee->hive = hive;
                new_bee->last_collection_time = time(NULL);
                new_bee->last_egg_laying_time = time(NULL);
                hive->born_bees++;
            }
        }
    }
}

void* honey_production_thread(void* arg) {
    Beehive* hive = (Beehive*)arg;
    
    while (hive->threads.threads_running) {
        // Lock más corto y específico
        if (pthread_mutex_trylock(&hive->threads.resources.polen_mutex) == 0) {
            int polen_available = hive->threads.resources.polen_for_honey;
            int honey_to_produce = 0;
            
            if (polen_available >= POLEN_TO_HONEY_RATIO) {
                honey_to_produce = polen_available / POLEN_TO_HONEY_RATIO;
                hive->threads.resources.polen_for_honey %= POLEN_TO_HONEY_RATIO;
            }
            pthread_mutex_unlock(&hive->threads.resources.polen_mutex);
            
            if (honey_to_produce > 0) {
                if (pthread_mutex_trylock(&hive->chamber_mutex) == 0) {
                    int honey_produced = 0;
                    
                    for (int c = 0; c < NUM_CHAMBERS && honey_to_produce > 0; c++) {
                        Chamber* chamber = &hive->chambers[c];
                        if (chamber->honey_count >= MAX_HONEY_PER_CHAMBER) continue;
                        
                        for (int x = 0; x < MAX_CHAMBER_SIZE && honey_to_produce > 0; x++) {
                            for (int y = 0; y < MAX_CHAMBER_SIZE && honey_to_produce > 0; y++) {
                                if (!is_egg_position(x, y) && !chamber->cells[x][y].has_honey &&
                                    chamber->honey_count < MAX_HONEY_PER_CHAMBER) {
                                    chamber->cells[x][y].has_honey = true;
                                    chamber->honey_count++;
                                    hive->honey_count++;
                                    honey_to_produce--;
                                    honey_produced++;
                                }
                            }
                        }
                    }

                    if (honey_produced > 0) {
                        hive->produced_honey += honey_produced;
                        printf("\n[Hilo Producción Miel - Colmena %d] Miel producida: %d, Total: %d/%d\n", 
                               hive->id, honey_produced, hive->honey_count, MAX_HONEY_PER_HIVE);
                    }
                    
                    pthread_mutex_unlock(&hive->chamber_mutex);
                }
            }
        }
        
        usleep(50000); // Reducido a 500ms
    }
    return NULL;
}

void* polen_collection_thread(void* arg) {
    Beehive* hive = (Beehive*)arg;

    while (hive->threads.threads_running && !hive->should_terminate) {
        bool collected = false;
        
        if (pthread_mutex_trylock(&hive->chamber_mutex) == 0) {
            int total_polen_collected = 0;
            int active_workers = 0;

            // Solo las obreras recolectan polen
            for (int i = 0; i < hive->bee_count && !hive->should_terminate; i++) {
                if (hive->bees[i].type == WORKER && hive->bees[i].is_alive) {
                    active_workers++;
                    int polen = random_range(MIN_POLEN_PER_TRIP, MAX_POLEN_PER_TRIP);
                    
                    if (pthread_mutex_trylock(&hive->threads.resources.polen_mutex) == 0) {
                        hive->threads.resources.total_polen += polen;
                        hive->threads.resources.polen_for_honey += polen;
                        hive->threads.resources.total_polen_collected += polen;
                        total_polen_collected += polen;
                        pthread_mutex_unlock(&hive->threads.resources.polen_mutex);
                        
                        hive->bees[i].polen_collected += polen;
                        collected = true;
                        
                        // Verificar muerte de abeja
                        if (hive->bees[i].polen_collected >= random_range(MIN_POLEN_LIFETIME, MAX_POLEN_LIFETIME)) {
                            hive->bees[i].is_alive = false;
                            hive->dead_bees++;
                        }
                    }
                }
            }

            if (collected) {
                printf("[Hilo Recolección Polen - Colmena %d] Obreras activas: %d, Polen recolectado: %d\n",
                       hive->id, active_workers, total_polen_collected);
            }
            
            pthread_mutex_unlock(&hive->chamber_mutex);
        }
        
        usleep(50000); // Reducido a 500ms
    }
    return NULL;
}

void* egg_hatching_thread(void* arg) {
    Beehive* hive = (Beehive*)arg;
    
    while (hive->threads.threads_running && !hive->should_terminate) {
        bool activity = false;
        
        if (pthread_mutex_trylock(&hive->chamber_mutex) == 0) {
            time_t current_time = time(NULL);
            
            // Contar reinas y gestionar huevos
            int queen_count = 0;
            int eggs_laid = 0;
            int eggs_hatched = 0;
            
            for (int i = 0; i < hive->bee_count && !hive->should_terminate; i++) {
                if (hive->bees[i].type == QUEEN && hive->bees[i].is_alive) {
                    queen_count++;
                }
            }
            
            // Puesta de huevos si hay una reina
            if (queen_count == 1) {
                int eggs_to_lay = random_range(MIN_EGGS_PER_LAYING, MAX_EGGS_PER_LAYING);
                
                for (int c = 0; c < NUM_CHAMBERS && eggs_to_lay > 0; c++) {
                    Chamber* chamber = &hive->chambers[c];
                    if (chamber->egg_count >= MAX_EGGS_PER_CHAMBER ||
                        hive->egg_count >= MAX_EGGS_PER_HIVE) continue;
                    
                    for (int x = 0; x < MAX_CHAMBER_SIZE && eggs_to_lay > 0; x++) {
                        for (int y = 0; y < MAX_CHAMBER_SIZE && eggs_to_lay > 0; y++) {
                            if (is_egg_position(x, y) && !chamber->cells[x][y].has_egg) {
                                chamber->cells[x][y].has_egg = true;
                                chamber->cells[x][y].egg_lay_time = current_time;
                                chamber->egg_count++;
                                hive->egg_count++;
                                eggs_to_lay--;
                                eggs_laid++;
                                activity = true;
                            }
                        }
                    }
                }
            }
            
            // Eclosión de huevos
            for (int c = 0; c < NUM_CHAMBERS && !hive->should_terminate; c++) {
                Chamber* chamber = &hive->chambers[c];
                for (int x = 0; x < MAX_CHAMBER_SIZE; x++) {
                    for (int y = 0; y < MAX_CHAMBER_SIZE; y++) {
                        if (chamber->cells[x][y].has_egg) {
                            double elapsed_time = difftime(current_time, chamber->cells[x][y].egg_lay_time) * 1000;
                            if (elapsed_time >= MAX_EGG_HATCH_TIME) {
                                handle_egg_hatching(hive, chamber, x, y, queen_count);
                                eggs_hatched++;
                                activity = true;
                            }
                        }
                    }
                }
            }
            
            if (activity) {
                printf("[Hilo Nacimiento - Colmena %d] Huevos puestos: %d, Eclosionados: %d\n",
                       hive->id, eggs_laid, eggs_hatched);
            }
            
            pthread_mutex_unlock(&hive->chamber_mutex);
        }
        
        usleep(50000); // Reducido a 500ms
    }
    return NULL;
}

void start_hive_threads(Beehive* hive) {
    hive->threads.threads_running = true;
    pthread_create(&hive->threads.honey_production, NULL, honey_production_thread, hive);
    pthread_create(&hive->threads.polen_collection, NULL, polen_collection_thread, hive);
    pthread_create(&hive->threads.egg_hatching, NULL, egg_hatching_thread, hive);
}

void stop_hive_threads(Beehive* hive) {
    hive->threads.threads_running = false;
    pthread_join(hive->threads.honey_production, NULL);
    pthread_join(hive->threads.polen_collection, NULL);
    pthread_join(hive->threads.egg_hatching, NULL);
}

bool check_new_queen(Beehive* hive) {
    pthread_mutex_lock(&hive->chamber_mutex);
    bool needs_new_hive = hive->should_create_new_hive;
    if (needs_new_hive) {
        hive->should_create_new_hive = false;  // Reset the flag
        printf("[Colmena %d] Detectada necesidad de nueva colmena\n", hive->id);
    }
    pthread_mutex_unlock(&hive->chamber_mutex);
    return needs_new_hive;
}

void cleanup_beehive(Beehive* hive) {
    // Detener los tres hilos principales
    stop_hive_threads(hive);
    
    // Liberar memoria y destruir mutex/semáforos
    free(hive->bees);
    pthread_mutex_destroy(&hive->chamber_mutex);
    sem_destroy(&hive->resource_sem);
}

void print_single_chamber(Chamber* chamber, int chamber_index) {
    printf("\nCámara #%d:\n", chamber_index);
    
    for (int i = 0; i < MAX_CHAMBER_SIZE; i++) {
        for (int j = 0; j < MAX_CHAMBER_SIZE; j++) {
            Cell* cell = &chamber->cells[i][j];
            if (is_egg_position(i, j)) {
                // Posición para huevos
                printf("H%d  ", cell->has_egg ? 1 : 0);
            } else {
                // Posición para miel
                printf("M%d  ", cell->has_honey ? 1 : 0);
            }
        }
        printf("\n");
    }
    printf("Miel en cámara: %d/%d\n", chamber->honey_count, MAX_HONEY_PER_CHAMBER);
    printf("Huevos en cámara: %d/%d\n", chamber->egg_count, MAX_EGGS_PER_CHAMBER);
}

void print_chamber_matrix(Beehive* hive) {
    printf("\nColmena #%d - Estado de las cámaras:\n", hive->id);
    
    for (int i = 0; i < NUM_CHAMBERS; i++) {
        print_single_chamber(&hive->chambers[i], i);
    }
}

void print_beehive_stats(Beehive* hive) {
    printf("\nColmena #%d, Estadísticas:\n", hive->id);
    
    int alive_count = 0;
    int queen_count = 0;
    int worker_count = 0;
    
    for (int i = 0; i < hive->bee_count; i++) {
        if (hive->bees[i].is_alive) {
            alive_count++;
            if (hive->bees[i].type == QUEEN) {
                queen_count++;
            } else if (hive->bees[i].type == WORKER) {
                worker_count++;
            }
        }
    }
    
    // Ajustar bee_count para reflejar solo las abejas vivas
    hive->bee_count = alive_count;
    
    printf("Total de abejas: %d\n", hive->bee_count);
    printf("Abejas vivas: %d (Reina: %d, Obreras: %d)\n", alive_count, queen_count, worker_count);
    printf("Total de miel: %d/%d\n", hive->honey_count, MAX_HONEY_PER_HIVE);
    printf("Total de huevos: %d/%d\n", hive->egg_count, MAX_EGGS_PER_HIVE);
    
    // Mostrar recursos por cámara
    for (int c = 0; c < NUM_CHAMBERS; c++) {
        Chamber* chamber = &hive->chambers[c];
        printf("Cámara %d - Miel: %d, Huevos: %d/%d\n",
                c, chamber->honey_count, chamber->egg_count, MAX_EGGS_PER_CHAMBER);
    }
    
    print_chamber_matrix(hive);
}