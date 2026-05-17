CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -O2 -fPIC

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
LIB_DIR = lib

# Archivos fuente del driver y bus
LIB_SRCS = $(SRC_DIR)/i2c/i2c.c $(SRC_DIR)/pca9685/pca9685.c
LIB_OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(LIB_SRCS))

# Archivos fuente con I2C mockeado (para tests unitarios)
MOCK_SRCS = $(SRC_DIR)/i2c/mock_i2c.c $(SRC_DIR)/pca9685/pca9685.c
MOCK_OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/mock_%.o, $(MOCK_SRCS))

# Objetivos finales
TARGET_DEMO = demo
TARGET_LIB = $(LIB_DIR)/libpca9685.so
TARGET_MOCK_LIB = $(LIB_DIR)/libpca9685_mock.so

all: $(TARGET_DEMO) test-lib

# Regla para compilar objetos normales
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Regla para compilar objetos para el mock
$(OBJ_DIR)/mock_%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Programa de demostración
$(TARGET_DEMO): $(LIB_OBJS) examples/main_demo.c
	@mkdir -p bin
	$(CC) $(CFLAGS) examples/main_demo.c $(LIB_OBJS) -o bin/$(TARGET_DEMO)
	@echo "----------------------------------------"
	@echo "Compilación exitosa."
	@echo "Puedes ejecutar la demo con: ./bin/demo"
	@echo "----------------------------------------"

# Librería compartida (Producción)
$(TARGET_LIB): $(LIB_OBJS)
	@mkdir -p $(LIB_DIR)
	$(CC) -shared -o $@ $(LIB_OBJS)

# Librería compartida (Mock para Unit Tests)
$(TARGET_MOCK_LIB): $(MOCK_OBJS)
	@mkdir -p $(LIB_DIR)
	$(CC) -shared -o $@ $(MOCK_OBJS)

# Generar ambos .so para testing con Python
test-lib: $(TARGET_LIB) $(TARGET_MOCK_LIB)
	@echo "Librerías compartidas generadas en $(LIB_DIR)/ para uso con Python ctypes"

clean:
	rm -rf $(OBJ_DIR) bin $(LIB_DIR)

.PHONY: all clean test-lib
