#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "../include/core/beehive.h"
#include "../include/core/utils.h"
#include "../include/core/process_manager.h"

void init_beehive(Beehive* hive, int id) {
    hive->id = id;
    hive->bee_count = random_range(MIN_BEES, MAX_BEES);
    hive->max_bees = MAX_BEES;
    hive->honey_count = random_range(MIN_HONEY, MAX_HONEY);
    hive->egg_count = random_range(MIN_EGGS, MAX_EGGS);
    
    printf("Inicializando colmena %d con %d abejas\n", id, hive->bee_count);
    
    pthread_mutex_init(&hive->chamber_mutex, NULL);
    sem_init(&hive->resource_sem, 0, 1);
    
    // Initialize bees
    hive->bees = calloc(hive->max_bees, sizeof(Bee));  // Cambiado a calloc para inicializar en 0
    if (!hive->bees) {
        printf("Error allocating memory for bees\n");
        return;
    }
    
    for (int i = 0; i < hive->bee_count; i++) {
        hive->bees[i].id = i;
        // 5% probabilidad de reina, 20% exploradoras, resto obreras
        int random_val = random_range(1, 100);
        hive->bees[i].type = (random_val <= QUEEN_BIRTH_PROBABILITY) ? QUEEN :
                            (random_val <= 20) ? SCOUT : WORKER;
        hive->bees[i].polen_collected = 0;
        hive->bees[i].max_polen_capacity = random_range(1, 5);
        hive->bees[i].is_alive = true;
        hive->bees[i].hive = hive;
        
        // Create thread for bee
        BeeThreadArgs* args = malloc(sizeof(BeeThreadArgs));
        if (!args) {
            printf("Error allocating memory for bee args\n");
            continue;
        }
        args->bee = &hive->bees[i];
        args->hive = hive;
        
        if (pthread_create(&hive->bees[i].thread, NULL, bee_lifecycle, args) != 0) {
            printf("Error creating thread for bee %d\n", i);
            free(args);
            continue;
        }
    }
    
    // Initialize chambers
    memset(&hive->honey_chamber, 0, sizeof(Chamber));
    memset(&hive->brood_chamber, 0, sizeof(Chamber));
    
    // Distribute initial honey and eggs
    int honey_distributed = 0;
    int eggs_distributed = 0;
    
    for (int i = 0; i < MAX_CHAMBER_SIZE && 
         (honey_distributed < hive->honey_count || eggs_distributed < hive->egg_count); i++) {
        for (int j = 0; j < MAX_CHAMBER_SIZE; j++) {
            if (i >= MAX_CHAMBER_SIZE/2-2 && i <= MAX_CHAMBER_SIZE/2+1 &&
                j >= MAX_CHAMBER_SIZE/2-2 && j <= MAX_CHAMBER_SIZE/2+1) {
                // Center area for eggs
                if (eggs_distributed < hive->egg_count) {
                    hive->brood_chamber.cells[i][j].eggs = 1;
                    eggs_distributed++;
                }
            } else {
                // Outer area for honey
                if (honey_distributed < hive->honey_count) {
                    hive->honey_chamber.cells[i][j].honey = random_range(1, 5);
                    honey_distributed++;
                }
            }
        }
    }
    
    hive->state = READY;
    update_beehive_history(hive->id, hive->bee_count, hive->honey_count, hive->egg_count);
}

void* bee_lifecycle(void* arg) {
    BeeThreadArgs* args = (BeeThreadArgs*)arg;
    Bee* bee = args->bee;
    Beehive* hive = args->hive;
    int max_polen = random_range(100, 150);
    
    printf("Iniciando ciclo de vida de abeja %d en colmena %d\n", bee->id, hive->id);
    
    while (bee->is_alive && bee->polen_collected < max_polen) {
        int polen = random_range(1, 5);
        delay_ms(random_range(1, 5));  // Time to collect polen
        
        if (!bee->is_alive) break;  // Check if bee is still alive
        
        pthread_mutex_lock(&hive->chamber_mutex);
        bee->polen_collected += polen;
        pthread_mutex_unlock(&hive->chamber_mutex);
        
        deposit_polen(hive, polen);
    }
    
    pthread_mutex_lock(&hive->chamber_mutex);
    if (bee->is_alive) {  // Solo decrementar si la abeja estaba viva
        bee->is_alive = false;
        if (hive->bee_count > 0) {  // Verificar que no sea negativo
            hive->bee_count--;
            printf("Abeja %d en colmena %d murió. Quedan %d abejas\n", 
                   bee->id, hive->id, hive->bee_count);
        }
    }
    pthread_mutex_unlock(&hive->chamber_mutex);
    
    update_beehive_history(hive->id, hive->bee_count, hive->honey_count, hive->egg_count);
    free(args);
    return NULL;
}

void deposit_polen(Beehive* hive, int polen_amount) {
    pthread_mutex_lock(&hive->chamber_mutex);
    
    // Convert polen to honey (10:1 ratio)
    int honey_produced = polen_amount / POLEN_TO_HONEY_RATIO;
    if (honey_produced > 0) {
        int attempts = 0;
        while (attempts < 100) {
            int x = random_range(0, MAX_CHAMBER_SIZE-1);
            int y = random_range(0, MAX_CHAMBER_SIZE-1);
            
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

void* egg_hatching_thread(void* arg) {
    EggHatchingArgs* args = (EggHatchingArgs*)arg;
    Beehive* hive = args->hive;
    
    // Random time for hatching between 1-10ms
    delay_ms(random_range(1, 10));
    
    pthread_mutex_lock(&hive->chamber_mutex);
    
    // Verificar si hay un huevo en esta celda
    if (hive->brood_chamber.cells[args->cell_x][args->cell_y].eggs > 0) {
        // Decrementar el huevo
        hive->brood_chamber.cells[args->cell_x][args->cell_y].eggs--;
        hive->egg_count--;
        
        // Solo crear nueva abeja si hay espacio
        if (hive->bee_count < MAX_BEES) {
            // Añadir nueva abeja
            hive->bee_count++;
            
            // Realloc para la nueva abeja
            Bee* new_bees = realloc(hive->bees, sizeof(Bee) * hive->bee_count);
            if (new_bees != NULL) {
                hive->bees = new_bees;
                Bee* new_bee = &hive->bees[hive->bee_count - 1];
                new_bee->id = hive->bee_count - 1;
                
                // 5% probabilidad de reina
                int random_val = random_range(1, 100);
                new_bee->type = (random_val <= QUEEN_BIRTH_PROBABILITY) ? QUEEN :
                               (random_val <= 25) ? SCOUT : WORKER;
                new_bee->polen_collected = 0;
                new_bee->max_polen_capacity = random_range(1, 5);
                new_bee->is_alive = true;
                new_bee->hive = hive;
                
                BeeThreadArgs* bee_args = malloc(sizeof(BeeThreadArgs));
                if (bee_args != NULL) {
                    bee_args->bee = new_bee;
                    bee_args->hive = hive;
                    
                    if (pthread_create(&new_bee->thread, NULL, bee_lifecycle, bee_args) == 0) {
                        printf("Nuevo huevo eclosionado en colmena %d. Total abejas: %d, Tipo: %s\n", 
                               hive->id, hive->bee_count,
                               new_bee->type == QUEEN ? "Reina" : 
                               new_bee->type == SCOUT ? "Exploradora" : "Obrera");
                    } else {
                        free(bee_args);
                        hive->bee_count--;
                    }
                }
            }
            
            // Actualizar historial
            update_beehive_history(hive->id, hive->bee_count, hive->honey_count, hive->egg_count);
        }
    }
    
    pthread_mutex_unlock(&hive->chamber_mutex);
    free(args);
    return NULL;
}

void process_egg_hatching(Beehive* hive) {
    printf("Procesando eclosión de huevos en colmena %d (%d huevos, %d abejas)\n",
           hive->id, hive->egg_count, hive->bee_count);
           
    pthread_mutex_lock(&hive->chamber_mutex);
    
    // Verificar cada celda de la cámara de cría
    for (int i = MAX_CHAMBER_SIZE/2-2; i <= MAX_CHAMBER_SIZE/2+1; i++) {
        for (int j = MAX_CHAMBER_SIZE/2-2; j <= MAX_CHAMBER_SIZE/2+1; j++) {
            if (hive->brood_chamber.cells[i][j].eggs > 0) {
                EggHatchingArgs* args = malloc(sizeof(EggHatchingArgs));
                if (args != NULL) {
                    args->hive = hive;
                    args->cell_x = i;
                    args->cell_y = j;
                    
                    pthread_t hatching_thread;
                    if (pthread_create(&hatching_thread, NULL, egg_hatching_thread, args) == 0) {
                        pthread_detach(hatching_thread);
                    } else {
                        free(args);
                    }
                }
            }
        }
    }
    
    pthread_mutex_unlock(&hive->chamber_mutex);
}

bool check_new_queen(Beehive* hive) {
    bool new_queen_found = false;
    
    pthread_mutex_lock(&hive->chamber_mutex);
    
    // Buscar reinas vivas
    for (int i = 0; i < hive->bee_count; i++) {
        if (hive->bees[i].type == QUEEN && hive->bees[i].is_alive) {
            // 5% de probabilidad de crear una nueva colmena
            if (random_range(1, 100) <= QUEEN_BIRTH_PROBABILITY) {
                new_queen_found = true;
                printf("¡Reina encontrada en colmena %d! Iniciando creación de nueva colmena...\n", 
                       hive->id);
                break;
            }
        }
    }
    
    pthread_mutex_unlock(&hive->chamber_mutex);
    return new_queen_found;
}

void cleanup_beehive(Beehive* hive) {
    for (int i = 0; i < hive->bee_count; i++) {
        if (hive->bees[i].is_alive) {
            pthread_cancel(hive->bees[i].thread);
        }
    }
    
    free(hive->bees);
    pthread_mutex_destroy(&hive->chamber_mutex);
    sem_destroy(&hive->resource_sem);
}

void print_beehive_stats(Beehive* hive) {
    pthread_mutex_lock(&hive->chamber_mutex);
    printf("\nColmena #%d Stats:\n", hive->id);
    printf("Abejas: %d\n", hive->bee_count);
    printf("Miel: %d\n", hive->honey_count);
    printf("Huevos: %d\n", hive->egg_count);
    
    // Contar abejas vivas para verificar
    int alive_bees = 0;
    for (int i = 0; i < hive->bee_count; i++) {
        if (hive->bees[i].is_alive) {
            alive_bees++;
        }
    }
    printf("Abejas vivas confirmadas: %d\n", alive_bees);
    
    print_chamber_matrix(hive);
    pthread_mutex_unlock(&hive->chamber_mutex);
}

void print_chamber_matrix(Beehive* hive) {
    printf("\nColmena #%d - Matriz de cámaras:\n", hive->id);
    
    for (int i = 0; i < MAX_CHAMBER_SIZE; i++) {
        for (int j = 0; j < MAX_CHAMBER_SIZE; j++) {
            if (hive->brood_chamber.cells[i][j].eggs > 0) {
                printf("E%d  ", hive->brood_chamber.cells[i][j].eggs);
            } else if (hive->honey_chamber.cells[i][j].honey > 0) {
                printf("M%d  ", hive->honey_chamber.cells[i][j].honey);
            } else {
                if (i >= MAX_CHAMBER_SIZE/2-2 && i <= MAX_CHAMBER_SIZE/2+1 &&
                    j >= MAX_CHAMBER_SIZE/2-2 && j <= MAX_CHAMBER_SIZE/2+1) {
                    printf("E0  ");
                } else {
                    printf("M0  ");
                }
            }
        }
        printf("\n");
    }
    
    printf("\nTotal de miel: %d\n", hive->honey_count);
    printf("Total de huevos: %d\n", hive->egg_count);
    printf("Total de abejas: %d\n", hive->bee_count);
}