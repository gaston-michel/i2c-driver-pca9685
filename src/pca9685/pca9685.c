#include "../../include/pca9685.h"
#include <unistd.h>
#include <string.h>

// --- Direcciones de registros internos PCA9685 ---
#define PCA9685_MODE1       0x00
#define PCA9685_MODE2       0x01
#define PCA9685_PRESCALE    0xFE
#define PCA9685_LED0_ON_L   0x06
#define PCA9685_ALL_LED_ON_L 0xFA

// --- Bits de configuración ---
#define MODE1_RESTART 0x80
#define MODE1_SLEEP   0x10
#define MODE1_ALLCALL 0x01
#define MODE1_AUTOINC 0x20
#define MODE2_OUTDRV  0x04

pca_status_t pca9685_init(pca9685_t *device, const char *i2c_bus, uint8_t slave_addr) {
    if (!device || !i2c_bus) return PCA_ERR_INVALID_PARAM;
    
    device->is_initialized = false;
    device->current_freq = 0.0f;

    // Inicializar la capa I2C subyacente
    if (i2c_open(&device->i2c_dev, i2c_bus, slave_addr) != I2C_SUCCESS) {
        return PCA_ERR_I2C;
    }

    // Resetear el dispositivo
    // Configuramos Auto-Increment y Sleep inicial
    i2c_write_byte(&device->i2c_dev, PCA9685_MODE1, MODE1_SLEEP | MODE1_AUTOINC);
    usleep(5000); // 5ms de demora para que el oscilador se asiente

    // Despertar del sleep inicial
    pca9685_wake(device);

    // Configurar MODE2 a Totem Pole (ideal para activar cargas o servos directamente)
    i2c_write_byte(&device->i2c_dev, PCA9685_MODE2, MODE2_OUTDRV);

    device->is_initialized = true;
    return PCA_SUCCESS;
}

pca_status_t pca9685_close(pca9685_t *device) {
    if (!device || !device->is_initialized) return PCA_ERR_NOT_INITIALIZED;
    
    // Apagamos todas las salidas por seguridad antes de salir
    pca9685_set_all_channels(device, 0, 0);
    // Ponemos a dormir el chip para reducir consumo
    pca9685_sleep(device);
    
    // Liberar file descriptor I2C
    if (i2c_close(&device->i2c_dev) != I2C_SUCCESS) {
        return PCA_ERR_I2C;
    }
    
    device->is_initialized = false;
    return PCA_SUCCESS;
}

pca_status_t pca9685_sleep(pca9685_t *device) {
    if (!device) return PCA_ERR_INVALID_PARAM;

    uint8_t mode1;
    if (i2c_read_byte(&device->i2c_dev, PCA9685_MODE1, &mode1) != I2C_SUCCESS) {
        return PCA_ERR_I2C;
    }

    // Poner a dormir apagando el bit restart pero encendiendo el bit sleep
    uint8_t sleep_mode = (mode1 & ~MODE1_RESTART) | MODE1_SLEEP;
    if (i2c_write_byte(&device->i2c_dev, PCA9685_MODE1, sleep_mode) != I2C_SUCCESS) {
        return PCA_ERR_I2C;
    }

    return PCA_SUCCESS;
}

pca_status_t pca9685_wake(pca9685_t *device) {
    if (!device) return PCA_ERR_INVALID_PARAM;

    uint8_t mode1;
    if (i2c_read_byte(&device->i2c_dev, PCA9685_MODE1, &mode1) != I2C_SUCCESS) {
        return PCA_ERR_I2C;
    }

    uint8_t wake_mode = mode1 & ~MODE1_SLEEP; // Limpiar bit sleep
    if (i2c_write_byte(&device->i2c_dev, PCA9685_MODE1, wake_mode) != I2C_SUCCESS) {
        return PCA_ERR_I2C;
    }

    usleep(500); // 500us mínimo requerido por el datasheet para que el oscilador arranque

    // Escribir un 1 lógico en el bit RESTART para reanudar el ciclo de PWM
    if (i2c_write_byte(&device->i2c_dev, PCA9685_MODE1, wake_mode | MODE1_RESTART) != I2C_SUCCESS) {
        return PCA_ERR_I2C;
    }

    return PCA_SUCCESS;
}

pca_status_t pca9685_set_pwm_freq(pca9685_t *device, float freq_hz) {
    if (!device || !device->is_initialized) return PCA_ERR_NOT_INITIALIZED;
    if (freq_hz < 24.0f || freq_hz > 1526.0f) return PCA_ERR_INVALID_FREQ;

    // Cálculo del registro prescaler según Datasheet (Oscilador 25MHz)
    float prescaleval = 25000000.0f;
    prescaleval /= 4096.0f;
    prescaleval /= freq_hz;
    prescaleval -= 1.0f;
    uint8_t prescale = (uint8_t)(prescaleval + 0.5f); // +0.5 para emular round() sin libm

    uint8_t old_mode;
    if (i2c_read_byte(&device->i2c_dev, PCA9685_MODE1, &old_mode) != I2C_SUCCESS) {
        return PCA_ERR_I2C;
    }

    uint8_t new_mode = (old_mode & ~MODE1_RESTART) | MODE1_SLEEP;
    
    // Entrar a sleep mode (requisito del hardware para cambiar prescale)
    if (i2c_write_byte(&device->i2c_dev, PCA9685_MODE1, new_mode) != I2C_SUCCESS) return PCA_ERR_I2C;
    
    // Escribir el nuevo prescaler
    if (i2c_write_byte(&device->i2c_dev, PCA9685_PRESCALE, prescale) != I2C_SUCCESS) return PCA_ERR_I2C;
    
    // Despertar al chip restaurando el estado original
    if (i2c_write_byte(&device->i2c_dev, PCA9685_MODE1, old_mode) != I2C_SUCCESS) return PCA_ERR_I2C;
    
    usleep(500); // Esperar estabilización del oscilador
    
    // Encender el motor PWM nuevamente
    if (i2c_write_byte(&device->i2c_dev, PCA9685_MODE1, old_mode | MODE1_RESTART) != I2C_SUCCESS) return PCA_ERR_I2C;

    device->current_freq = freq_hz;
    return PCA_SUCCESS;
}

pca_status_t pca9685_set_channel(pca9685_t *device, uint8_t channel, uint16_t on_tick, uint16_t off_tick) {
    if (!device || !device->is_initialized) return PCA_ERR_NOT_INITIALIZED;
    if (channel > 15) return PCA_ERR_INVALID_CHANNEL;
    if (on_tick > 4095 || off_tick > 4095) return PCA_ERR_INVALID_VALUE;

    uint8_t reg_base = PCA9685_LED0_ON_L + (4 * channel);
    uint8_t data[4];
    data[0] = on_tick & 0xFF;
    data[1] = on_tick >> 8;
    data[2] = off_tick & 0xFF;
    data[3] = off_tick >> 8;

    // Escribir los 4 bytes simultáneamente gracias a MODE1_AUTOINC
    if (i2c_write_block(&device->i2c_dev, reg_base, data, 4) != I2C_SUCCESS) {
        return PCA_ERR_I2C;
    }

    return PCA_SUCCESS;
}

pca_status_t pca9685_set_all_channels(pca9685_t *device, uint16_t on_tick, uint16_t off_tick) {
    if (!device || !device->is_initialized) return PCA_ERR_NOT_INITIALIZED;
    if (on_tick > 4095 || off_tick > 4095) return PCA_ERR_INVALID_VALUE;

    uint8_t data[4];
    data[0] = on_tick & 0xFF;
    data[1] = on_tick >> 8;
    data[2] = off_tick & 0xFF;
    data[3] = off_tick >> 8;

    if (i2c_write_block(&device->i2c_dev, PCA9685_ALL_LED_ON_L, data, 4) != I2C_SUCCESS) {
        return PCA_ERR_I2C;
    }

    return PCA_SUCCESS;
}

pca_status_t pca9685_set_duty_cycle(pca9685_t *device, uint8_t channel, float percentage) {
    if (percentage < 0.0f) percentage = 0.0f;
    if (percentage > 100.0f) percentage = 100.0f;

    // Casos especiales definidos en el datasheet usando Bits específicos
    if (percentage == 0.0f) {
        return pca9685_set_full_off(device, channel);
    }
    if (percentage == 100.0f) {
        return pca9685_set_full_on(device, channel);
    }

    // Regla de 3 simple (100% == 4096 ticks)
    uint16_t off_tick = (uint16_t)((percentage / 100.0f) * 4096.0f);
    if (off_tick > 4095) off_tick = 4095;

    // Siempre arrancamos en el tick 0
    return pca9685_set_channel(device, channel, 0, off_tick);
}

pca_status_t pca9685_set_full_on(pca9685_t *device, uint8_t channel) {
    if (!device || !device->is_initialized) return PCA_ERR_NOT_INITIALIZED;
    if (channel > 15) return PCA_ERR_INVALID_CHANNEL;

    uint8_t reg_base = PCA9685_LED0_ON_L + (4 * channel);
    uint8_t data[4];
    data[0] = 0x00;
    data[1] = 0x10; // 0x10 es el bit 4 (Full ON bit activo)
    data[2] = 0x00;
    data[3] = 0x00; // Full OFF bit inactivo

    if (i2c_write_block(&device->i2c_dev, reg_base, data, 4) != I2C_SUCCESS) {
        return PCA_ERR_I2C;
    }

    return PCA_SUCCESS;
}

pca_status_t pca9685_set_full_off(pca9685_t *device, uint8_t channel) {
    if (!device || !device->is_initialized) return PCA_ERR_NOT_INITIALIZED;
    if (channel > 15) return PCA_ERR_INVALID_CHANNEL;

    uint8_t reg_base = PCA9685_LED0_ON_L + (4 * channel);
    uint8_t data[4];
    data[0] = 0x00;
    data[1] = 0x00; // Full ON inactivo
    data[2] = 0x00;
    data[3] = 0x10; // 0x10 es el bit 4 (Full OFF bit activo)

    if (i2c_write_block(&device->i2c_dev, reg_base, data, 4) != I2C_SUCCESS) {
        return PCA_ERR_I2C;
    }

    return PCA_SUCCESS;
}

pca_status_t pca9685_read_channel_config(pca9685_t *device, uint8_t channel, uint16_t *on_tick, uint16_t *off_tick) {
    if (!device || !device->is_initialized) return PCA_ERR_NOT_INITIALIZED;
    if (channel > 15) return PCA_ERR_INVALID_CHANNEL;
    if (!on_tick || !off_tick) return PCA_ERR_INVALID_PARAM;

    uint8_t reg_base = PCA9685_LED0_ON_L + (4 * channel);
    uint8_t data[4];

    if (i2c_read_block(&device->i2c_dev, reg_base, data, 4) != I2C_SUCCESS) {
        return PCA_ERR_I2C;
    }

    // Reconstruir los enteros de 12 bits leyendo sólo los 4 bits bajos de los registros HIGH
    *on_tick = data[0] | ((data[1] & 0x0F) << 8);
    *off_tick = data[2] | ((data[3] & 0x0F) << 8);
    
    // Mapeo semántico especial: Si está activo el flag de Full On/Off, indicarlo de forma especial (ej. 4096)
    if (data[1] & 0x10) *on_tick = 4096;
    if (data[3] & 0x10) *off_tick = 4096;

    return PCA_SUCCESS;
}
