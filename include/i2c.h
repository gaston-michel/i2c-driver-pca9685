#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Códigos de error para las operaciones I2C.
 */
typedef enum {
    I2C_SUCCESS = 0,            /**< Operación exitosa */
    I2C_ERR_OPEN = -1,          /**< Error al abrir el dispositivo I2C */
    I2C_ERR_SLAVE = -2,         /**< Error al configurar la dirección del esclavo */
    I2C_ERR_WRITE = -3,         /**< Error de escritura en el bus */
    I2C_ERR_READ = -4,          /**< Error de lectura en el bus */
    I2C_ERR_INVALID_PARAM = -5  /**< Parámetros de función inválidos */
} i2c_status_t;

/**
 * @brief Estructura de contexto para el dispositivo I2C.
 */
typedef struct {
    int fd;                     /**< Descriptor de archivo del dispositivo I2C */
    uint8_t slave_addr;         /**< Dirección de 7 bits del dispositivo esclavo */
    char device_path[64];       /**< Ruta del archivo de dispositivo (ej. /dev/i2c-1) */
} i2c_device_t;

/**
 * @brief Abre el bus I2C y configura la dirección del dispositivo esclavo.
 * 
 * @param dev Puntero a la estructura del dispositivo I2C.
 * @param device_path Ruta al dispositivo I2C en Linux (ej. "/dev/i2c-1").
 * @param slave_addr Dirección I2C del chip esclavo.
 * @return i2c_status_t I2C_SUCCESS en caso de éxito, o un código de error negativo.
 */
i2c_status_t i2c_open(i2c_device_t *dev, const char *device_path, uint8_t slave_addr);

/**
 * @brief Cierra el descriptor del bus I2C de forma limpia.
 * 
 * @param dev Puntero a la estructura del dispositivo I2C.
 * @return i2c_status_t I2C_SUCCESS en caso de éxito.
 */
i2c_status_t i2c_close(i2c_device_t *dev);

/**
 * @brief Cambia de forma dinámica la dirección I2C del esclavo en el bus abierto.
 * 
 * @param dev Puntero a la estructura del dispositivo I2C.
 * @param slave_addr Nueva dirección I2C del chip esclavo.
 * @return i2c_status_t I2C_SUCCESS en caso de éxito.
 */
i2c_status_t i2c_set_slave_addr(i2c_device_t *dev, uint8_t slave_addr);

/**
 * @brief Escribe un único byte de datos en un registro del dispositivo esclavo.
 * 
 * @param dev Puntero a la estructura del dispositivo I2C.
 * @param reg Dirección del registro destino.
 * @param value Valor de 8 bits a escribir.
 * @return i2c_status_t I2C_SUCCESS en caso de éxito.
 */
i2c_status_t i2c_write_byte(i2c_device_t *dev, uint8_t reg, uint8_t value);

/**
 * @brief Escribe un bloque secuencial de bytes partiendo desde un registro base.
 * 
 * @param dev Puntero a la estructura del dispositivo I2C.
 * @param reg Dirección del registro base inicial.
 * @param data Buffer que contiene los datos a escribir.
 * @param length Cantidad de bytes a escribir.
 * @return i2c_status_t I2C_SUCCESS en caso de éxito.
 */
i2c_status_t i2c_write_block(i2c_device_t *dev, uint8_t reg, const uint8_t *data, size_t length);

/**
 * @brief Lee un único byte desde un registro del dispositivo esclavo.
 * 
 * @param dev Puntero a la estructura del dispositivo I2C.
 * @param reg Dirección del registro a leer.
 * @param value Puntero donde se almacenará el byte leído.
 * @return i2c_status_t I2C_SUCCESS en caso de éxito.
 */
i2c_status_t i2c_read_byte(i2c_device_t *dev, uint8_t reg, uint8_t *value);

/**
 * @brief Lee un bloque secuencial de bytes partiendo desde un registro base.
 * 
 * @param dev Puntero a la estructura del dispositivo I2C.
 * @param reg Dirección del registro base inicial.
 * @param data Buffer donde se almacenarán los datos leídos.
 * @param length Cantidad de bytes a leer.
 * @return i2c_status_t I2C_SUCCESS en caso de éxito.
 */
i2c_status_t i2c_read_block(i2c_device_t *dev, uint8_t reg, uint8_t *data, size_t length);

#endif // I2C_H
