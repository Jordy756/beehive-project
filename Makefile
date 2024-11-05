CC=gcc
CFLAGS=-Wall -Wextra -I./include/core -I./include/types -pthread
LDFLAGS=-pthread

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

.PHONY: all clean run directories

all: directories $(EXEC)

# Crear directorios necesarios
directories:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(DATA_DIR)
	@mkdir -p $(CORE_DIR)
	@mkdir -p $(TYPES_DIR)

$(EXEC): directories $(OBJ_FILES)
	@echo "Enlazando $@..."
	@$(CC) $(OBJ_FILES) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c directories
	@echo "Compilando $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(EXEC)

clean:
	@echo "Limpiando archivos objeto y ejecutables..."
	@rm -rf $(OBJ_DIR) $(BIN_DIR)