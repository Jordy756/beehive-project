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
    
    // Distribuir huevos solo en cámaras de cría
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
    
    // Distribuir miel solo en cámaras de miel, de la misma manera que los huevos
    for (int c = 0; c < hive->chamber_count && honey_distributed < hive->honey_count; c++) {
        if (!hive->chambers[c].is_brood_chamber) {
            for (int i = 0; i < MAX_CHAMBER_SIZE && honey_distributed < hive->honey_count; i++) {
                for (int j = 0; j < MAX_CHAMBER_SIZE && honey_distributed < hive->honey_count; j++) {
                    hive->chambers[c].cells[i][j].honey = 1;
                    honey_distributed++;
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
    hive->chamber_count = MAX_CHAMBERS;  // Inicializamos con 20 cámaras
    
    pthread_mutex_init(&hive->chamber_mutex, NULL);
    sem_init(&hive->resource_sem, 0, 1);
    
    // Inicializar todas las cámaras
    // Inicializar todas las cámaras
    for (int i = 0; i < MAX_CHAMBERS; i++) {
        memset(&hive->chambers[i], 0, sizeof(Chamber));
        
        // Determinar si esta cámara debe ser de cría o miel
        // Para una matriz de 4x5 cámaras
        int row = i / 5;    // Fila en la matriz de 4x5 (ahora usamos 5 columnas)
        int col = i % 5;    // Columna en la matriz de 4x5
        
        // Según el patrón:
        // Fila 0: todas miel
        // Fila 1 y 2: miel en los extremos (col 0 y 4), huevos en el medio (col 1,2,3)
        // Fila 3: todas miel
        bool is_brood = (row == 1 || row == 2) && (col >= 1 && col <= 3);
        
        hive->chambers[i].is_brood_chamber = is_brood;
        
        init_hex_coordinates(&hive->chambers[i]);
        connect_hex_cells(&hive->chambers[i]);
    }
    
    // El resto del código de init_beehive se mantiene igual...
    
    // Inicializar bees con roles específicos
    hive->bees = malloc(sizeof(Bee) * hive->bee_count);
    bool has_queen = false;
    
    for (int i = 0; i < hive->bee_count; i++) {
        hive->bees[i].id = i;
        
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
    
    // Distribuir recursos iniciales
    distribute_initial_resources(hive);
    
    hive->state = READY;
}

// Implementación de comportamientos específicos
void queen_behavior(Bee* bee) {
    Beehive* hive = bee->hive;
    time_t current_time = time(NULL);
    
    // La reina pone huevos cada cierto tiempo
    if (difftime(current_time, bee->role.role_data.queen.last_egg_time) >= EGG_LAYING_INTERVAL) {
        pthread_mutex_lock(&hive->chamber_mutex);
        
        // Verificar límite de huevos
        if (can_add_eggs(hive, 1)) {
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
                                
                                printf("Huevo puesto en colmena %d\n", hive->id);
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
}

void worker_behavior(Bee* bee) {
    if (bee->polen_collected < bee->max_polen_capacity) {
        int polen = random_range(1, 5);
        delay_ms(random_range(100, 500));  // Tiempo recolectando polen
        
        bee->polen_collected += polen;
        bee->role.role_data.worker.polen_carried += polen;
        
        if (bee->polen_collected >= bee->max_polen_capacity) {
            deposit_polen(bee->hive, bee->polen_collected);
            bee->role.role_data.worker.honey_produced += bee->polen_collected / POLEN_TO_HONEY_RATIO;
            printf("Miel producida en colmena %d: %d unidades\n", bee->hive->id, 
                   bee->polen_collected / POLEN_TO_HONEY_RATIO);
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
    Beehive* hive = args->hive;
    
    // Determinar la cantidad máxima de polen que recolectará en su vida
    int max_polen_lifetime = random_range(100, 150);
    int total_polen_collected = 0;  // Para tracking del polen total en su vida
    
    while (bee->is_alive && total_polen_collected < max_polen_lifetime) {
        switch (bee->role.type) {
            case QUEEN:
                queen_behavior(bee);
                break;
            case WORKER:
                worker_behavior(bee);
                if (bee->polen_collected > 0) {
                    total_polen_collected += bee->polen_collected;
                    printf("Abeja %d de colmena %d: polen total recolectado %d/%d\n", 
                           bee->id, hive->id, total_polen_collected, max_polen_lifetime);
                }
                break;
            case SCOUT:
                scout_behavior(bee);
                if (bee->polen_collected > 0) {
                    total_polen_collected += bee->polen_collected;
                    printf("Scout %d de colmena %d: polen total recolectado %d/%d\n", 
                           bee->id, hive->id, total_polen_collected, max_polen_lifetime);
                }
                break;
        }
        
        delay_ms(random_range(100, 500));
    }
    
    // La abeja muere después de alcanzar su límite de polen
    pthread_mutex_lock(&hive->chamber_mutex);
    bee->is_alive = false;
    hive->bee_count--;
    printf("Abeja %d de colmena %d murió después de recolectar %d unidades de polen\n", 
           bee->id, hive->id, total_polen_collected);
    pthread_mutex_unlock(&hive->chamber_mutex);
    
    free(args);
    return NULL;
}

void deposit_polen(Beehive* hive, int polen_amount) {
    pthread_mutex_lock(&hive->chamber_mutex);
    
    // Convertir polen a miel (10:1 ratio)
    int honey_produced = polen_amount / POLEN_TO_HONEY_RATIO;
    
    if (honey_produced > 0 && can_add_honey(hive, honey_produced)) {
        // Buscar una cámara de miel con espacio disponible
        for (int c = 0; c < hive->chamber_count; c++) {
            if (!hive->chambers[c].is_brood_chamber) {
                for (int i = 0; i < MAX_CHAMBER_SIZE; i++) {
                    for (int j = 0; j < MAX_CHAMBER_SIZE; j++) {
                        if (hive->chambers[c].cells[i][j].honey < 10) {  // Límite por celda
                            int space_available = 10 - hive->chambers[c].cells[i][j].honey;
                            int honey_to_add = min(honey_produced, space_available);
                            
                            hive->chambers[c].cells[i][j].honey += honey_to_add;
                            hive->honey_count += honey_to_add;
                            honey_produced -= honey_to_add;
                            
                            if (honey_produced <= 0) {
                                pthread_mutex_unlock(&hive->chamber_mutex);
                                return;
                            }
                        }
                    }
                }
            }
        }
    }
    
    pthread_mutex_unlock(&hive->chamber_mutex);
}

bool can_add_eggs(Beehive* hive, int amount) {
    return (hive->egg_count + amount) <= MAX_TOTAL_EGGS;
}

bool can_add_honey(Beehive* hive, int amount) {
    return (hive->honey_count + amount) <= MAX_TOTAL_HONEY;
}

void update_beehive_resources(Beehive* hive) {
    pthread_mutex_lock(&hive->chamber_mutex);
    
    // Actualizar tiempo de última actividad
    hive->last_activity_time = time(NULL);
    
    // Verificar y ajustar recursos si exceden los límites
    if (hive->egg_count > MAX_TOTAL_EGGS) {
        hive->egg_count = MAX_TOTAL_EGGS;
    }
    
    if (hive->honey_count > MAX_TOTAL_HONEY) {
        hive->honey_count = MAX_TOTAL_HONEY;
    }
    
    pthread_mutex_unlock(&hive->chamber_mutex);
}

void* egg_hatching_thread(void* arg) {
    EggHatchingArgs* args = (EggHatchingArgs*)arg;
    delay_ms(random_range(1000, 10000));  // Tiempo de eclosión entre 1 y 10 segundos
    
    pthread_mutex_lock(&args->hive->chamber_mutex);
    
    if (args->hive->chambers[args->chamber_index].cells[args->cell_x][args->cell_y].eggs > 0) {
        args->hive->chambers[args->chamber_index].cells[args->cell_x][args->cell_y].eggs--;
        args->hive->egg_count--;
        
        // Crear nueva abeja
        Bee* new_bees = realloc(args->hive->bees, sizeof(Bee) * (args->hive->bee_count + 1));
        if (new_bees != NULL) {
            args->hive->bees = new_bees;
            Bee* new_bee = &args->hive->bees[args->hive->bee_count];
            
            new_bee->id = args->hive->bee_count;
            new_bee->role.type = (random_range(1, 100) <= 20) ? SCOUT : WORKER;
            new_bee->polen_collected = 0;
            new_bee->max_polen_capacity = random_range(1, 5);
            new_bee->is_alive = true;
            new_bee->hive = args->hive;
            new_bee->creation_time = time(NULL);
            
            // Inicializar datos específicos del rol
            switch (new_bee->role.type) {
                case WORKER:
                    new_bee->role.role_data.worker.honey_produced = 0;
                    new_bee->role.role_data.worker.polen_carried = 0;
                    new_bee->role.role_data.worker.last_collection_time = time(NULL);
                    break;
                case SCOUT:
                    new_bee->role.role_data.scout.found_food_source = false;
                    new_bee->role.role_data.scout.discovered_locations = 0;
                    new_bee->role.role_data.scout.last_search_time = time(NULL);
                    break;
                default:
                    break;
            }
            
            // Crear y ejecutar el thread para la nueva abeja
            BeeThreadArgs* bee_args = malloc(sizeof(BeeThreadArgs));
            bee_args->bee = new_bee;
            bee_args->hive = args->hive;
            
            pthread_create(&new_bee->thread, NULL, bee_lifecycle, bee_args);
            args->hive->bee_count++;
            
            printf("Nueva abeja nacida en colmena %d (total: %d)\n", 
                   args->hive->id, args->hive->bee_count);
        }
    }
    
    pthread_mutex_unlock(&args->hive->chamber_mutex);
    free(args);
    return NULL;
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
    printf("Cámaras: %d\n\n", hive->chamber_count);

    const int ROWS = 4;
    const int COLS = 5;
    
    // Iterar sobre cada fila de la matriz 10x10
    for (int chamber_row = 0; chamber_row < ROWS; chamber_row++) {
        // Imprimir cada fila de celdas para todas las cámaras en esta fila
        for (int cell_row = 0; cell_row < MAX_CHAMBER_SIZE; cell_row++) {
            // Iterar sobre cada cámara en esta fila
            for (int chamber_col = 0; chamber_col < COLS; chamber_col++) {
                int chamber_index = chamber_row * COLS + chamber_col;
                
                // Imprimir la fila de celdas para esta cámara
                for (int cell_col = 0; cell_col < MAX_CHAMBER_SIZE; cell_col++) {
                    if (hive->chambers[chamber_index].is_brood_chamber) {
                        printf("H%-2d ", hive->chambers[chamber_index].cells[cell_row][cell_col].eggs);
                    } else {
                        printf("M%-2d ", hive->chambers[chamber_index].cells[cell_row][cell_col].honey);
                    }
                }
                printf("    ");  // Espacio entre cámaras
            }
            printf("\n");  // Nueva línea después de cada fila de celdas
        }
        printf("\n");  // Espacio entre filas de cámaras
    }

    // Imprimir totales
    printf("\nTotales por cámara:\n");
    for (int i = 0; i < hive->chamber_count; i++) {
        if (hive->chambers[i].is_brood_chamber) {
            printf("Cámara %d (Cría): ", i);
            int total_eggs = 0;
            for (int r = 0; r < MAX_CHAMBER_SIZE; r++) {
                for (int c = 0; c < MAX_CHAMBER_SIZE; c++) {
                    total_eggs += hive->chambers[i].cells[r][c].eggs;
                }
            }
            printf("%d huevos\n", total_eggs);
        } else {
            printf("Cámara %d (Miel): ", i);
            int total_honey = 0;
            for (int r = 0; r < MAX_CHAMBER_SIZE; r++) {
                for (int c = 0; c < MAX_CHAMBER_SIZE; c++) {
                    total_honey += hive->chambers[i].cells[r][c].honey;
                }
            }
            printf("%d miel\n", total_honey);
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
    
    print_chamber_matrix(hive);
}