CC=gcc
CFLAGS=-Wall -Wextra -I./include -pthread
LDFLAGS=-pthread

SRC_DIR=src
OBJ_DIR=obj
BIN_DIR=bin
DATA_DIR=data

SRC_FILES=$(wildcard $(SRC_DIR)/*.c)
OBJ_FILES=$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))
EXEC=$(BIN_DIR)/beehive_sim

.PHONY: all clean run

all: $(EXEC)

$(EXEC): $(OBJ_FILES)
	@mkdir -p $(BIN_DIR)
	@echo "Enlazando $@..."
	@$(CC) $(OBJ_FILES) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	@echo "Compilando $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

run: $(EXEC)
	@mkdir -p $(DATA_DIR)
	./$(EXEC)

clean:
	@echo "Limpiando archivos objeto y ejecutables..."
	@rm -f $(OBJ_DIR)/*.o $(EXEC)