CC=gcc
CFLAGS=-Wall -Wextra -I./include -pthread
LDFLAGS=-pthread

# Directorios
SRC_DIR=src
OBJ_DIR=obj
BIN_DIR=bin
DATA_DIR=data
INCLUDE_DIR=include
TYPES_DIR=$(INCLUDE_DIR)/types
CORE_DIR=$(INCLUDE_DIR)/core

# Encontrar todos los archivos fuente
SRC_FILES=$(wildcard $(SRC_DIR)/*.c)
OBJ_FILES=$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))
EXEC=$(BIN_DIR)/beehive_sim

# Headers
CORE_HEADERS=$(wildcard $(CORE_DIR)/*.h)
TYPE_HEADERS=$(wildcard $(TYPES_DIR)/*.h)

.PHONY: all clean run directories

# Target principal
all: directories $(EXEC)

# Crear directorios necesarios
directories:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(DATA_DIR)
	@mkdir -p $(TYPES_DIR)
	@mkdir -p $(CORE_DIR)

# Linkear el ejecutable
$(EXEC): $(OBJ_FILES)
	@echo "Enlazando $@..."
	@$(CC) $(OBJ_FILES) -o $@ $(LDFLAGS)

# Compilar archivos objeto
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(CORE_HEADERS) $(TYPE_HEADERS)
	@echo "Compilando $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Ejecutar el programa
run: all
	./$(EXEC)

# Limpiar archivos compilados
clean:
	@echo "Limpiando archivos objeto y ejecutables..."
	@rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "Limpiando archivos de datos..."
	@rm -f $(DATA_DIR)/*

# Mostrar información de compilación
info:
	@echo "Archivos fuente:"
	@echo "$(SRC_FILES)"
	@echo "\nArchivos objeto:"
	@echo "$(OBJ_FILES)"
	@echo "\nHeaders del core:"
	@echo "$(CORE_HEADERS)"
	@echo "\nHeaders de tipos:"
	@echo "$(TYPE_HEADERS)"