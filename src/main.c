#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "../include/types/beehive_types.h"
#include "../include/types/scheduler_types.h"
#include "../include/types/file_manager_types.h"
#include "../include/core/beehive.h"
#include "../include/core/scheduler.h"
#include "../include/core/file_manager.h"
#include "../include/core/utils.h"

// Variables globales
static volatile sig_atomic_t running = 1;// Indicador de que el programa está en ejecución
static ProcessInfo processes[MAX_PROCESSES];// Tabla de procesos (para almacenar la información de cada proceso)

// Manejo de señales
static void handle_signal(int sig) {// Manejar la señal de terminación
    (void)sig;// Ignorar el parámetro
    printf("\nRecibida señal de terminación (Ctrl+C). Finalizando el programa...\n");// Imprimir mensaje de terminación
    running = 0;// Indicar que el programa está en ejecución

    // Detener todos los procesos
    for (int i = 0; i < scheduler_state.process_table->total_processes; i++) {// Recorrer todas las colmenas
        if (processes[i].hive != NULL) {// Comprobar si la colmena está inicializada
            processes[i].hive->should_terminate = 1;// Indicar que se debe detener el hilo del proceso
        }
    }
}

static void setup_signal_handlers(void) {// Configurar los manejadores de señales
    struct sigaction sa;// Estructura de manejador de señales
    sa.sa_handler = handle_signal;// Asignar el manejador de señales
    sigemptyset(&sa.sa_mask);// Inicializar el mask de señales
    sa.sa_flags = 0;// Inicializar los flags de señales

    if (sigaction(SIGINT, &sa, NULL) == -1) {// Comprobar si se pudo configurar el manejador de señales
        perror("Error configurando el manejador de señales");// Imprimir mensaje de error
        exit(1);// Salir del programa
    }
}

// Gestión de procesos
static void init_processes(void) {// Inicializar los procesos
    memset(processes, 0, sizeof(processes));// Inicializar la tabla de procesos

    for (int i = 0; i < INITIAL_BEEHIVES; i++) {// Recorrer todas las colmenas iniciales
        ProcessInfo* process = &processes[i];// Obtener la información del proceso
        process->index = i;// Asignar el índice del proceso
        init_process_semaphores(process);// Inicializar los semáforos del proceso
        init_beehive_process(process, i);// Inicializar el proceso
        add_to_ready_queue(process);// Añadir el proceso a la cola de listos
    }
}

static void cleanup_processes(void) {// Limpiar los procesos
    printf("\nLimpiando todos los procesos...\n");// Imprimir mensaje de limpieza
    for (int i = 0; i < scheduler_state.process_table->total_processes; i++) {// Recorrer todas las colmenas
        if (processes[i].hive != NULL) {// Comprobar si la colmena está inicializada
            printf("├─ Limpiando proceso #%d...\n", i);// Imprimir mensaje de limpieza
            cleanup_beehive_process(&processes[i]);// Limpiar el proceso
            cleanup_process_semaphores(&processes[i]);// Limpiar los semáforos del proceso
        }
    }
}

static void handle_new_process(ProcessInfo* process_info) {// Manejar nuevas colmenas
    if (check_new_queen(process_info) && scheduler_state.process_table->total_processes < MAX_PROCESSES) {// Comprobar si hay una reina y si se puede crear una nueva colmena
        int new_index = -1;// Inicializar el índice de la colmena
        
        // Buscar siguiente índice disponible
        for (int i = 0; i < MAX_PROCESSES; i++) {// Recorrer todas las colmenas
            if (processes[i].hive == NULL) {// Comprobar si la colmena está vacía
                new_index = i;// Asignar el índice de la colmena
                break;// Salir del bucle
            }
        }

        if (new_index != -1) {// Comprobar si se ha encontrado un índice disponible
            ProcessInfo* new_process = &processes[new_index];// Obtener la información del proceso
            new_process->index = new_index;// Asignar el índice del proceso
            init_process_semaphores(new_process);// Inicializar los semáforos del proceso
            init_beehive_process(new_process, new_index);// Inicializar el proceso
            add_to_ready_queue(new_process);// Añadir el proceso a la cola de listos
            scheduler_state.process_table->total_processes++;// Incrementar el número de procesos
            printf("- Total de procesos activos: %d/%d\n\n", scheduler_state.process_table->total_processes, MAX_PROCESSES);// Imprimir el número de procesos activos
        }
    }
}

static void print_ready_queue() {// Imprimir la cola de listos
    printf("\nProcesos en cola de listos: %d\n", scheduler_state.ready_queue->size);// Imprimir el número de procesos en cola de listos
    
    if(scheduler_state.ready_queue->size == 0) {// Comprobar si la cola de listos está vacía
        printf("└─ No hay procesos en cola de listos\n");// Imprimir que no hay procesos en cola de listos
        return;// Salir del bucle
    }

    for (int i = 0; i < scheduler_state.ready_queue->size - 1; i++) {// Recorrer todas las entradas de la cola de listos
        ProcessInfo* process = scheduler_state.ready_queue->processes[i];// Obtener la información del proceso actual (para calcular la posición vacía)
        printf("├─ Proceso #%d: %d abejas, %d miel, %d recursos\n", process->index, process->hive->bee_count, process->hive->honey_count, process->hive->bees_and_honey_count);// Imprimir el mensaje de la cola de listos
    }

    ProcessInfo* process = scheduler_state.ready_queue->processes[scheduler_state.ready_queue->size - 1];// Obtener la información del último proceso en la cola de listos
    printf("└─ Proceso #%d: %d abejas, %d miel, %d recursos\n", process->index, process->hive->bee_count, process->hive->honey_count, process->hive->bees_and_honey_count);// Imprimir el mensaje del último proceso en la cola de listos
}

static void print_io_queue() {// Imprimir la cola de E/S
    printf("\nProcesos en cola de E/S: %d\n", scheduler_state.io_queue->size);// Imprimir el número de procesos en cola de E/S
    
    if(scheduler_state.io_queue->size == 0) {// Comprobar si la cola de E/S está vacía
        printf("└─ No hay procesos en cola de E/S\n");// Imprimir que no hay procesos en cola de E/S
        return;// Salir del bucle
    }

    for (int i = 0; i < scheduler_state.io_queue->size - 1; i++) {// Recorrer todas las entradas de la cola de E/S
        ProcessInfo* process = scheduler_state.io_queue->entries[i].process;// Obtener la información del proceso actual (para calcular la posición vacía)
        printf("├─ Proceso #%d: %d abejas, %d miel, %d recursos\n", process->index, process->hive->bee_count, process->hive->honey_count, process->hive->bees_and_honey_count);// Imprimir el mensaje de la cola de E/S
    }

    ProcessInfo* process = scheduler_state.io_queue->entries[scheduler_state.io_queue->size - 1].process;// Obtener la información del último proceso en la cola de E/S
    printf("└─ Proceso #%d: %d abejas, %d miel, %d recursos\n", process->index, process->hive->bee_count, process->hive->honey_count, process->hive->bees_and_honey_count);// Imprimir el mensaje del último proceso en la cola de E/S
}

// Impresión de información
static void print_scheduler_stats(void) {// Imprimir el estado del planificador
    printf("\n=========== Estado del Planificador ===========\n");// Imprimir el mensaje de estado del planificador
    printf("Política actual: %s\n", scheduler_state.current_policy == ROUND_ROBIN ? "Round Robin" : "Shortest Job First (FSJ)");// Imprimir la política actual del planificador

    if (scheduler_state.current_policy == ROUND_ROBIN) {// Comprobar si la política actual es Round Robin
        printf("Quantum actual: %d segundos\n", scheduler_state.current_quantum);// Imprimir el quantum actual del planificador
    }

    printf("\nProceso en ejecución: %d\n", scheduler_state.active_process->index);// Imprimir el índice del proceso en ejecución
    print_ready_queue();// Imprimir la cola de listos
    print_io_queue();// Imprimir la cola de E/S
    printf("===============================================\n");// Imprimir un salto de línea
}

static void print_initial_state(void) {// Imprimir el estado inicial
    printf("\n=== Simulación de Colmenas Iniciada ===\n");// Imprimir el mensaje de inicio de simulación
    printf("├─ Colmenas iniciales: %d\n", INITIAL_BEEHIVES);// Imprimir el número de colmenas iniciales
    printf("├─ Máximo de colmenas: %d\n", MAX_PROCESSES);// Imprimir el número máximo de colmenas
    printf("├─ Política inicial: %s\n", scheduler_state.current_policy == ROUND_ROBIN ? "Round Robin" : "FSJ");// Imprimir la política inicial
    printf("├─ Quantum inicial: %d segundos\n", scheduler_state.current_quantum);// Imprimir el quantum inicial
    printf("└─ Presione Ctrl+C para finalizar\n\n");// Imprimir un salto de línea
}

// Ciclo principal
static void run_simulation(void) {
    time_t last_stats_time = time(NULL);// Obtener la hora actual (para calcular el tiempo de actualización de estadísticas)

    while (running) {// Mientras no se ha detenido el programa
        time_t current_time = time(NULL);// Obtener la hora actual (para calcular el tiempo de actualización de estadísticas)

        // Imprimir estadísticas cada 5 segundos
        if (difftime(current_time, last_stats_time) >= 5.0) {// Comprobar si se han pasado 5 segundos desde la última actualización de estadísticas
            print_scheduler_stats();// Imprimir el estado del planificador
            last_stats_time = current_time;// Actualizar la hora de la última actualización de estadísticas
        }

        // Verificar nuevas colmenas
        if (scheduler_state.active_process) {// Comprobar si hay un proceso activo
            handle_new_process(scheduler_state.active_process);// Manejar la creación de nuevas colmenas
        }

        // Actualizar archivos de estado
        if (scheduler_state.active_process) {// Comprobar si hay un proceso activo
            update_process_table(scheduler_state.active_process->pcb);// Actualizar el estado del PCB del proceso activo
        }

        // Esperar antes del siguiente ciclo
        delay_ms(1000);// Retrasar el programa por un número de milisegundos
    }
}

int main(void) {
    // Configuración inicial
    srand(time(NULL));// Inicializar el generador de números aleatorios
    setup_signal_handlers();// Configurar los manejadores de señales
    
    // Inicializar componentes
    init_file_manager();// Inicializar el gestor de archivos
    init_scheduler();// Inicializar el planificador
    init_processes();// Inicializar los procesos
    
    // Ejecutar simulación
    print_initial_state();// Imprimir el estado inicial
    run_simulation();// Ejecutar la simulación
    
    // Limpieza
    cleanup_processes();// Limpiar los procesos y sus recursos (PCB y colmenas)
    cleanup_scheduler();// Limpiar el planificador y sus recursos (colas de listos y E/S)
    
    printf("\n=== Simulación Finalizada ===\n");// Imprimir el mensaje de finalización de simulación
    printf("Total de procesos: %d\n", scheduler_state.process_table->total_processes);// Imprimir el número de procesos iniciales
    printf("Recursos liberados correctamente\n\n");// Imprimir un salto de línea
    
    return 0;
}