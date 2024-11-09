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
    hive->has_queen = false;
    
    pthread_mutex_init(&hive->chamber_mutex, NULL);
    sem_init(&hive->resource_sem, 0, 1);
    
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
        
        if (i == queen_index) {
            hive->has_queen = true;
        }
    }
    
    // Inicializar cámaras
    init_chambers(hive);
    
    hive->state = READY;
    hive->threads.threads_running = false;
    
    start_hive_threads(hive);
}

void* honey_production_thread(void* arg) {
    Beehive* hive = (Beehive*)arg;
    
    while (hive->threads.threads_running) {
        pthread_mutex_lock(&hive->threads.resources.polen_mutex);
        
        // Convertir polen a miel (10:1)
        if (hive->threads.resources.polen_for_honey >= POLEN_TO_HONEY_RATIO) {
            int honey_to_produce = hive->threads.resources.polen_for_honey / POLEN_TO_HONEY_RATIO;
            hive->threads.resources.polen_for_honey %= POLEN_TO_HONEY_RATIO; // Mantener el residuo
            
            printf("\n[Hilo Producción Miel - Colmena %d] ", hive->id);
            printf("Polen disponible para convertir: %d, ", hive->threads.resources.polen_for_honey);
            printf("Miel a producir: %d\n", honey_to_produce);
            
            pthread_mutex_lock(&hive->chamber_mutex);
            int honey_produced = 0;
            
            // Intentar depositar miel en cámaras disponibles
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
            
            printf("[Hilo Producción Miel - Colmena %d] ", hive->id);
            printf("Miel producida: %d, Total miel en colmena: %d/%d\n", 
                   honey_produced, hive->honey_count, MAX_HONEY_PER_HIVE);
            
            pthread_mutex_unlock(&hive->chamber_mutex);
        }
        
        pthread_mutex_unlock(&hive->threads.resources.polen_mutex);
        delay_ms(2000); // Aumentado para dar más tiempo entre producciones
    }
    
    return NULL;
}

void* polen_collection_thread(void* arg) {
    Beehive* hive = (Beehive*)arg;
    
    while (hive->threads.threads_running) {
        pthread_mutex_lock(&hive->chamber_mutex);
        
        printf("\n[Hilo Recolección Polen - Colmena %d] Iniciando ciclo de recolección\n", hive->id);
        int total_polen_collected = 0;
        int active_workers = 0;
        
        // Solo las obreras recolectan polen
        for (int i = 0; i < hive->bee_count; i++) {
            if (hive->bees[i].type == WORKER && hive->bees[i].is_alive) {
                active_workers++;
                int polen = random_range(MIN_POLEN_PER_TRIP, MAX_POLEN_PER_TRIP);
                
                pthread_mutex_lock(&hive->threads.resources.polen_mutex);
                hive->threads.resources.total_polen += polen;
                hive->threads.resources.polen_for_honey += polen;
                total_polen_collected += polen;
                
                printf("[Abeja %d] Recolectó %d polen (Total personal: %d)\n", 
                       hive->bees[i].id, polen, hive->bees[i].polen_collected + polen);
                
                pthread_mutex_unlock(&hive->threads.resources.polen_mutex);
                
                hive->bees[i].polen_collected += polen;
                hive->bees[i].last_collection_time = time(NULL);
                
                // Verificar si la abeja debe morir
                if (hive->bees[i].polen_collected >= random_range(MIN_POLEN_LIFETIME, MAX_POLEN_LIFETIME)) {
                    hive->bees[i].is_alive = false;
                    printf("[Abeja %d] ¡Ha muerto! Polen total recolectado en su vida: %d\n", 
                           hive->bees[i].id, hive->bees[i].polen_collected);
                }
            }
        }
        
        printf("[Hilo Recolección Polen - Colmena %d] ", hive->id);
        printf("Resumen: %d obreras activas, Polen recolectado en este ciclo: %d, ", 
               active_workers, total_polen_collected);
        printf("Polen total en colmena: %d\n", hive->threads.resources.total_polen);
        
        pthread_mutex_unlock(&hive->chamber_mutex);
        delay_ms(2000); // Aumentado para dar más tiempo entre recolecciones
    }
    
    return NULL;
}

void* egg_hatching_thread(void* arg) {
    Beehive* hive = (Beehive*)arg;
    
    while (hive->threads.threads_running) {
        pthread_mutex_lock(&hive->chamber_mutex);
        time_t current_time = time(NULL);
        
        printf("\n[Hilo Nacimiento - Colmena %d] Iniciando ciclo de reproducción\n", hive->id);
        
        // Gestionar la reina y puesta de huevos
        for (int i = 0; i < hive->bee_count; i++) {
            if (hive->bees[i].type == QUEEN && hive->bees[i].is_alive) {
                int eggs_to_lay = random_range(MIN_EGGS_PER_LAYING, MAX_EGGS_PER_LAYING);
                int eggs_laid = 0;
                
                printf("[Reina - Colmena %d] Intentando poner %d huevos\n", hive->id, eggs_to_lay);
                
                // Intentar poner huevos en cámaras disponibles
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
                            }
                        }
                    }
                }
                
                printf("[Reina - Colmena %d] Huevos puestos: %d, Total huevos en colmena: %d/%d\n",
                       hive->id, eggs_laid, hive->egg_count, MAX_EGGS_PER_HIVE);
                break; // Solo hay una reina
            }
        }
        
        // Gestionar eclosión de huevos
        int eggs_hatched = 0;
        bool new_queen_born = false;
        
        for (int c = 0; c < NUM_CHAMBERS && !new_queen_born; c++) {
            Chamber* chamber = &hive->chambers[c];
            for (int x = 0; x < MAX_CHAMBER_SIZE && !new_queen_born; x++) {
                for (int y = 0; y < MAX_CHAMBER_SIZE && !new_queen_born; y++) {
                    if (chamber->cells[x][y].has_egg) {
                        double elapsed_time = difftime(current_time, chamber->cells[x][y].egg_lay_time) * 1000;
                        if (elapsed_time >= MAX_EGG_HATCH_TIME) {
                            // Eclosión del huevo
                            chamber->cells[x][y].has_egg = false;
                            chamber->egg_count--;
                            hive->egg_count--;
                            eggs_hatched++;
                            
                            // Crear nueva abeja si hay espacio
                            if (hive->bee_count < MAX_BEES) {
                                int new_bee_index = hive->bee_count;
                                hive->bee_count++;
                                hive->bees = realloc(hive->bees, sizeof(Bee) * hive->bee_count);
                                
                                // Determinar si será reina (90% de probabilidad)
                                bool will_be_queen = random_range(1, 100) <= QUEEN_BIRTH_PROBABILITY;
                                
                                // Inicializar nueva abeja
                                Bee* new_bee = &hive->bees[new_bee_index];
                                new_bee->id = new_bee_index;
                                new_bee->type = will_be_queen ? QUEEN : WORKER;
                                new_bee->polen_collected = 0;
                                new_bee->is_alive = true;
                                new_bee->hive = hive;
                                new_bee->last_collection_time = current_time;
                                new_bee->last_egg_laying_time = current_time;
                                
                                printf("[Nacimiento - Colmena %d] Nueva abeja %d nacida (Tipo: %s)\n",
                                       hive->id, new_bee->id, new_bee->type == QUEEN ? "Reina" : "Obrera");
                                
                                if (new_bee->type == QUEEN) {
                                    printf("[Nacimiento - Colmena %d] ¡Nueva reina nacida! Se creará una nueva colmena.\n", 
                                           hive->id);
                                    new_queen_born = true;
                                    break; // Salir del bucle más interno
                                }
                            }
                        }
                    }
                }
                if (new_queen_born) break; // Salir del bucle medio
            }
            if (new_queen_born) break; // Salir del bucle externo
        }
        
        printf("[Hilo Nacimiento - Colmena %d] ", hive->id);
        printf("Huevos eclosionados: %d, Abejas totales: %d/%d\n",
               eggs_hatched, hive->bee_count, MAX_BEES);
        
        pthread_mutex_unlock(&hive->chamber_mutex);
        delay_ms(2000);
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

void deposit_polen(Beehive* hive, int polen_amount) {
    pthread_mutex_lock(&hive->chamber_mutex);
    
    int honey_produced = polen_amount / POLEN_TO_HONEY_RATIO;
    if (honey_produced > 0 && hive->honey_count < MAX_HONEY_PER_HIVE) {
        for (int c = 0; c < NUM_CHAMBERS && honey_produced > 0; c++) {
            Chamber* chamber = &hive->chambers[c];
            // Verificar límite de miel por cámara
            if (chamber->honey_count >= MAX_HONEY_PER_CHAMBER) {
                continue;
            }
            
            for (int x = 0; x < MAX_CHAMBER_SIZE && honey_produced > 0; x++) {
                for (int y = 0; y < MAX_CHAMBER_SIZE && honey_produced > 0; y++) {
                    if (!is_egg_position(x, y) && !chamber->cells[x][y].has_honey && 
                        chamber->honey_count < MAX_HONEY_PER_CHAMBER) {
                        chamber->cells[x][y].has_honey = true;
                        chamber->honey_count++;
                        hive->honey_count++;
                        honey_produced--;
                    }
                }
            }
        }
    }
    
    pthread_mutex_unlock(&hive->chamber_mutex);
}

bool check_new_queen(Beehive* hive) {
    pthread_mutex_lock(&hive->chamber_mutex);
    bool new_queen_found = false;
    
    // Verificar si hay una reina nueva
    for (int i = 0; i < hive->bee_count && !new_queen_found; i++) {
        if (hive->bees[i].type == QUEEN && hive->bees[i].is_alive && 
            difftime(time(NULL), hive->bees[i].last_egg_laying_time) < 10) { // Reina recién nacida
            new_queen_found = true;
        }
    }
    
    pthread_mutex_unlock(&hive->chamber_mutex);
    return new_queen_found;
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
    delay_ms(2000);
}