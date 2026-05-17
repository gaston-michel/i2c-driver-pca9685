#include "../../include/i2c.h"
#include <string.h>
#include <stdio.h>

/*
 * MOCK I2C BUS PARA TESTING (Nivel Unitario)
 * Este archivo reemplaza a src/i2c/i2c.c al compilar libpca9685_mock.so.
 * Emula la memoria interna del chip PCA9685 y simula operaciones I2C exitosas
 * sin necesidad de hardware real ni un SO Linux.
 */

// Simulamos la memoria del PCA9685 (256 registros direccionables)
static uint8_t mock_registers[256];
static int mock_fd_counter = 100;

// -- Funciones exclusivas del Mock para Python (ctypes) --
uint8_t mock_i2c_get_reg(uint8_t reg) {
    return mock_registers[reg];
}

void mock_i2c_reset(void) {
    memset(mock_registers, 0, sizeof(mock_registers));
    mock_fd_counter = 100;
}
// --------------------------------------------------------

i2c_status_t i2c_open(i2c_device_t *dev, const char *device_path, uint8_t slave_addr) {
    if (!dev || !device_path) return I2C_ERR_INVALID_PARAM;
    strncpy(dev->device_path, device_path, sizeof(dev->device_path) - 1);
    dev->device_path[sizeof(dev->device_path) - 1] = '\0';
    dev->slave_addr = slave_addr;
    dev->fd = mock_fd_counter++; // Simulamos un File Descriptor válido
    return I2C_SUCCESS;
}

i2c_status_t i2c_close(i2c_device_t *dev) {
    if (!dev || dev->fd < 0) return I2C_ERR_INVALID_PARAM;
    dev->fd = -1;
    return I2C_SUCCESS;
}

i2c_status_t i2c_set_slave_addr(i2c_device_t *dev, uint8_t slave_addr) {
    if (!dev || dev->fd < 0) return I2C_ERR_INVALID_PARAM;
    dev->slave_addr = slave_addr;
    return I2C_SUCCESS;
}

i2c_status_t i2c_write_byte(i2c_device_t *dev, uint8_t reg, uint8_t value) {
    if (!dev || dev->fd < 0) return I2C_ERR_INVALID_PARAM;
    mock_registers[reg] = value;
    return I2C_SUCCESS;
}

i2c_status_t i2c_write_block(i2c_device_t *dev, uint8_t reg, const uint8_t *data, size_t length) {
    if (!dev || dev->fd < 0 || !data || length == 0) return I2C_ERR_INVALID_PARAM;
    
    for (size_t i = 0; i < length; i++) {
        // En un dispositivo real con Auto-Increment, se escribe secuencialmente
        // El tamaño de la memoria es 256, usamos módulo para prevenir overflow
        mock_registers[(reg + i) % 256] = data[i];
    }
    return I2C_SUCCESS;
}

i2c_status_t i2c_read_byte(i2c_device_t *dev, uint8_t reg, uint8_t *value) {
    if (!dev || dev->fd < 0 || !value) return I2C_ERR_INVALID_PARAM;
    *value = mock_registers[reg];
    return I2C_SUCCESS;
}

i2c_status_t i2c_read_block(i2c_device_t *dev, uint8_t reg, uint8_t *data, size_t length) {
    if (!dev || dev->fd < 0 || !data || length == 0) return I2C_ERR_INVALID_PARAM;
    for (size_t i = 0; i < length; i++) {
        data[i] = mock_registers[(reg + i) % 256];
    }
    return I2C_SUCCESS;
}
