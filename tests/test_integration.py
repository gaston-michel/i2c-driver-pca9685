import pytest
import os
import ctypes
from conftest import PCA9685

# Este test se saltará automáticamente si no se detecta el bus I2C (si corres esto en Windows o sin hardware)
pytestmark = pytest.mark.skipif(
    not os.path.exists("/dev/i2c-1"), 
    reason="Hardware I2C real no disponible. Saltando tests de integración."
)

@pytest.fixture
def pca_device(lib_real):
    """
    Fixture de Setup & Teardown.
    Inicializa el PCA9685 antes de cada test y lo cierra limpiamente al finalizar.
    Esto garantiza que cada test sea completamente independiente del anterior.
    """
    dev = PCA9685()
    status = lib_real.pca9685_init(dev, b"/dev/i2c-1", 0x40)
    assert status == 0, "No se pudo comunicar con el chip físico. Revisa conexiones."
    
    yield dev
    
    lib_real.pca9685_close(dev)


# ---------------------------------------------------------------------------
# Test 1: Inicialización y configuración de frecuencia
# ---------------------------------------------------------------------------
def test_set_pwm_freq(lib_real, pca_device):
    """Verifica que la frecuencia PWM se aplique correctamente al registro PRE_SCALE."""
    status = lib_real.pca9685_set_pwm_freq(pca_device, 100.0)
    assert status == 0, "Fallo al configurar la frecuencia del PWM"
    
    # Auditoría externa con smbus2
    import smbus2
    bus = smbus2.SMBus(1)
    prescale_phys = bus.read_byte_data(0x40, 0xFE)
    bus.close()
    
    # Matemática: (25M / (4096 * 100)) - 1 = 60
    assert prescale_phys == 60, f"PRE_SCALE incorrecto: chip guardó {prescale_phys}, esperado 60"


# ---------------------------------------------------------------------------
# Test 2: Escritura y lectura cruzada de un canal (set_channel + read_channel_config)
# ---------------------------------------------------------------------------
def test_channel_readback_integrity(lib_real, pca_device):
    """
    Escribe valores asimétricos en un canal, los lee con nuestra API en C
    y los compara leyendo el bus con smbus2.
    """
    # 1. Escribir con nuestra librería
    status = lib_real.pca9685_set_channel(pca_device, 1, 102, 512)
    assert status == 0, "Fallo al escribir en el canal 1"
    
    # 2. Leer con nuestra API en C
    on_tick_c = ctypes.c_uint16(0)
    off_tick_c = ctypes.c_uint16(0)
    status = lib_real.pca9685_read_channel_config(pca_device, 1, ctypes.byref(on_tick_c), ctypes.byref(off_tick_c))
    assert status == 0, "Fallo al leer la configuración del canal 1"
    
    # 3. Leer con smbus2 (auditoría independiente)
    import smbus2
    bus = smbus2.SMBus(1)
    base_reg = 0x0A  # Registros del canal 1
    data = bus.read_i2c_block_data(0x40, base_reg, 4)
    bus.close()
        
    on_tick_smbus = data[0] | ((data[1] & 0x0F) << 8)
    off_tick_smbus = data[2] | ((data[3] & 0x0F) << 8)
    
    # 4. Aserciones cruzadas
    assert on_tick_c.value == 102, f"API C leyó ON_TICK={on_tick_c.value}, esperado 102"
    assert off_tick_c.value == 512, f"API C leyó OFF_TICK={off_tick_c.value}, esperado 512"
    assert on_tick_c.value == on_tick_smbus, "Discrepancia ON_TICK: C vs smbus2"
    assert off_tick_c.value == off_tick_smbus, "Discrepancia OFF_TICK: C vs smbus2"


# ---------------------------------------------------------------------------
# Test 3: Duty Cycle por porcentaje
# ---------------------------------------------------------------------------
def test_duty_cycle(lib_real, pca_device):
    """Verifica que set_duty_cycle(25%) calcule correctamente 1024 ticks."""
    status = lib_real.pca9685_set_duty_cycle(pca_device, 2, 25.0)
    assert status == 0, "Fallo al configurar duty cycle"
    
    on_tick = ctypes.c_uint16(0)
    off_tick = ctypes.c_uint16(0)
    status = lib_real.pca9685_read_channel_config(pca_device, 2, ctypes.byref(on_tick), ctypes.byref(off_tick))
    assert status == 0, "Fallo al leer el canal 2"
    assert off_tick.value == 1024, f"Duty Cycle 25% debería ser 1024 ticks, obtuvo {off_tick.value}"


# ---------------------------------------------------------------------------
# Test 4: Full ON
# ---------------------------------------------------------------------------
def test_full_on(lib_real, pca_device):
    """Verifica que set_full_on active la bandera especial Full ON (mapeada a 4096)."""
    status = lib_real.pca9685_set_full_on(pca_device, 3)
    assert status == 0, "Fallo al configurar Full ON"
    
    on_tick = ctypes.c_uint16(0)
    off_tick = ctypes.c_uint16(0)
    status = lib_real.pca9685_read_channel_config(pca_device, 3, ctypes.byref(on_tick), ctypes.byref(off_tick))
    assert status == 0, "Fallo al leer el canal 3"
    assert on_tick.value == 4096, f"Full ON debería mapear a 4096, obtuvo {on_tick.value}"


# ---------------------------------------------------------------------------
# Test 5: Full OFF
# ---------------------------------------------------------------------------
def test_full_off(lib_real, pca_device):
    """Verifica que set_full_off active la bandera especial Full OFF (mapeada a 4096)."""
    status = lib_real.pca9685_set_full_off(pca_device, 4)
    assert status == 0, "Fallo al configurar Full OFF"
    
    on_tick = ctypes.c_uint16(0)
    off_tick = ctypes.c_uint16(0)
    status = lib_real.pca9685_read_channel_config(pca_device, 4, ctypes.byref(on_tick), ctypes.byref(off_tick))
    assert status == 0, "Fallo al leer el canal 4"
    assert off_tick.value == 4096, f"Full OFF debería mapear a 4096, obtuvo {off_tick.value}"


# ---------------------------------------------------------------------------
# Test 6: Set All Channels (Broadcast)
# ---------------------------------------------------------------------------
def test_set_all_channels(lib_real, pca_device):
    """Verifica que set_all_channels aplique los mismos ticks a todos los canales."""
    status = lib_real.pca9685_set_all_channels(pca_device, 0, 2048)
    assert status == 0, "Fallo al escribir en todos los canales"
    
    # Verificar que múltiples canales recibieron el valor (muestreo en canal 0, 7 y 15)
    on_tick = ctypes.c_uint16(0)
    off_tick = ctypes.c_uint16(0)
    
    for ch in [0, 7, 15]:
        status = lib_real.pca9685_read_channel_config(pca_device, ch, ctypes.byref(on_tick), ctypes.byref(off_tick))
        assert status == 0, f"Fallo al leer el canal {ch}"
        assert off_tick.value == 2048, f"Canal {ch}: OFF_TICK es {off_tick.value}, esperado 2048"


# ---------------------------------------------------------------------------
# Test 7: Close (Ciclo de vida completo)
# ---------------------------------------------------------------------------
def test_close(lib_real):
    """
    Verifica que pca9685_close apague las salidas y cierre el bus correctamente.
    Este test no usa el fixture pca_device porque necesita controlar close manualmente.
    """
    dev = PCA9685()
    status = lib_real.pca9685_init(dev, b"/dev/i2c-1", 0x40)
    assert status == 0, "Fallo la inicialización"
    
    status = lib_real.pca9685_close(dev)
    assert status == 0, "Fallo al cerrar el dispositivo"
    assert dev.is_initialized == False, "El flag is_initialized debería ser False después de close()"
