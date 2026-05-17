# Sistema de Testing Híbrido (Python + C)

Si bien el driver está escrito puramente en C para lograr alta eficiencia y bajo nivel, se utiliza Python para el testing debido a las siguientes ventajas:
*   **Velocidad de desarrollo:** Escribir y modificar tests en Python es mucho más ágil que en C.
*   **Ecosistema de análisis:** Podemos integrar fácilmente `pandas` y `matplotlib` para registrar tiempos de ejecución y generar gráficas sobre la fiabilidad del bus I2C.

---

## 2. Arquitectura de Interfaz (ctypes)

Para que Python pueda ejecutar y probar nuestro código en C, se utiliza una técnica de enlace (binding).
1. Las funciones en C se compilan no como un ejecutable, sino como una **librería compartida** (`libpca9685.so` en Linux / `.dll` en Windows).
2. Se utiliza la librería estándar `ctypes` de Python para cargar ese archivo binario.
3. Python llama a las funciones de C (ej. `pca9685_set_duty_cycle`) pasando las variables nativas, orquestando el entorno de prueba.

---

## 3. Nivel A: Tests Unitarios (Software Puro)

El objetivo de los tests unitarios es probar la matemática, la validación de parámetros y la lógica de estado sin tocar el hardware real.

**Estrategia (Mocking):**
*   Se crea un archivo `src/i2c/mock_i2c.c` que reemplaza al `i2c.c` original exclusivamente durante la fase de unit testing.
*   Este "Mock" interceptará las llamadas a `open()`, `ioctl()`, `write()` y `read()`. En lugar de comunicarse con `/dev/i2c-1`, guardará los bytes en un **array de memoria virtual** (simulando los registros del PCA9685).
*   **Flujo del Test:**
    1. Python llama a `pca9685_set_duty_cycle(..., 50.0)`.
    2. El driver en C calcula los ticks y llama a `i2c_write_block`.
    3. El `mock_i2c.c` guarda los valores simulados.
    4. Python lee ese array virtual para hacer un `assert` comprobando si la matemática en C calculó exactamente 2048 ticks.

---

## 4. Nivel B: Tests de Integración (Hardware in the Loop)

Estos tests se correrán de forma exclusiva en la Raspberry Pi 5 con el PCA9685 físicamente conectado.

**Estrategia (Black-box y White-box testing):**
*   Aquí se compila la librería compartida real (`libpca9685.so` usando el verdadero `i2c.c`).
*   **White-box:** Python llama al driver en C para establecer frecuencias y encender LEDs, y luego utiliza la función de diagnóstico de nuestra propia librería (`pca9685_read_channel_config`) para validar que el chip reteniendo esos valores.
*   **Black-box (Auditoría externa):** Se utiliza un paquete independiente (como `smbus2`) para asomarse al bus I2C por su cuenta. De esta forma, verificamos con un "juez externo e imparcial" que nuestra librería en C realmente alteró el estado físico de la manera que esperábamos.

---

## 5. Estructura de Archivos a Implementar

Se añadirán los siguientes componentes al repositorio:

```text
i2c-driver-pca9685/
├── ...
├── src/i2c/mock_i2c.c        # Sustituto para tests unitarios
├── tests/
│   ├── requirements.txt      # Dependencias Python (pytest, smbus2)
│   ├── conftest.py           # Configuración de pytest y carga de ctypes
│   ├── test_unit_logic.py    # Tests sin hardware (Matemática y Mocking)
│   └── test_integration.py   # Tests con hardware real (RPi 5 y PCA9685)
└── Makefile                  # Se agrega una regla `make test-lib` para compilar los .so
```
