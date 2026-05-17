CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -O2

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj

# Archivos fuente del driver y bus
LIB_SRCS = $(SRC_DIR)/i2c/i2c.c $(SRC_DIR)/pca9685/pca9685.c
LIB_OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(LIB_SRCS))

# Objetivo final
TARGET_DEMO = demo

all: $(TARGET_DEMO)

# Regla para compilar los objetos de la librería (.o)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Regla para compilar el programa de demostración
$(TARGET_DEMO): $(LIB_OBJS) examples/main_demo.c
	@mkdir -p bin
	$(CC) $(CFLAGS) examples/main_demo.c $(LIB_OBJS) -o bin/$(TARGET_DEMO)
	@echo "----------------------------------------"
	@echo "Compilación exitosa."
	@echo "Puedes ejecutar la demo con: ./bin/demo"
	@echo "----------------------------------------"

clean:
	rm -rf $(OBJ_DIR) bin

.PHONY: all clean
