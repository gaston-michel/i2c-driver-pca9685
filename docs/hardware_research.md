# Investigación del Hardware: PCA9685

## 1. Resumen Técnico del Dispositivo

*   **¿Qué hace el chip?**
    El PCA9685 es un controlador PWM de 16 canales controlado a través del bus I2C. Está diseñado para aplicaciones de retroiluminación de color y control de servomotores. Cada uno de sus 16 canales ofrece una resolución de 12 bits (4096 pasos), lo que permite un control muy fino del ciclo de trabajo (duty cycle).
*   **¿Cómo funciona su dirección I2C?**
    Utiliza direccionamiento de 7 bits. La dirección I2C base de fábrica es `0x40`. Posee 6 pines de configuración física (A0 a A5) que permiten configurar hasta 62 dispositivos independientes en el mismo bus I2C (desde `0x40` hasta `0x7E`). Además, responde a una dirección especial de "All Call" (`0x70` por defecto) para programar múltiples chips simultáneamente.
*   **¿Cómo se configura la frecuencia PWM?**
    Todos los 16 canales comparten la misma frecuencia PWM, la cual se configura a través del registro `PRE_SCALE`. Posee un oscilador interno de 25 MHz y soporta frecuencias de salida desde 24 Hz hasta 1526 Hz. 
    **⚠️ Detalle crucial:** Para modificar el registro `PRE_SCALE`, el bit de Sleep en el registro `MODE1` debe estar en `1` (el oscilador debe apagarse). Luego de configurar la frecuencia, se debe despertar el chip y reiniciar la señal PWM.
*   **¿Cómo se escriben los valores de salida por canal?**
    El chip posee un contador interno de 12 bits (de 0 a 4095). Cada canal tiene 4 registros de control: `ON_L`, `ON_H`, `OFF_L` y `OFF_H`.
    *   **ON registers:** Definen en qué "tick" (0-4095) el pin pasará a estado ALTO.
    *   **OFF registers:** Definen en qué "tick" el pin pasará a estado BAJO.
    *   **Full ON / Full OFF:** El bit 4 del registro `ON_H` fuerza el canal a estar siempre encendido (ignorando los contadores). El bit 4 del registro `OFF_H` lo fuerza a estar siempre apagado.
*   **¿Qué condiciones de error pueden aparecer?**
    *   **NACK (Not Acknowledge):** Ocurre a nivel del protocolo I2C si el dispositivo no tiene la dirección correcta o está desconectado.
    *   **Escritura bloqueada:** Si se intenta escribir la frecuencia (`PRE_SCALE`) sin estar en modo Sleep, el chip ignorará el comando.
    *   **Conflicto de estados:** Si el bit "Full ON" y el "Full OFF" se configuran en 1 al mismo tiempo, el "Full OFF" tiene prioridad.

---

## 2. Tabla de Registros Relevantes

El PCA9685 tiene múltiples registros. Para el driver base, nos enfocaremos en estos esenciales:

| Registro | Dir. (Hex) | Propósito Principal |
| :--- | :--- | :--- |
| `MODE1` | `0x00` | Configuración general: Modo Sleep (bit 4), Auto-Increment (bit 5), Restart (bit 7). |
| `MODE2` | `0x01` | Estructura de salidas: Lógica invertida (bit 4), Salida tipo Totem-pole/Open-drain (bit 2). |
| `LED0_ON_L` | `0x06` | Canal 0: Byte bajo del tick de encendido (bits 0-7). |
| `LED0_ON_H` | `0x07` | Canal 0: Byte alto de encendido (bits 8-11) y **Bit de Full ON** (bit 4). |
| `LED0_OFF_L`| `0x08` | Canal 0: Byte bajo del tick de apagado (bits 0-7). |
| `LED0_OFF_H`| `0x09` | Canal 0: Byte alto de apagado (bits 8-11) y **Bit de Full OFF** (bit 4). |
| *(siguientes)*| `+ 4 bytes` | La misma estructura de 4 bytes se repite para los canales 1 al 15 (`LED1_ON_L`, etc). |
| `ALL_LED_ON_L`| `0xFA` | Escribe simultáneamente en todos los registros `ON_L` de los 16 canales. |
| `ALL_LED_ON_H`| `0xFB` | Escribe simultáneamente en todos los registros `ON_H` de los 16 canales. |
| `ALL_LED_OFF_L`| `0xFC` | Escribe simultáneamente en todos los registros `OFF_L` de los 16 canales. |
| `ALL_LED_OFF_H`| `0xFD` | Escribe simultáneamente en todos los registros `OFF_H` de los 16 canales. |
| `PRE_SCALE` | `0xFE` | Prescaler para configurar la frecuencia PWM. Solo se puede escribir en modo Sleep. |

> **Nota sobre Auto-Increment**: Si activamos el bit 5 de `MODE1`, podemos enviar los 4 bytes de un canal en una sola transacción I2C enviando la dirección de `LEDn_ON_L` seguida de 4 bytes de datos. Esto es clave para optimizar la velocidad del driver.

---

## 3. Esquema de Conexión (PCA9685 a Raspberry Pi 5)

Las comunicaciones con el bus principal de I2C de la Raspberry Pi 5 (`/dev/i2c-1`) ocurren a través de los pines GPIO 2 y 3.

### Lógica (Raspberry Pi a Módulo)
| Pin Módulo PCA9685 | Pin Raspberry Pi 5 (Header de 40 pines) | Descripción |
| :--- | :--- | :--- |
| **VCC** | Pin 1 (3.3V) | Alimentación del circuito lógico del PCA9685. Recomendado 3.3V para compatibilidad directa con los niveles de voltaje I2C de la RPi. |
| **GND** | Pin 6, 9 o 14 (GND) | Masa lógica común del sistema. |
| **SDA** | Pin 3 (GPIO 2 / SDA1) | Línea de datos bidireccional I2C. |
| **SCL** | Pin 5 (GPIO 3 / SCL1) | Línea de reloj I2C. |
| **OE** (Output Enable) | A masa (GND) | Pin activo en bajo. Conectarlo a masa habilita de forma permanente las salidas de los canales PWM. |

### Potencia (Para Servomotores o LEDs de alto consumo)
| Pin Módulo PCA9685 | Conexión a | Descripción |
| :--- | :--- | :--- |
| **V+** (Power) | Batería o Fuente Externa (Ej. 5V - 3A) | Alimentación dedicada para la carga (servos/leds). **NO conectar V+ a los pines de 5V de la Raspberry Pi**, los picos de consumo de los servos reiniciarán o quemarán la placa. |
| **GND** (Power) | Masa de la Fuente Externa | **Importante:** La masa de esta fuente de potencia debe estar conectada (puenteada) a la masa lógica de la Raspberry Pi para cerrar el circuito. |
