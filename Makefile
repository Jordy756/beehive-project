CC=gcc
CFLAGS=-Wall -Wextra -I./include/core -I./include/types -pthread
LDFLAGS=-pthread -ljson-c

# Directorios
SRC_DIR=src
OBJ_DIR=obj
BIN_DIR=bin
DATA_DIR=data
INCLUDE_DIR=include
CORE_DIR=$(INCLUDE_DIR)/core
TYPES_DIR=$(INCLUDE_DIR)/types

# Archivos fuente y objeto
SRC_FILES=$(wildcard $(SRC_DIR)/*.c)
OBJ_FILES=$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))
EXEC=$(BIN_DIR)/beehive_sim

# Colores para mensajes
GREEN=\033[0;32m
RED=\033[0;31m
YELLOW=\033[1;33m
NC=\033[0m

.PHONY: all clean run directories check-deps

all: check-deps directories $(EXEC)
	@echo "$(GREEN)Compilación completada con éxito$(NC)"

# Verificar dependencias
check-deps:
	@echo "$(YELLOW)Verificando dependencias...$(NC)"
	@which $(CC) >/dev/null 2>&1 || (echo "$(RED)Error: GCC no está instalado$(NC)" && exit 1)
	@ldconfig -p | grep libjson-c >/dev/null 2>&1 || (echo "$(RED)Error: libjson-c no está instalada. Por favor, instale usando:$(NC)\nsudo apt-get install libjson-c-dev" && exit 1)
	@echo "$(GREEN)Todas las dependencias están instaladas$(NC)"

# Crear directorios necesarios
directories:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(DATA_DIR)
	@mkdir -p $(CORE_DIR)
	@mkdir -p $(TYPES_DIR)

$(EXEC): directories $(OBJ_FILES)
	@echo "$(YELLOW)Enlazando $@...$(NC)"
	@$(CC) $(OBJ_FILES) -o $@ $(LDFLAGS)
	@echo "$(GREEN)Enlazado completado$(NC)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c directories
	@echo "$(YELLOW)Compilando $<...$(NC)"
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "$(GREEN)Compilación de $< exitosa$(NC)"

run: all
	./$(EXEC)

clean:
	@echo "$(YELLOW)Limpiando archivos generados...$(NC)"
	@rm -rf $(OBJ_DIR) $(BIN_DIR) $(DATA_DIR)
	@echo "$(GREEN)Limpieza completada$(NC)"