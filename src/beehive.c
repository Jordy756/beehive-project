#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "../include/core/beehive.h"
#include "../include/core/utils.h"

void init_beehive(Beehive* hive, int id) {
    hive->id = id;
    hive->bee_count = random_range(MIN_BEES, MAX_BEES);
    hive->honey_count = random_range(MIN_HONEY, MAX_HONEY);
    hive->egg_count = random_range(MIN_EGGS, MAX_EGGS);
    
    pthread_mutex_init(&hive->chamber_mutex, NULL);
    sem_init(&hive->resource_sem, 0, 1);
    
    // Initialize bees
    hive->bees = malloc(sizeof(Bee) * hive->bee_count);
    for (int i = 0; i < hive->bee_count; i++) {
        hive->bees[i].id = i;
        hive->bees[i].type = (random_range(1, 100) <= QUEEN_BIRTH_PROBABILITY) ? QUEEN : 
                            (random_range(1, 100) <= 20) ? SCOUT : WORKER;
        hive->bees[i].polen_collected = 0;
        hive->bees[i].max_polen_capacity = random_range(1, 5);
        hive->bees[i].is_alive = true;
        
        // Crear argumentos para el thread
        BeeThreadArgs* args = malloc(sizeof(BeeThreadArgs));
        args->bee = &hive->bees[i];
        args->hive = hive;
        
        pthread_create(&hive->bees[i].thread, NULL, bee_lifecycle, args);
    }
    
    // Initialize chambers
    memset(&hive->honey_chamber, 0, sizeof(Chamber));
    memset(&hive->brood_chamber, 0, sizeof(Chamber));
    
    // Distribute initial honey and eggs in chambers
    int honey_distributed = 0;
    int eggs_distributed = 0;
    
    for (int i = 0; i < MAX_CHAMBER_SIZE && (honey_distributed < hive->honey_count || eggs_distributed < hive->egg_count); i++) {
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
}

void* bee_lifecycle(void* arg) {
    BeeThreadArgs* args = (BeeThreadArgs*)arg;
    Bee* bee = args->bee;
    Beehive* hive = args->hive;
    int max_polen = random_range(100, 150);
    
    while (bee->is_alive && bee->polen_collected < max_polen) {
        int polen = random_range(1, 5);
        delay_ms(random_range(1, 5));  // Time to collect polen
        
        bee->polen_collected += polen;
        deposit_polen(hive, polen);
    }
    
    bee->is_alive = false;
    free(args);  // Liberar memoria de los argumentos
    return NULL;
}

void deposit_polen(Beehive* hive, int polen_amount) {
    pthread_mutex_lock(&hive->chamber_mutex);
    
    // Convert polen to honey (10:1 ratio)
    int honey_produced = polen_amount / POLEN_TO_HONEY_RATIO;
    if (honey_produced > 0) {
        // Find random empty cell
        int attempts = 0;
        while (attempts < 100) {  // Limit attempts to prevent infinite loop
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

void* egg_hatching_thread(void* arg) {
    EggHatchingArgs* args = (EggHatchingArgs*)arg;
    delay_ms(random_range(1, 10));  // Random hatching time
    
    pthread_mutex_lock(&args->hive->chamber_mutex);
    
    if (args->hive->brood_chamber.cells[args->cell_x][args->cell_y].eggs > 0) {
        args->hive->brood_chamber.cells[args->cell_x][args->cell_y].eggs--;
        args->hive->egg_count--;
        args->hive->bee_count++;
        
        // Create new bee
        int new_bee_index = args->hive->bee_count - 1;
        args->hive->bees = realloc(args->hive->bees, sizeof(Bee) * args->hive->bee_count);
        
        Bee* new_bee = &args->hive->bees[new_bee_index];
        new_bee->id = new_bee_index;
        new_bee->type = (random_range(1, 100) <= QUEEN_BIRTH_PROBABILITY) ? QUEEN : 
                       (random_range(1, 100) <= 20) ? SCOUT : WORKER;
        new_bee->polen_collected = 0;
        new_bee->max_polen_capacity = random_range(1, 5);
        new_bee->is_alive = true;
        
        // Crear argumentos para el thread
        BeeThreadArgs* bee_args = malloc(sizeof(BeeThreadArgs));
        bee_args->bee = new_bee;
        bee_args->hive = args->hive;
        
        pthread_create(&new_bee->thread, NULL, bee_lifecycle, bee_args);
    }
    
    pthread_mutex_unlock(&args->hive->chamber_mutex);
    free(args);
    return NULL;
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
                // Print M0 for empty honey cells in honey area, E0 for empty egg cells in egg area
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

bool check_new_queen(Beehive* hive) {
    pthread_mutex_lock(&hive->chamber_mutex);
    
    bool new_queen_found = false;
    
    for (int i = 0; i < hive->bee_count; i++) {
        if (hive->bees[i].type == QUEEN && hive->bees[i].is_alive) {
            if (random_range(1, 100) <= QUEEN_BIRTH_PROBABILITY) {
                new_queen_found = true;
                hive->bees[i].is_alive = false;
                break;
            }
        }
    }
    
    if (!new_queen_found && hive->egg_count > 0) {
        if (random_range(1, 100) <= QUEEN_BIRTH_PROBABILITY) {
            new_queen_found = true;
            hive->egg_count--;
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
    printf("\nColmena #%d, Estadisticas:\n", hive->id);
    printf("Abejas: %d\n", hive->bee_count);
    printf("Miel: %d\n", hive->honey_count);
    printf("Huevos: %d\n", hive->egg_count);
    print_chamber_matrix(hive);
}