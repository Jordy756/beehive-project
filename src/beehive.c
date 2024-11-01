#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "../include/core/beehive.h"
#include "../include/core/utils.h"

void init_beehive(Beehive* hive, int id) {
    hive->id = id;
    // Asegurar que los valores iniciales estén dentro de los rangos requeridos
    hive->bee_count = random_range(MIN_BEES, MAX_BEES);
    hive->honey_count = random_range(MIN_HONEY, MAX_HONEY);
    hive->egg_count = random_range(MIN_EGGS, MAX_EGGS);
    
    pthread_mutex_init(&hive->chamber_mutex, NULL);
    sem_init(&hive->resource_sem, 0, 1);
    
    // Initialize bees
    hive->bees = malloc(sizeof(Bee) * hive->bee_count);
    if (hive->bees == NULL) {
        log_message("Error: Failed to allocate memory for bees");
        return;
    }

    char message[100];
    snprintf(message, sizeof(message), 
            "Initializing beehive %d with %d bees, %d honey, %d eggs",
            id, hive->bee_count, hive->honey_count, hive->egg_count);
    log_message(message);
    
    for (int i = 0; i < hive->bee_count; i++) {
        hive->bees[i].id = i;
        // Probabilidad del 5% para reina, 20% para exploradoras, resto obreras
        int random_val = random_range(1, 100);
        hive->bees[i].type = (random_val <= QUEEN_BIRTH_PROBABILITY) ? QUEEN :
                            (random_val <= 20) ? SCOUT : WORKER;
        hive->bees[i].polen_collected = 0;
        hive->bees[i].max_polen_capacity = random_range(1, 5);
        hive->bees[i].is_alive = true;
        
        BeeThreadArgs* args = malloc(sizeof(BeeThreadArgs));
        if (args == NULL) {
            log_message("Error: Failed to allocate memory for bee args");
            continue;
        }
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
    int max_polen = random_range(100, 150);  // Límite de polen para la vida de la abeja
    
    while (bee->is_alive && bee->polen_collected < max_polen) {
        int polen_capacity = random_range(1, 5);  // Capacidad de transporte de polen
        delay_ms(random_range(1, 5));  // Tiempo de recolección
        
        if (!bee->is_alive) break;  // Verificar si la abeja sigue viva
        
        bee->polen_collected += polen_capacity;
        deposit_polen(hive, polen_capacity);
        
        // Log de actividad de la abeja
        char message[100];
        snprintf(message, sizeof(message), 
                "Bee %d collected %d polen (total: %d/%d)",
                bee->id, polen_capacity, bee->polen_collected, max_polen);
        log_message(message);
    }
    
    pthread_mutex_lock(&hive->chamber_mutex);
    bee->is_alive = false;
    hive->bee_count--;
    pthread_mutex_unlock(&hive->chamber_mutex);
    
    update_beehive_history(hive->id, hive->bee_count, hive->honey_count, hive->egg_count);
    free(args);
    return NULL;
}

void deposit_polen(Beehive* hive, int polen_amount) {
    pthread_mutex_lock(&hive->chamber_mutex);
    
    // Convertir polen a miel (ratio 10:1)
    int honey_produced = polen_amount / POLEN_TO_HONEY_RATIO;
    if (honey_produced > 0) {
        // Encontrar una celda aleatoria para la miel
        bool honey_deposited = false;
        int attempts = 0;
        while (!honey_deposited && attempts < 100) {
            int x = random_range(0, MAX_CHAMBER_SIZE-1);
            int y = random_range(0, MAX_CHAMBER_SIZE-1);
            
            // Evitar área central reservada para huevos
            if (!(x >= MAX_CHAMBER_SIZE/2-2 && x <= MAX_CHAMBER_SIZE/2+1 &&
                  y >= MAX_CHAMBER_SIZE/2-2 && y <= MAX_CHAMBER_SIZE/2+1)) {
                hive->honey_chamber.cells[x][y].honey += honey_produced;
                hive->honey_count += honey_produced;
                honey_deposited = true;
                
                char message[100];
                snprintf(message, sizeof(message), 
                        "Honey produced: %d, Total honey: %d",
                        honey_produced, hive->honey_count);
                log_message(message);
            }
            attempts++;
        }
    }
    
    pthread_mutex_unlock(&hive->chamber_mutex);
    update_beehive_history(hive->id, hive->bee_count, hive->honey_count, hive->egg_count);
}

void* egg_hatching_thread(void* arg) {
    EggHatchingArgs* args = (EggHatchingArgs*)arg;
    
    // Tiempo aleatorio para la eclosión
    delay_ms(random_range(1, 10));
    
    pthread_mutex_lock(&args->hive->chamber_mutex);
    
    if (args->hive->brood_chamber.cells[args->cell_x][args->cell_y].eggs > 0) {
        args->hive->brood_chamber.cells[args->cell_x][args->cell_y].eggs--;
        args->hive->egg_count--;
        args->hive->bee_count++;
        
        // Crear nueva abeja
        int new_bee_index = args->hive->bee_count - 1;
        Bee* new_bees = realloc(args->hive->bees, sizeof(Bee) * args->hive->bee_count);
        
        if (new_bees != NULL) {
            args->hive->bees = new_bees;
            Bee* new_bee = &args->hive->bees[new_bee_index];
            new_bee->id = new_bee_index;
            
            // 5% probabilidad de reina, 20% exploradoras
            int random_val = random_range(1, 100);
            new_bee->type = (random_val <= QUEEN_BIRTH_PROBABILITY) ? QUEEN :
                           (random_val <= 20) ? SCOUT : WORKER;
            new_bee->polen_collected = 0;
            new_bee->max_polen_capacity = random_range(1, 5);
            new_bee->is_alive = true;
            
            BeeThreadArgs* bee_args = malloc(sizeof(BeeThreadArgs));
            if (bee_args != NULL) {
                bee_args->bee = new_bee;
                bee_args->hive = args->hive;
                pthread_create(&new_bee->thread, NULL, bee_lifecycle, bee_args);
                
                char message[100];
                snprintf(message, sizeof(message), 
                        "New bee hatched (type: %s) in hive %d",
                        new_bee->type == QUEEN ? "Queen" : 
                        new_bee->type == SCOUT ? "Scout" : "Worker",
                        args->hive->id);
                log_message(message);
            }
        }
        
        update_beehive_history(args->hive->id, args->hive->bee_count, 
                             args->hive->honey_count, args->hive->egg_count);
    }
    
    pthread_mutex_unlock(&args->hive->chamber_mutex);
    free(args);
    return NULL;
}

void process_egg_hatching(Beehive* hive) {
    pthread_mutex_lock(&hive->chamber_mutex);
    
    for (int i = 0; i < MAX_CHAMBER_SIZE; i++) {
        for (int j = 0; j < MAX_CHAMBER_SIZE; j++) {
            if (hive->brood_chamber.cells[i][j].eggs > 0) {
                EggHatchingArgs* args = malloc(sizeof(EggHatchingArgs));
                args->hive = hive;
                args->cell_x = i;
                args->cell_y = j;
                
                pthread_t hatching_thread;
                pthread_create(&hatching_thread, NULL, egg_hatching_thread, args);
                pthread_detach(hatching_thread);
            }
        }
    }
    
    pthread_mutex_unlock(&hive->chamber_mutex);
}

bool check_new_queen(Beehive* hive) {
    pthread_mutex_lock(&hive->chamber_mutex);
    
    bool new_queen_found = false;
    
    // Revisar abejas existentes
    for (int i = 0; i < hive->bee_count; i++) {
        if (hive->bees[i].type == QUEEN && hive->bees[i].is_alive) {
            if (random_range(1, 100) <= QUEEN_BIRTH_PROBABILITY) {
                new_queen_found = true;
                log_message("New queen detected! Creating new hive...");
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
    printf("\nColmena #%d Stats:\n", hive->id);
    printf("Abejas: %d\n", hive->bee_count);
    printf("Miel: %d\n", hive->honey_count);
    printf("Huevos: %d\n", hive->egg_count);
    
    // Obtener estadísticas históricas
    int total_bees, total_honey, total_eggs;
    get_beehive_stats(hive->id, &total_bees, &total_honey, &total_eggs);
    
    printf("\nEstadísticas históricas:\n");
    printf("Total de abejas producidas: %d\n", total_bees);
    printf("Total de miel acumulada: %d\n", total_honey);
    printf("Total de huevos producidos: %d\n", total_eggs);
    
    print_chamber_matrix(hive);
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