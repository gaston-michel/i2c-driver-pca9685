#ifndef PCA9685_H
#define PCA9685_H

#include "i2c.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Códigos de error de la librería del PCA9685.
 */
typedef enum {
    PCA_SUCCESS = 0,                /**< Operación exitosa */
    PCA_ERR_I2C = -1,              /**< Error en la transacción I2C subyacente */
    PCA_ERR_INVALID_CHANNEL = -2,  /**< Canal seleccionado fuera de rango (debe ser 0-15) */
    PCA_ERR_INVALID_VALUE = -3,    /**< Valor de ticks fuera de rango (debe ser 0-4095) */
    PCA_ERR_INVALID_FREQ = -4,     /**< Frecuencia de PWM fuera de límites (24Hz a 1526Hz) */
    PCA_ERR_NOT_INITIALIZED = -5,  /**< Intento de uso sin inicializar el dispositivo */
    PCA_ERR_INVALID_PARAM = -6     /**< Parámetro inválido (ej. puntero nulo) */
} pca_status_t;

/**
 * @brief Estructura de control para la instancia del PCA9685.
 */
typedef struct {
    i2c_device_t i2c_dev;          /**< Instancia de conexión I2C interna */
    bool is_initialized;           /**< Bandera de estado de inicialización */
    float current_freq;            /**< Frecuencia actual del PWM configurada */
} pca9685_t;

/**
 * @brief Inicializa la conexión I2C y configura el PCA9685 a un estado por defecto.
 * 
 * @param device Puntero a la estructura de control de PCA9685.
 * @param i2c_bus Ruta al bus I2C (ej. "/dev/i2c-1").
 * @param slave_addr Dirección I2C del PCA9685 (ej. 0x40).
 * @return pca_status_t PCA_SUCCESS si la inicialización y el handshake inicial fueron exitosos.
 */
pca_status_t pca9685_init(pca9685_t *device, const char *i2c_bus, uint8_t slave_addr);

/**
 * @brief Cierra limpiamente la conexión y deshabilita salidas si es necesario.
 * 
 * @param device Puntero al dispositivo PCA9685.
 * @return pca_status_t PCA_SUCCESS en caso de éxito.
 */
pca_status_t pca9685_close(pca9685_t *device);

/**
 * @brief Pone al PCA9685 en modo de bajo consumo (Sleep).
 * 
 * @param device Puntero al dispositivo PCA9685.
 * @return pca_status_t PCA_SUCCESS en caso de éxito.
 */
pca_status_t pca9685_sleep(pca9685_t *device);

/**
 * @brief Despierta al PCA9685 restableciendo el oscilador interno.
 * 
 * @param device Puntero al dispositivo PCA9685.
 * @return pca_status_t PCA_SUCCESS en caso de éxito.
 */
pca_status_t pca9685_wake(pca9685_t *device);

/**
 * @brief Configura la frecuencia del PWM global en Hertz.
 * 
 * @param device Puntero al dispositivo PCA9685.
 * @param freq_hz Frecuencia objetivo (valores típicos entre 24 y 1526).
 * @return pca_status_t PCA_SUCCESS en caso de éxito.
 */
pca_status_t pca9685_set_pwm_freq(pca9685_t *device, float freq_hz);

/**
 * @brief Configura los ticks exactos de encendido y apagado para un canal PWM específico.
 * 
 * @param device Puntero al dispositivo PCA9685.
 * @param channel Canal a configurar (0 a 15).
 * @param on_tick Punto dentro de la cuenta (0-4095) donde la señal pasa a ALTO.
 * @param off_tick Punto dentro de la cuenta (0-4095) donde la señal pasa a BAJO.
 * @return pca_status_t PCA_SUCCESS en caso de éxito.
 */
pca_status_t pca9685_set_channel(pca9685_t *device, uint8_t channel, uint16_t on_tick, uint16_t off_tick);

/**
 * @brief Configura el ciclo de trabajo de un canal en base a un porcentaje (0.0 a 100.0).
 * 
 * @param device Puntero al dispositivo PCA9685.
 * @param channel Canal a configurar (0 a 15).
 * @param percentage Porcentaje de duty cycle (0.0% a 100.0%).
 * @return pca_status_t PCA_SUCCESS en caso de éxito.
 */
pca_status_t pca9685_set_duty_cycle(pca9685_t *device, uint8_t channel, float percentage);

/**
 * @brief Fuerza a un canal a estar 100% encendido (Full ON).
 * 
 * @param device Puntero al dispositivo PCA9685.
 * @param channel Canal a configurar (0 a 15).
 * @return pca_status_t PCA_SUCCESS en caso de éxito.
 */
pca_status_t pca9685_set_full_on(pca9685_t *device, uint8_t channel);

/**
 * @brief Fuerza a un canal a estar 100% apagado (Full OFF).
 * 
 * @param device Puntero al dispositivo PCA9685.
 * @param channel Canal a configurar (0 a 15).
 * @return pca_status_t PCA_SUCCESS en caso de éxito.
 */
pca_status_t pca9685_set_full_off(pca9685_t *device, uint8_t channel);

/**
 * @brief Modifica todos los canales de forma simultánea a los mismos ticks ON/OFF.
 * 
 * @param device Puntero al dispositivo PCA9685.
 * @param on_tick Tick común de encendido (0-4095).
 * @param off_tick Tick común de apagado (0-4095).
 * @return pca_status_t PCA_SUCCESS en caso de éxito.
 */
pca_status_t pca9685_set_all_channels(pca9685_t *device, uint16_t on_tick, uint16_t off_tick);

/**
 * @brief Diagnóstico: Lee los registros ON y OFF físicos de un canal para validar su configuración actual.
 * 
 * @param device Puntero al dispositivo PCA9685.
 * @param channel Canal a consultar (0 a 15).
 * @param on_tick Puntero donde se almacenará el tick ON leído.
 * @param off_tick Puntero donde se almacenará el tick OFF leído.
 * @return pca_status_t PCA_SUCCESS en caso de éxito.
 */
pca_status_t pca9685_read_channel_config(pca9685_t *device, uint8_t channel, uint16_t *on_tick, uint16_t *off_tick);

#endif // PCA9685_H
