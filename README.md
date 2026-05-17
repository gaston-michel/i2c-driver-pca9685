# PCA9685 Linux User-Space I2C Driver

## 🎯 Definición Funcional

### 1. Propósito General
Una biblioteca en C, modular y reutilizable, que permite a aplicaciones en Linux controlar el generador PWM de 16 canales PCA9685 de manera segura, predecible y tolerante a fallos, comunicándose puramente en espacio de usuario a través del subsistema `/dev/i2c-*`.

### 2. Características Funcionales (Capacidades de la API)
La librería expone una interfaz que permite al desarrollador realizar las siguientes operaciones:

* **Gestión del Ciclo de Vida:**
  * Abre la conexión I2C especificando el bus y la dirección esclava del dispositivo.
  * Inicializa el hardware a un estado conocido (Software Reset y configuración de registros `MODE1`/`MODE2`).
  * Cierra la conexión limpiamente liberando descriptores de archivo.

* **Configuración Global:**
  * Modifica la frecuencia de trabajo del PWM (modificando el registro `PRE_SCALE`), manejando internamente la transición al modo *sleep* requerida por el hardware para aplicar este cambio.

* **Control Individual y Grupal de Canales:**
  * Establece un ciclo de trabajo (*duty cycle*) específico (0 a 4095) para un canal dado (0 a 15).
  * Fuerza el encendido total (Full ON) o apagado total (Full OFF) de un canal específico, omitiendo la señal PWM.
  * Establece retrasos de fase (*phase shift*) en las señales PWM para mitigar picos de consumo de corriente al encender múltiples cargas simultáneamente.

* **Lectura y Diagnóstico:**
  * Permite la lectura de los registros del dispositivo para verificar el estado actual de la configuración (funcionalidad crucial que habilita los tests de integración automatizados).

### 3. Robustez y Calidad de la Implementación
Para cumplir con los estándares de un desarrollo de nivel profesional, el driver garantiza:

* **Validación Estricta:** Toda función pública valida sus parámetros de entrada (ej. canales limitados a 0-15, valores de ticks dentro del rango 0-4095) antes de intentar cualquier operación de hardware.
* **Manejo de Errores Seguro:** El driver nunca provoca un *crash* (terminación abrupta o segmentation fault) en el programa que lo utiliza. Ante fallos físicos (cable desconectado, error de bus I2C) o parámetros inválidos, retorna códigos de error semánticos específicos (ej. `PCA9685_ERR_I2C_WRITE`, `PCA9685_ERR_INVALID_CHANNEL`).
* **Agnosticismo del Hardware Final:** El driver habla el idioma del chip (*frecuencia* y *ticks de PWM*), evitando abstracciones de alto nivel como "ángulos de un servo" o "luminosidad de un LED". Dichas abstracciones se delegan a capas superiores en los ejemplos de aplicación, manteniendo la librería core pura y genérica.

---

## 🚀 Compilación y Ejecución (Demo en Hardware)

El proyecto incluye un programa de demostración interactivo (`examples/main_demo.c`) que prueba las distintas funcionalidades del driver: control de encendido total (ideal para probar con un LED), configuración de ciclos de trabajo puro (PWM), y una capa de abstracción matemática añadida sobre la API para mover Servomotores SG90 utilizando grados reales (0° a 180°).

### Esquema de Conexión (Raspberry Pi 5)
Antes de ejecutar la demo, asegúrate de conectar el módulo PCA9685 de la siguiente manera:

| PCA9685 | Raspberry Pi 5 | Observaciones |
| :--- | :--- | :--- |
| **VCC** | 3.3V (Pin 1) | Alimentación lógica del chip. |
| **GND** | GND (Pin 6 o 9)| Masa lógica común. |
| **SDA** | GPIO 2 (Pin 3) | Datos I2C (`/dev/i2c-1`). |
| **SCL** | GPIO 3 (Pin 5) | Reloj I2C (`/dev/i2c-1`). |
| **OE** | GND | Habilita las salidas de forma permanente. |
| **V+** | Fuente Externa | **NO conectar a los 5V de la Raspberry Pi**. Usar fuente externa dedicada (ej. 5V 2A) para alimentar los servomotores y no quemar la Raspberry. |
| **GND** | Fuente Externa | **Importante:** Unir la masa de la fuente externa con la masa de la Raspberry Pi para cerrar el circuito. |

### Cómo correr la demo
1. **Transferir el código**: Copia el directorio completo de este proyecto a tu Raspberry Pi 5.
2. **Compilar**: Posiciónate en la carpeta raíz del proyecto y ejecuta Make:
   ```bash
   make
   ```
   Esto generará el binario compilado dentro de la carpeta `bin/` con el nombre `demo`.
3. **Ejecutar**: Corre el programa interactivo. Para evitar problemas de permisos con los buses I2C (`/dev/i2c-1`), es recomendable ejecutarlo con privilegios elevados:
   ```bash
   sudo ./bin/demo
   ```
   *Para visualizar la prueba física detallada, te recomendamos realizar las siguientes conexiones (nota: los LEDs siempre deben llevar una resistencia en serie de ~220Ω a 330Ω para no quemarlos):*
   * **Canal 0:** Un LED. En la demo se encenderá al 100% de brillo (prueba de *Full ON / Full OFF*).
   * **Canal 1:** Un LED. En la demo se encenderá de forma tenue al 25% de brillo (prueba de control por porcentaje / *Duty Cycle*).
   * **Canal 15:** Un Servomotor (ej. SG90). En la demo se moverá secuencialmente a 0°, 90° y 180° (prueba de capa matemática).
