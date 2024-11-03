#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "../include/core/beehive.h"
#include "../include/core/utils.h"

// Funciones auxiliares para la estructura hexagonal
static void init_hex_coordinates(Chamber* chamber);
static void connect_hex_cells(Chamber* chamber);
static void distribute_initial_resources(Beehive* hive);

// Funciones auxiliares para la estructura hexagonal
static void init_hex_coordinates(Chamber* chamber) {
    for (int i = 0; i < MAX_CHAMBER_SIZE; i++) {
        for (int j = 0; j < MAX_CHAMBER_SIZE; j++) {
            chamber->cells[i][j].hex_coords.x = i - (j / 2);
            chamber->cells[i][j].hex_coords.y = j;
        }
    }
}

static void connect_hex_cells(Chamber* chamber) {
    for (int i = 0; i < MAX_CHAMBER_SIZE; i++) {
        for (int j = 0; j < MAX_CHAMBER_SIZE; j++) {
            // Configurar conexiones hexagonales
            if (j % 2 == 0) {  // Fila par
                chamber->cells[i][j].adjacent[HEX_TOP_RIGHT] = (i < MAX_CHAMBER_SIZE-1 && j > 0) ? HEX_TOP_RIGHT : -1;
                chamber->cells[i][j].adjacent[HEX_RIGHT] = (i < MAX_CHAMBER_SIZE-1) ? HEX_RIGHT : -1;
                chamber->cells[i][j].adjacent[HEX_BOTTOM_RIGHT] = (i < MAX_CHAMBER_SIZE-1 && j < MAX_CHAMBER_SIZE-1) ? HEX_BOTTOM_RIGHT : -1;
                chamber->cells[i][j].adjacent[HEX_BOTTOM_LEFT] = (i > 0 && j < MAX_CHAMBER_SIZE-1) ? HEX_BOTTOM_LEFT : -1;
                chamber->cells[i][j].adjacent[HEX_LEFT] = (i > 0) ? HEX_LEFT : -1;
                chamber->cells[i][j].adjacent[HEX_TOP_LEFT] = (i > 0 && j > 0) ? HEX_TOP_LEFT : -1;
            } else {  // Fila impar
                chamber->cells[i][j].adjacent[HEX_TOP_RIGHT] = (i < MAX_CHAMBER_SIZE-1 && j > 0) ? HEX_TOP_RIGHT : -1;
                chamber->cells[i][j].adjacent[HEX_RIGHT] = (i < MAX_CHAMBER_SIZE-1) ? HEX_RIGHT : -1;
                chamber->cells[i][j].adjacent[HEX_BOTTOM_RIGHT] = (i < MAX_CHAMBER_SIZE-1 && j < MAX_CHAMBER_SIZE-1) ? HEX_BOTTOM_RIGHT : -1;
                chamber->cells[i][j].adjacent[HEX_BOTTOM_LEFT] = (j < MAX_CHAMBER_SIZE-1) ? HEX_BOTTOM_LEFT : -1;
                chamber->cells[i][j].adjacent[HEX_LEFT] = (i > 0) ? HEX_LEFT : -1;
                chamber->cells[i][j].adjacent[HEX_TOP_LEFT] = (j > 0) ? HEX_TOP_LEFT : -1;
            }
        }
    }
}

static void distribute_initial_resources(Beehive* hive) {
    int honey_distributed = 0;
    int eggs_distributed = 0;
    
    pthread_mutex_lock(&hive->chamber_mutex);
    
    // Distribuir huevos en cámaras de cría
    for (int c = 0; c < hive->chamber_count && eggs_distributed < hive->egg_count; c++) {
        if (hive->chambers[c].is_brood_chamber) {
            for (int i = 0; i < MAX_CHAMBER_SIZE && eggs_distributed < hive->egg_count; i++) {
                for (int j = 0; j < MAX_CHAMBER_SIZE && eggs_distributed < hive->egg_count; j++) {
                    hive->chambers[c].cells[i][j].eggs = 1;
                    eggs_distributed++;
                }
            }
        }
    }
    
    // Distribuir miel en cámaras de miel
    for (int c = 0; c < hive->chamber_count && honey_distributed < hive->honey_count; c++) {
        if (!hive->chambers[c].is_brood_chamber) {
            for (int i = 0; i < MAX_CHAMBER_SIZE && honey_distributed < hive->honey_count; i++) {
                for (int j = 0; j < MAX_CHAMBER_SIZE && honey_distributed < hive->honey_count; j++) {
                    hive->chambers[c].cells[i][j].honey = random_range(1, 5);
                    honey_distributed += hive->chambers[c].cells[i][j].honey;
                }
            }
        }
    }
    
    pthread_mutex_unlock(&hive->chamber_mutex);
}

void init_beehive(Beehive* hive, int id) {
    hive->id = id;
    hive->bee_count = random_range(MIN_BEES, MAX_BEES);
    hive->honey_count = random_range(MIN_HONEY, MAX_HONEY);
    hive->egg_count = random_range(MIN_EGGS, MAX_EGGS);
    hive->chamber_count = 0;  // Inicialmente sin cámaras
    
    pthread_mutex_init(&hive->chamber_mutex, NULL);
    sem_init(&hive->resource_sem, 0, 1);
    
    // Initialize chambers (máximo 20)
    int brood_chambers = MAX_CHAMBERS / 3;  // 1/3 para cría
    int honey_chambers = MAX_CHAMBERS - brood_chambers;  // 2/3 para miel
    
    // Inicializar cámaras de cría
    for (int i = 0; i < brood_chambers && hive->chamber_count < MAX_CHAMBERS; i++) {
        Chamber* chamber = &hive->chambers[hive->chamber_count];
        memset(chamber, 0, sizeof(Chamber));
        chamber->is_brood_chamber = true;
        init_hex_coordinates(chamber);
        connect_hex_cells(chamber);
        hive->chamber_count++;
    }
    
    // Inicializar cámaras de miel
    for (int i = 0; i < honey_chambers && hive->chamber_count < MAX_CHAMBERS; i++) {
        Chamber* chamber = &hive->chambers[hive->chamber_count];
        memset(chamber, 0, sizeof(Chamber));
        chamber->is_brood_chamber = false;
        init_hex_coordinates(chamber);
        connect_hex_cells(chamber);
        hive->chamber_count++;
    }
    
    // Initialize bees with specific roles
    hive->bees = malloc(sizeof(Bee) * hive->bee_count);
    bool has_queen = false;
    
    for (int i = 0; i < hive->bee_count; i++) {
        hive->bees[i].id = i;
        
        // Asignar roles
        if (!has_queen) {
            hive->bees[i].role.type = QUEEN;
            has_queen = true;
        } else {
            hive->bees[i].role.type = (random_range(1, 100) <= 20) ? SCOUT : WORKER;
        }
        
        // Inicializar datos específicos del rol
        switch (hive->bees[i].role.type) {
            case QUEEN:
                hive->bees[i].role.role_data.queen.eggs_laid = 0;
                hive->bees[i].role.role_data.queen.last_egg_time = time(NULL);
                break;
            case WORKER:
                hive->bees[i].role.role_data.worker.honey_produced = 0;
                hive->bees[i].role.role_data.worker.polen_carried = 0;
                break;
            case SCOUT:
                hive->bees[i].role.role_data.scout.found_food_source = false;
                hive->bees[i].role.role_data.scout.discovered_locations = 0;
                break;
        }
        
        hive->bees[i].polen_collected = 0;
        hive->bees[i].max_polen_capacity = random_range(1, 5);
        hive->bees[i].is_alive = true;
        hive->bees[i].hive = hive;
        
        BeeThreadArgs* args = malloc(sizeof(BeeThreadArgs));
        args->bee = &hive->bees[i];
        args->hive = hive;
        
        pthread_create(&hive->bees[i].thread, NULL, bee_lifecycle, args);
    }
    
    // Distribuir huevos y miel inicial en las cámaras
    distribute_initial_resources(hive);
    
    hive->state = READY;
}

// Implementación de comportamientos específicos
void queen_behavior(Bee* bee) {
    Beehive* hive = bee->hive;
    time_t current_time = time(NULL);
    
    // La reina pone huevos cada cierto tiempo
    if (difftime(current_time, bee->role.role_data.queen.last_egg_time) >= 5.0) {  // 5 segundos entre huevos
        pthread_mutex_lock(&hive->chamber_mutex);
        
        // Buscar una celda vacía en una cámara de cría
        for (int c = 0; c < hive->chamber_count; c++) {
            if (hive->chambers[c].is_brood_chamber) {
                for (int i = 0; i < MAX_CHAMBER_SIZE; i++) {
                    for (int j = 0; j < MAX_CHAMBER_SIZE; j++) {
                        if (hive->chambers[c].cells[i][j].eggs == 0) {
                            hive->chambers[c].cells[i][j].eggs = 1;
                            hive->egg_count++;
                            bee->role.role_data.queen.eggs_laid++;
                            bee->role.role_data.queen.last_egg_time = current_time;
                            
                            // Crear thread para eclosión
                            EggHatchingArgs* args = malloc(sizeof(EggHatchingArgs));
                            args->hive = hive;
                            args->chamber_index = c;
                            args->cell_x = i;
                            args->cell_y = j;
                            
                            pthread_t hatching_thread;
                            pthread_create(&hatching_thread, NULL, egg_hatching_thread, args);
                            pthread_detach(hatching_thread);
                            
                            pthread_mutex_unlock(&hive->chamber_mutex);
                            return;
                        }
                    }
                }
            }
        }
        
        pthread_mutex_unlock(&hive->chamber_mutex);
    }
}

void worker_behavior(Bee* bee) {
    if (bee->polen_collected < bee->max_polen_capacity) {
        int polen = random_range(1, 5);
        delay_ms(random_range(1, 5));  // Tiempo recolectando polen
        
        bee->polen_collected += polen;
        bee->role.role_data.worker.polen_carried += polen;
        
        if (bee->polen_collected >= bee->max_polen_capacity) {
            deposit_polen(bee->hive, bee->polen_collected);
            bee->role.role_data.worker.honey_produced += bee->polen_collected / POLEN_TO_HONEY_RATIO;
            bee->polen_collected = 0;
        }
    }
}

void scout_behavior(Bee* bee) {
    // Los exploradores tienen más probabilidad de encontrar polen
    if (!bee->role.role_data.scout.found_food_source) {
        delay_ms(random_range(1, 3));  // Más rápidos buscando
        
        if (random_range(1, 100) <= 30) {  // 30% de probabilidad de encontrar fuente
            bee->role.role_data.scout.found_food_source = true;
            bee->role.role_data.scout.discovered_locations++;
            
            // Al encontrar una fuente, pueden recolectar más polen
            int polen = random_range(3, 7);  // Más eficientes que las obreras
            bee->polen_collected += polen;
            
            if (bee->polen_collected >= bee->max_polen_capacity) {
                deposit_polen(bee->hive, bee->polen_collected);
                bee->polen_collected = 0;
                bee->role.role_data.scout.found_food_source = false;  // Buscar nueva fuente
            }
        }
    }
}

void* bee_lifecycle(void* arg) {
    BeeThreadArgs* args = (BeeThreadArgs*)arg;
    Bee* bee = args->bee;
    int max_polen = random_range(100, 150);
    
    while (bee->is_alive && bee->polen_collected < max_polen) {
        switch (bee->role.type) {
            case QUEEN:
                queen_behavior(bee);
                break;
            case WORKER:
                worker_behavior(bee);
                break;
            case SCOUT:
                scout_behavior(bee);
                break;
        }
        
        delay_ms(random_range(1, 5));
    }
    
    bee->is_alive = false;
    free(args);
    return NULL;
}

void deposit_polen(Beehive* hive, int polen_amount) {
    pthread_mutex_lock(&hive->chamber_mutex);
    
    // Convertir polen a miel (10:1 ratio)
    int honey_produced = polen_amount / POLEN_TO_HONEY_RATIO;
    
    if (honey_produced > 0) {
        // Buscar una cámara de miel con espacio disponible
        for (int c = 0; c < hive->chamber_count; c++) {
            if (!hive->chambers[c].is_brood_chamber) {
                for (int i = 0; i < MAX_CHAMBER_SIZE; i++) {
                    for (int j = 0; j < MAX_CHAMBER_SIZE; j++) {
                        if (hive->chambers[c].cells[i][j].honey < 10) {  // Límite por celda
                            hive->chambers[c].cells[i][j].honey += honey_produced;
                            hive->honey_count += honey_produced;
                            pthread_mutex_unlock(&hive->chamber_mutex);
                            return;
                        }
                    }
                }
            }
        }
    }
    
    pthread_mutex_unlock(&hive->chamber_mutex);
}

void* egg_hatching_thread(void* arg) {
    EggHatchingArgs* args = (EggHatchingArgs*)arg;
    delay_ms(random_range(1, 10));  // Tiempo de eclosión
    
    pthread_mutex_lock(&args->hive->chamber_mutex);
    
    if (args->hive->chambers[args->chamber_index].cells[args->cell_x][args->cell_y].eggs > 0) {
        args->hive->chambers[args->chamber_index].cells[args->cell_x][args->cell_y].eggs--;
        args->hive->egg_count--;
        args->hive->bee_count++;
        
        // Crear nueva abeja
        int new_bee_index = args->hive->bee_count - 1;
        args->hive->bees = realloc(args->hive->bees, sizeof(Bee) * args->hive->bee_count);
        
        Bee* new_bee = &args->hive->bees[new_bee_index];
        new_bee->id = new_bee_index;
        
        // Asignar rol (nunca reina de huevo)
        new_bee->role.type = (random_range(1, 100) <= 20) ? SCOUT : WORKER;
        
        // Inicializar datos del rol
        switch (new_bee->role.type) {
            case WORKER:
                new_bee->role.role_data.worker.honey_produced = 0;
                new_bee->role.role_data.worker.polen_carried = 0;
                break;
            case SCOUT:
                new_bee->role.role_data.scout.found_food_source = false;
                new_bee->role.role_data.scout.discovered_locations = 0;
                break;
            default:
                break;
        }
        
        new_bee->polen_collected = 0;
        new_bee->max_polen_capacity = random_range(1, 5);
        new_bee->is_alive = true;
        new_bee->hive = args->hive;
        
        BeeThreadArgs* bee_args = malloc(sizeof(BeeThreadArgs));
        bee_args->bee = new_bee;
        bee_args->hive = args->hive;
        
        pthread_create(&new_bee->thread, NULL, bee_lifecycle, bee_args);
    }
    
    pthread_mutex_unlock(&args->hive->chamber_mutex);
    free(args);
    return NULL;
}

void process_honey_production(Beehive* hive) {
    pthread_mutex_lock(&hive->chamber_mutex);
    // La producción de miel se maneja ahora en deposit_polen y los comportamientos específicos
    pthread_mutex_unlock(&hive->chamber_mutex);
}

void process_egg_hatching(Beehive* hive) {
    pthread_mutex_lock(&hive->chamber_mutex);
    
    for (int c = 0; c < hive->chamber_count; c++) {
        if (hive->chambers[c].is_brood_chamber) {
            for (int i = 0; i < MAX_CHAMBER_SIZE; i++) {
                for (int j = 0; j < MAX_CHAMBER_SIZE; j++) {
                    if (hive->chambers[c].cells[i][j].eggs > 0) {
                        EggHatchingArgs* args = malloc(sizeof(EggHatchingArgs));
                        args->hive = hive;
                        args->chamber_index = c;
                        args->cell_x = i;
                        args->cell_y = j;
                        
                        pthread_t hatching_thread;
                        pthread_create(&hatching_thread, NULL, egg_hatching_thread, args);
                        pthread_detach(hatching_thread);
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
    
    // Verificar reinas existentes
    for (int i = 0; i < hive->bee_count; i++) {
        if (hive->bees[i].role.type == QUEEN && hive->bees[i].is_alive) {
            if (random_range(1, 100) <= QUEEN_BIRTH_PROBABILITY) {
                new_queen_found = true;
                hive->bees[i].is_alive = false;  // La reina vieja muere
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

void print_chamber_matrix(Beehive* hive) {
    printf("\nColmena #%d - Estado de las cámaras:\n", hive->id);
    printf("Total de cámaras: %d\n", hive->chamber_count);
    
    for (int c = 0; c < hive->chamber_count; c++) {
        printf("\nCámara %d (%s):\n", c, hive->chambers[c].is_brood_chamber ? "Cría" : "Miel");
        
        for (int i = 0; i < MAX_CHAMBER_SIZE; i++) {
            // Añadir sangría para estructura hexagonal
            if (i % 2 == 1) printf(" ");
            
            for (int j = 0; j < MAX_CHAMBER_SIZE; j++) {
                if (hive->chambers[c].is_brood_chamber) {
                    printf("E%d  ", hive->chambers[c].cells[i][j].eggs);
                } else {
                    printf("M%d  ", hive->chambers[c].cells[i][j].honey);
                }
            }
            printf("\n");
        }
    }
}

void print_beehive_stats(Beehive* hive) {
    printf("\nEstadísticas de la Colmena #%d:\n", hive->id);
    printf("Número total de abejas: %d\n", hive->bee_count);
    printf("Desglose por tipo:\n");
    
    int queens = 0, workers = 0, scouts = 0;
    for (int i = 0; i < hive->bee_count; i++) {
        if (hive->bees[i].is_alive) {
            switch (hive->bees[i].role.type) {
                case QUEEN: queens++; break;
                case WORKER: workers++; break;
                case SCOUT: scouts++; break;
            }
        }
    }
    
    printf("- Reinas: %d\n", queens);
    printf("- Obreras: %d\n", workers);
    printf("- Exploradoras: %d\n", scouts);
    printf("Miel total: %d\n", hive->honey_count);
    printf("Huevos totales: %d\n", hive->egg_count);
    printf("Número de cámaras: %d\n", hive->chamber_count);
    
    print_chamber_matrix(hive);
}