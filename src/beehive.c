#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "../include/core/beehive.h"
#include "../include/core/utils.h"

void distribute_initial_resources(Beehive* hive) {
    int honey_distributed = 0;
    int eggs_distributed = 0;
    
    for (int i = 0; i < MAX_CHAMBER_SIZE && 
         (honey_distributed < hive->honey_count || eggs_distributed < hive->egg_count); i++) {
        for (int j = 0; j < MAX_CHAMBER_SIZE; j++) {
            if (i >= MAX_CHAMBER_SIZE/2-2 && i <= MAX_CHAMBER_SIZE/2+1 &&
                j >= MAX_CHAMBER_SIZE/2-2 && j <= MAX_CHAMBER_SIZE/2+1) {
                // Center area for eggs
                if (eggs_distributed < hive->egg_count && 
                    eggs_distributed < MAX_EGGS_PER_HIVE &&
                    hive->brood_chamber.total_eggs < MAX_EGGS_PER_CHAMBER) {
                    hive->brood_chamber.cells[i][j].eggs = 1;
                    eggs_distributed++;
                    hive->brood_chamber.total_eggs++;
                }
            } else {
                // Outer area for honey
                if (honey_distributed < hive->honey_count && 
                    honey_distributed < MAX_HONEY_PER_HIVE) {
                    hive->honey_chamber.cells[i][j].honey = 1;
                    honey_distributed++;
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
    
    // Asegurar que haya una reina
    int queen_index = random_range(0, hive->bee_count - 1);
    
    for (int i = 0; i < hive->bee_count; i++) {
        hive->bees[i].id = i;
        hive->bees[i].type = (i == queen_index) ? QUEEN : WORKER;
        hive->bees[i].polen_collected = 0;
        hive->bees[i].max_polen_capacity = random_range(1, 5);
        hive->bees[i].is_alive = true;
        hive->bees[i].hive = hive;
        
        if (i == queen_index) {
            hive->has_queen = true;
        }
    }
    
    // Initialize chambers
    memset(&hive->honey_chamber, 0, sizeof(Chamber));
    memset(&hive->brood_chamber, 0, sizeof(Chamber));
    hive->honey_chamber.total_eggs = 0;
    hive->brood_chamber.total_eggs = 0;
    
    // Initialize initial distribution
    distribute_initial_resources(hive);
    
    hive->state = READY;
    hive->threads.threads_running = false;
    
    // Iniciar los hilos de la colmena
    start_hive_threads(hive);
}

void* honey_production_thread(void* arg) {
    Beehive* hive = (Beehive*)arg;
    
    while (hive->threads.threads_running) {
        pthread_mutex_lock(&hive->chamber_mutex);
        
        // Convertir polen a miel (10:1)
        for (int i = 0; i < hive->bee_count; i++) {
            if (hive->bees[i].polen_collected >= POLEN_TO_HONEY_RATIO) {
                int honey_to_produce = hive->bees[i].polen_collected / POLEN_TO_HONEY_RATIO;
                if (hive->honey_count + honey_to_produce <= MAX_HONEY_PER_HIVE) {
                    // Buscar celda vacía para miel
                    for (int x = 0; x < MAX_CHAMBER_SIZE && honey_to_produce > 0; x++) {
                        for (int y = 0; y < MAX_CHAMBER_SIZE && honey_to_produce > 0; y++) {
                            if (!(x >= MAX_CHAMBER_SIZE/2-2 && x <= MAX_CHAMBER_SIZE/2+1 &&
                                y >= MAX_CHAMBER_SIZE/2-2 && y <= MAX_CHAMBER_SIZE/2+1)) {
                                if (hive->honey_chamber.cells[x][y].honey == 0) {
                                    hive->honey_chamber.cells[x][y].honey = 1;
                                    hive->honey_count++;
                                    honey_to_produce--;
                                }
                            }
                        }
                    }
                    hive->bees[i].polen_collected = 0;
                }
            }
        }
        
        pthread_mutex_unlock(&hive->chamber_mutex);
        delay_ms(random_range(100, 300));
    }
    
    return NULL;
}

void* polen_collection_thread(void* arg) {
    Beehive* hive = (Beehive*)arg;
    
    while (hive->threads.threads_running) {
        pthread_mutex_lock(&hive->chamber_mutex);
        
        // Solo las obreras recolectan polen
        for (int i = 0; i < hive->bee_count; i++) {
            if (hive->bees[i].type == WORKER && hive->bees[i].is_alive) {
                int polen = random_range(1, 5);
                hive->bees[i].polen_collected += polen;
                
                // Verificar si la abeja debe morir (excepto la reina)
                if (hive->bees[i].polen_collected >= random_range(100, 150)) {
                    hive->bees[i].is_alive = false;
                }
            }
        }
        
        pthread_mutex_unlock(&hive->chamber_mutex);
        delay_ms(random_range(100, 300));
    }
    
    return NULL;
}

void* egg_hatching_thread(void* arg) {
    Beehive* hive = (Beehive*)arg;
    
    while (hive->threads.threads_running) {
        pthread_mutex_lock(&hive->chamber_mutex);
        
        // Procesar nacimientos
        for (int i = MAX_CHAMBER_SIZE/2-2; i <= MAX_CHAMBER_SIZE/2+1; i++) {
            for (int j = MAX_CHAMBER_SIZE/2-2; j <= MAX_CHAMBER_SIZE/2+1; j++) {
                if (hive->brood_chamber.cells[i][j].eggs > 0) {
                    // Simular tiempo de eclosión
                    if (random_range(1, 100) <= 10) { // 10% de probabilidad de eclosión
                        hive->brood_chamber.cells[i][j].eggs--;
                        hive->egg_count--;
                        hive->brood_chamber.total_eggs--;
                        
                        // Crear nueva abeja
                        hive->bee_count++;
                        hive->bees = realloc(hive->bees, sizeof(Bee) * hive->bee_count);
                        int new_bee_index = hive->bee_count - 1;
                        
                        // Inicializar nueva abeja
                        hive->bees[new_bee_index].id = new_bee_index;
                        hive->bees[new_bee_index].type = !hive->has_queen ? QUEEN : WORKER;
                        hive->bees[new_bee_index].polen_collected = 0;
                        hive->bees[new_bee_index].max_polen_capacity = random_range(1, 5);
                        hive->bees[new_bee_index].is_alive = true;
                        hive->bees[new_bee_index].hive = hive;
                        
                        if (hive->bees[new_bee_index].type == QUEEN) {
                            hive->has_queen = true;
                        }
                    }
                }
            }
        }
        
        // Poner nuevos huevos si hay espacio
        if (hive->has_queen && hive->egg_count < MAX_EGGS_PER_HIVE) {
            for (int i = MAX_CHAMBER_SIZE/2-2; i <= MAX_CHAMBER_SIZE/2+1 && hive->egg_count < MAX_EGGS_PER_HIVE; i++) {
                for (int j = MAX_CHAMBER_SIZE/2-2; j <= MAX_CHAMBER_SIZE/2+1 && hive->egg_count < MAX_EGGS_PER_HIVE; j++) {
                    if (hive->brood_chamber.cells[i][j].eggs == 0 &&
                        hive->brood_chamber.total_eggs < MAX_EGGS_PER_CHAMBER) {
                        if (random_range(1, 100) <= 20) { // 20% de probabilidad de poner huevo
                            hive->brood_chamber.cells[i][j].eggs = 1;
                            hive->egg_count++;
                            hive->brood_chamber.total_eggs++;
                        }
                    }
                }
            }
        }
        
        pthread_mutex_unlock(&hive->chamber_mutex);
        delay_ms(random_range(1, 10));
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
    
    // Convert polen to honey (10:1 ratio)
    int honey_produced = polen_amount / POLEN_TO_HONEY_RATIO;
    if (honey_produced > 0 && hive->honey_count < MAX_HONEY_PER_HIVE) {
        // Find random empty cell
        int attempts = 0;
        while (attempts < 100 && hive->honey_count < MAX_HONEY_PER_HIVE) {
            int x = random_range(0, MAX_CHAMBER_SIZE-1);
            int y = random_range(0, MAX_CHAMBER_SIZE-1);
            
            // Avoid center area reserved for eggs
            if (!(x >= MAX_CHAMBER_SIZE/2-2 && x <= MAX_CHAMBER_SIZE/2+1 &&
                  y >= MAX_CHAMBER_SIZE/2-2 && y <= MAX_CHAMBER_SIZE/2+1)) {
                hive->honey_chamber.cells[x][y].honey += honey_produced;
                hive->honey_count += honey_produced;
                break;
            }
            attempts++;
        }
    }
    
    pthread_mutex_unlock(&hive->chamber_mutex);
}

bool check_new_queen(Beehive* hive) {
    pthread_mutex_lock(&hive->chamber_mutex);
    bool new_queen_found = false;
    
    // Solo buscar nueva reina si no hay una viva
    if (!hive->has_queen) {
        // Intentar crear nueva reina desde huevo existente
        if (hive->egg_count > 0 && random_range(1, 100) <= QUEEN_BIRTH_PROBABILITY) {
            new_queen_found = true;
            // La nueva reina se creará cuando el huevo eclosione
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

void print_chamber_matrix(Beehive* hive) {
    printf("\nColmena #%d - Matriz de cámaras:\n", hive->id);
    
    for (int i = 0; i < MAX_CHAMBER_SIZE; i++) {
        for (int j = 0; j < MAX_CHAMBER_SIZE; j++) {
            if (hive->brood_chamber.cells[i][j].eggs > 0) {
                printf("H1  ");
            } else if (hive->honey_chamber.cells[i][j].honey > 0) {
                printf("M1  ");
            } else {
                // Print M0 for empty honey cells in honey area, H0 for empty egg cells in egg area
                if (i >= MAX_CHAMBER_SIZE/2-2 && i <= MAX_CHAMBER_SIZE/2+1 &&
                    j >= MAX_CHAMBER_SIZE/2-2 && j <= MAX_CHAMBER_SIZE/2+1) {
                    printf("H0  ");
                } else {
                    printf("M0  ");
                }
            }
        }
        printf("\n");
    }
}

void print_beehive_stats(Beehive* hive) {
    printf("\nColmena #%d, Estadísticas:\n", hive->id);
    printf("Total de abejas: %d\n", hive->bee_count);
    printf("Abejas vivas: ");
    int alive_count = 0;
    int queen_count = 0;
    int worker_count = 0;
    
    for (int i = 0; i < hive->bee_count; i++) {
        if (hive->bees[i].is_alive) {
            alive_count++;
            if (hive->bees[i].type == QUEEN) queen_count++;
            else worker_count++;
        }
    }
    
    printf("%d (Reina: %d, Obreras: %d)\n", alive_count, queen_count, worker_count);
    printf("Total de miel: %d/%d\n", hive->honey_count, MAX_HONEY_PER_HIVE);
    printf("Total de huevos: %d/%d\n", hive->egg_count, MAX_EGGS_PER_HIVE);
    printf("Huevos por cámara: %d/%d\n", hive->brood_chamber.total_eggs, MAX_EGGS_PER_CHAMBER);
    print_chamber_matrix(hive);
}