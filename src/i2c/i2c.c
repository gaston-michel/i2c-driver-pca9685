#include "../../include/i2c.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <string.h>

i2c_status_t i2c_open(i2c_device_t *dev, const char *device_path, uint8_t slave_addr) {
    if (!dev || !device_path) {
        return I2C_ERR_INVALID_PARAM;
    }

    // Copiar la ruta del dispositivo y asignar dirección esclava
    strncpy(dev->device_path, device_path, sizeof(dev->device_path) - 1);
    dev->device_path[sizeof(dev->device_path) - 1] = '\0';
    dev->slave_addr = slave_addr;

    // Abrir el dispositivo I2C del sistema en modo lectura/escritura
    dev->fd = open(dev->device_path, O_RDWR);
    if (dev->fd < 0) {
        return I2C_ERR_OPEN;
    }

    // Configurar el ioctl con la dirección del esclavo
    if (ioctl(dev->fd, I2C_SLAVE, dev->slave_addr) < 0) {
        close(dev->fd);
        dev->fd = -1;
        return I2C_ERR_SLAVE;
    }

    return I2C_SUCCESS;
}

i2c_status_t i2c_close(i2c_device_t *dev) {
    if (!dev || dev->fd < 0) {
        return I2C_ERR_INVALID_PARAM;
    }

    if (close(dev->fd) < 0) {
        // Fallo al cerrar descriptor
        dev->fd = -1; // Lo forzamos a inválido de todas formas
        return I2C_ERR_INVALID_PARAM; 
    }
    
    dev->fd = -1;
    return I2C_SUCCESS;
}

i2c_status_t i2c_set_slave_addr(i2c_device_t *dev, uint8_t slave_addr) {
    if (!dev || dev->fd < 0) {
        return I2C_ERR_INVALID_PARAM;
    }

    // Reconfigurar el dispositivo para hablar con otra dirección
    if (ioctl(dev->fd, I2C_SLAVE, slave_addr) < 0) {
        return I2C_ERR_SLAVE;
    }
    
    dev->slave_addr = slave_addr;
    return I2C_SUCCESS;
}

i2c_status_t i2c_write_byte(i2c_device_t *dev, uint8_t reg, uint8_t value) {
    if (!dev || dev->fd < 0) {
        return I2C_ERR_INVALID_PARAM;
    }

    // Para escribir en un registro, se envía: [Registro][Valor]
    uint8_t buffer[2] = {reg, value};
    
    if (write(dev->fd, buffer, 2) != 2) {
        return I2C_ERR_WRITE;
    }

    return I2C_SUCCESS;
}

i2c_status_t i2c_write_block(i2c_device_t *dev, uint8_t reg, const uint8_t *data, size_t length) {
    if (!dev || dev->fd < 0 || !data || length == 0) {
        return I2C_ERR_INVALID_PARAM;
    }

    // Buffer temporal para enviar el bloque.
    // Tamaño máximo asumido de 128 bytes es suficiente para el PCA9685.
    uint8_t buffer[128];
    
    if (length > (sizeof(buffer) - 1)) {
        return I2C_ERR_INVALID_PARAM; // Bloque demasiado grande para la pila local
    }

    buffer[0] = reg; // Primer byte es la dirección base del registro
    memcpy(&buffer[1], data, length); // Luego los datos

    if (write(dev->fd, buffer, length + 1) != (ssize_t)(length + 1)) {
        return I2C_ERR_WRITE;
    }

    return I2C_SUCCESS;
}

i2c_status_t i2c_read_byte(i2c_device_t *dev, uint8_t reg, uint8_t *value) {
    if (!dev || dev->fd < 0 || !value) {
        return I2C_ERR_INVALID_PARAM;
    }

    // Primero escribimos en el bus qué registro queremos leer
    if (write(dev->fd, &reg, 1) != 1) {
        return I2C_ERR_WRITE;
    }

    // Luego leemos la respuesta del esclavo
    if (read(dev->fd, value, 1) != 1) {
        return I2C_ERR_READ;
    }

    return I2C_SUCCESS;
}

i2c_status_t i2c_read_block(i2c_device_t *dev, uint8_t reg, uint8_t *data, size_t length) {
    if (!dev || dev->fd < 0 || !data || length == 0) {
        return I2C_ERR_INVALID_PARAM;
    }

    // Escribimos el registro desde donde iniciaremos la lectura
    if (write(dev->fd, &reg, 1) != 1) {
        return I2C_ERR_WRITE;
    }

    // Leemos el bloque entero
    if (read(dev->fd, data, length) != (ssize_t)length) {
        return I2C_ERR_READ;
    }

    return I2C_SUCCESS;
}
