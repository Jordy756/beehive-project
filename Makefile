# Variables de configuración
CC=gcc
CFLAGS=-Wall -Wextra -I./include/core -I./include/types -pthread
LDFLAGS=-pthread -ljson-c # Esta es la librería que se añadió para los json

# Directorios
SRC_DIR=src #  Directorio de código fuente
OBJ_DIR=obj # Directorio de objetos
BIN_DIR=bin #  Directorio de ejecutables
DATA_DIR=data #  Directorio de datos
INCLUDE_DIR=include #  Directorio de archivos de cabecera
CORE_DIR=$(INCLUDE_DIR)/core # Directorio de archivos de cabecera de core
TYPES_DIR=$(INCLUDE_DIR)/types #  Directorio de archivos de cabecera de types

# Archivos
SRC_FILES=$(wildcard $(SRC_DIR)/*.c) # Archivos de código fuente
OBJ_FILES=$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES)) # Archivos de objetos
EXEC=$(BIN_DIR)/beehive_sim # Nombre del ejecutable

# Colores para mensajes en la terminal
GREEN=\033[0;32m # Color verde
RED=\033[0;31m # Color rojo
YELLOW=\033[1;33m # Color amarillo
NC=\033[0m # Color normal

.PHONY: all clean run directories check-deps # Tareas de utilidad

all: check-deps directories $(EXEC) # Compilación
	@echo "$(GREEN)Compilación completada con éxito$(NC)" 

# Verificación de dependencias
check-deps:
	@echo "$(YELLOW)Verificando dependencias...$(NC)"
	@which $(CC) >/dev/null 2>&1 || (echo "$(RED)Error: GCC no está instalado$(NC)" && exit 1)
	@ldconfig -p | grep libjson-c >/dev/null 2>&1 || (echo "$(RED)Error: libjson-c no está instalada. Por favor, instale usando:$(NC)\nsudo apt-get install libjson-c-dev" && exit 1)
	@echo "$(GREEN)Todas las dependencias están instaladas$(NC)"

# Creación de directorios necesarios
directories:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(DATA_DIR)
	@mkdir -p $(CORE_DIR)
	@mkdir -p $(TYPES_DIR)

# Generación del ejecutable
$(EXEC): directories $(OBJ_FILES)
	@echo "$(YELLOW)Enlazando $@...$(NC)"
	@$(CC) $(OBJ_FILES) -o $@ $(LDFLAGS)
	@echo "$(GREEN)Enlazado completado$(NC)"

# Compilación
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c directories
	@echo "$(YELLOW)Compilando $<...$(NC)"
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "$(GREEN)Compilación de $< exitosa$(NC)"

# Ejecutar el programa
run: all # Ejecución
	./$(EXEC) 

# Limpia archivos generados
clean:
	@echo "$(YELLOW)Limpiando archivos generados...$(NC)"
	@rm -rf $(OBJ_DIR) $(BIN_DIR) $(DATA_DIR)
	@echo "$(GREEN)Limpieza completada$(NC)" 