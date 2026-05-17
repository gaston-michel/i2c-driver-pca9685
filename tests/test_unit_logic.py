import pytest
from conftest import PCA9685

# Constantes del hardware a validar
PCA9685_MODE1 = 0x00
PCA9685_MODE2 = 0x01
PCA9685_PRESCALE = 0xFE
PCA9685_LED0_ON_L = 0x06

def test_init_sets_correct_registers(lib_mock):
    """Prueba que pca9685_init configure el modo Auto-Increment y Totem Pole."""
    dev = PCA9685()
    
    # 0 = PCA_SUCCESS
    status = lib_mock.pca9685_init(dev, b"/dev/i2c-1", 0x40)
    assert status == 0
    assert dev.is_initialized == True
    
    # Verificar MODE1 (0x00): Debe estar despierto y con Auto-Increment (0x20)
    mode1 = lib_mock.mock_i2c_get_reg(PCA9685_MODE1)
    assert (mode1 & 0x10) == 0, "El bit SLEEP (bit 4) debería estar en 0 (Despierto)"
    assert (mode1 & 0x20) != 0, "El bit AUTO_INCREMENT (bit 5) debería estar activado"
    
    # Verificar MODE2 (0x01): Debe tener Totem-Pole habilitado (0x04)
    mode2 = lib_mock.mock_i2c_get_reg(PCA9685_MODE2)
    assert (mode2 & 0x04) == 0x04, "El bit OUTDRV (bit 2) debería ser 1 para Totem-Pole"


def test_duty_cycle_math(lib_mock):
    """
    Prueba que un 50% de duty cycle se traduzca matemáticamente
    en exactamente 2048 ticks y se escriba correctamente en memoria I2C.
    """
    dev = PCA9685()
    lib_mock.pca9685_init(dev, b"/dev/i2c-1", 0x40)
    
    # Establecer canal 0 a 50.0% (50% de 4096 = 2048 = 0x0800 en Hexadecimal)
    status = lib_mock.pca9685_set_duty_cycle(dev, 0, 50.0)
    assert status == 0
    
    # El canal siempre arranca en ON_TICK = 0
    on_l = lib_mock.mock_i2c_get_reg(PCA9685_LED0_ON_L)
    on_h = lib_mock.mock_i2c_get_reg(PCA9685_LED0_ON_L + 1)
    
    # El OFF_TICK debería ser 2048 (0x0800 -> OFF_L = 0x00, OFF_H = 0x08)
    off_l = lib_mock.mock_i2c_get_reg(PCA9685_LED0_ON_L + 2)
    off_h = lib_mock.mock_i2c_get_reg(PCA9685_LED0_ON_L + 3)
    
    assert on_l == 0x00
    assert on_h == 0x00
    assert off_l == 0x00
    assert off_h == 0x08, "Matemática errónea: El byte alto de OFF_TICK debería ser 0x08 (2048)"


def test_prescale_math_for_50hz(lib_mock):
    """
    Prueba la matemática de pca9685_set_pwm_freq().
    A 50Hz, con reloj de 25MHz, la fórmula es (25M / (4096*50)) - 1 = 121 (0x79).
    """
    dev = PCA9685()
    lib_mock.pca9685_init(dev, b"/dev/i2c-1", 0x40)
    
    status = lib_mock.pca9685_set_pwm_freq(dev, 50.0)
    assert status == 0
    
    prescale = lib_mock.mock_i2c_get_reg(PCA9685_PRESCALE)
    assert prescale == 121, f"Fórmula errónea: Esperaba 121 (0x79), obtuvo {prescale}"
