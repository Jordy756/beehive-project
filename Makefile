CC = gcc
CFLAGS = -Wall -Wextra -Werror -pthread -g -I./include/core -I./include/types
LDFLAGS = -pthread -ljson-c

# Directorios
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
DATA_DIR = data
INCLUDE_DIR = include
CORE_DIR = $(INCLUDE_DIR)/core
TYPES_DIR = $(INCLUDE_DIR)/types

# Archivos fuente y objeto
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))
EXEC = $(BIN_DIR)/beehive_sim

# Archivos de cabecera
HEADERS = $(wildcard $(CORE_DIR)/*.h) $(wildcard $(TYPES_DIR)/*.h)

# Colores para mensajes
GREEN = \033[0;32m
RED = \033[0;31m
YELLOW = \033[1;33m
NC = \033[0m

.PHONY: all clean run directories check-dirs check-deps

all: check-deps directories $(EXEC)
	@echo "$(GREEN)Compilación completada con éxito$(NC)"

# Verificar dependencias
check-deps:
	@echo "$(YELLOW)Verificando dependencias...$(NC)"
	@which $(CC) >/dev/null 2>&1 || (echo "$(RED)Error: GCC no está instalado$(NC)" && exit 1)
	@ldconfig -p | grep libjson-c >/dev/null 2>&1 || (echo "$(RED)Error: libjson-c no está instalada$(NC)" && exit 1)
	@echo "$(GREEN)Todas las dependencias están instaladas$(NC)"

# Crear directorios necesarios
directories: check-dirs
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(DATA_DIR)
	@mkdir -p $(CORE_DIR)
	@mkdir -p $(TYPES_DIR)

# Verificar permisos de directorios
check-dirs:
	@echo "$(YELLOW)Verificando directorios...$(NC)"
	@if [ ! -w . ]; then \
		echo "$(RED)Error: No hay permisos de escritura en el directorio actual$(NC)"; \
		exit 1; \
	fi

# Regla principal de compilación
$(EXEC): $(OBJ_FILES)
	@echo "$(YELLOW)Enlazando $@...$(NC)"
	@$(CC) $(OBJ_FILES) -o $@ $(LDFLAGS)
	@echo "$(GREEN)Enlazado completado$(NC)"

# Regla para compilar archivos objeto
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@echo "$(YELLOW)Compilando $<...$(NC)"
	@$(CC) $(CFLAGS) -c $< -o $@ || (echo "$(RED)Error al compilar $<$(NC)" && exit 1)
	@echo "$(GREEN)Compilación de $< exitosa$(NC)"

# Ejecutar con depuración
run: all
	@echo "$(YELLOW)Ejecutando programa con depuración...$(NC)"
	@gdb -ex=r -ex="bt" -ex=q --args $(EXEC)

# Ejecutar sin depuración
run-no-debug: all
	@echo "$(YELLOW)Ejecutando programa...$(NC)"
	@$(EXEC)

# Verificar memoria con Valgrind
memcheck: all
	@echo "$(YELLOW)Ejecutando Valgrind...$(NC)"
	@valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(EXEC)

# Limpiar archivos generados
clean:
	@echo "$(YELLOW)Limpiando archivos generados...$(NC)"
	@rm -rf $(OBJ_DIR) $(BIN_DIR) $(DATA_DIR)
	@echo "$(GREEN)Limpieza completada$(NC)"

# Mostrar información de compilación
info:
	@echo "$(YELLOW)Información de compilación:$(NC)"
	@echo "Compilador: $(CC)"
	@echo "Flags de compilación: $(CFLAGS)"
	@echo "Flags de enlazado: $(LDFLAGS)"
	@echo "Archivos fuente: $(SRC_FILES)"
	@echo "Archivos objeto: $(OBJ_FILES)"