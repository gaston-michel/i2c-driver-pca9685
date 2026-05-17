#include <stdio.h>
#include <unistd.h>
#include "../include/pca9685.h"

/* 
 * Ejemplo de abstracción para un Servomotor Estándar (SG90 o similar)
 * A 50Hz, el periodo es de 20ms = 4096 ticks.
 * - 0.5 ms de pulso (0 grados) = (0.5 / 20.0) * 4096 ≈ 102 ticks
 * - 2.5 ms de pulso (180 grados) = (2.5 / 20.0) * 4096 ≈ 512 ticks
 */
#define SERVO_MIN_TICK 102
#define SERVO_MAX_TICK 512

void set_servo_angle(pca9685_t *dev, uint8_t channel, float angle) {
    if (angle < 0.0f) angle = 0.0f;
    if (angle > 180.0f) angle = 180.0f;
    
    // Mapeo lineal: angle(0..180) -> tick(MIN..MAX)
    uint16_t tick = SERVO_MIN_TICK + (uint16_t)((angle / 180.0f) * (SERVO_MAX_TICK - SERVO_MIN_TICK));
    pca9685_set_channel(dev, channel, 0, tick);
}

int main() {
    pca9685_t pwm_controller;
    
    printf("=========================================\n");
    printf("      Demo PCA9685 - Espacio de Usuario  \n");
    printf("=========================================\n\n");
    
    printf("1. Inicializando dispositivo en /dev/i2c-1 a 0x40...\n");
    // Nota: Es posible que requieras ejecutar el binario con 'sudo' 
    // o tener al usuario en el grupo 'i2c' dependiendo de los permisos en Linux.
    pca_status_t status = pca9685_init(&pwm_controller, "/dev/i2c-1", 0x40);
    if (status != PCA_SUCCESS) {
        printf("[ERROR] Falló la inicialización (código %d).\n", status);
        printf("Asegurate de que el bus I2C exista y el chip esté conectado.\n");
        return -1;
    }
    printf("[OK] Dispositivo inicializado.\n\n");
    
    printf("2. Configurando frecuencia a 50Hz (típico para servos)...\n");
    pca9685_set_pwm_freq(&pwm_controller, 50.0f);
    printf("[OK] Frecuencia configurada.\n\n");
    
    printf("3. Secuencia de prueba de estado y PWM:\n");
    
    // Prueba 1: Encendido/Apagado total (Ideal para probar con un LED)
    printf("   [Prueba LED] Forzando Full ON en Canal 0...\n");
    pca9685_set_full_on(&pwm_controller, 0);
    sleep(1);
    
    printf("   [Prueba LED] Forzando Full OFF en Canal 0...\n");
    pca9685_set_full_off(&pwm_controller, 0);
    sleep(1);
    
    // Prueba 2: Uso del porcentaje
    printf("   [Prueba PWM] Ciclo de trabajo al 25%% en Canal 1...\n");
    pca9685_set_duty_cycle(&pwm_controller, 1, 25.0f);
    sleep(1);
    pca9685_set_full_off(&pwm_controller, 1); // Apagar para limpiar
    
    // Prueba 3: Servo (Capa de abstracción)
    printf("   [Prueba SERVO] Moviendo servo (Canal 15) a 0 grados...\n");
    set_servo_angle(&pwm_controller, 15, 0.0f);
    sleep(1);
    
    printf("   [Prueba SERVO] Moviendo servo a 90 grados (centro)...\n");
    set_servo_angle(&pwm_controller, 15, 90.0f);
    sleep(1);
    
    printf("   [Prueba SERVO] Moviendo servo a 180 grados...\n");
    set_servo_angle(&pwm_controller, 15, 180.0f);
    sleep(1);
    
    printf("\n4. Apagando el hardware y cerrando bus...\n");
    pca9685_close(&pwm_controller);
    
    printf("[OK] Demostración finalizada exitosamente.\n");
    return 0;
}
