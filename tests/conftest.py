import ctypes
import os
import pytest

# Definición de las estructuras C correspondientes en Python usando ctypes
class I2CDevice(ctypes.Structure):
    _fields_ = [
        ("fd", ctypes.c_int),
        ("slave_addr", ctypes.c_uint8),
        ("device_path", ctypes.c_char * 64)
    ]

class PCA9685(ctypes.Structure):
    _fields_ = [
        ("i2c_dev", I2CDevice),
        ("is_initialized", ctypes.c_bool),
        ("current_freq", ctypes.c_float)
    ]

@pytest.fixture
def lib_mock():
    """
    Fixture que carga la librería compilada con el Mock de I2C.
    Sirve para probar la matemática y la lógica de registros sin hardware real.
    """
    lib_path = os.path.abspath(os.path.join(os.path.dirname(__file__), '../lib/libpca9685_mock.so'))
    
    try:
        lib = ctypes.CDLL(lib_path)
    except OSError:
        pytest.skip("Librería compartida libpca9685_mock.so no encontrada. Corre 'make test-lib' en un entorno Linux/WSL.")
    
    # Declarar firmas de funciones del driver (argtypes y restype)
    lib.pca9685_init.argtypes = [ctypes.POINTER(PCA9685), ctypes.c_char_p, ctypes.c_uint8]
    lib.pca9685_init.restype = ctypes.c_int
    
    lib.pca9685_set_duty_cycle.argtypes = [ctypes.POINTER(PCA9685), ctypes.c_uint8, ctypes.c_float]
    lib.pca9685_set_duty_cycle.restype = ctypes.c_int
    
    lib.pca9685_set_pwm_freq.argtypes = [ctypes.POINTER(PCA9685), ctypes.c_float]
    lib.pca9685_set_pwm_freq.restype = ctypes.c_int
    
    # Firmas exclusivas del Mock
    lib.mock_i2c_get_reg.argtypes = [ctypes.c_uint8]
    lib.mock_i2c_get_reg.restype = ctypes.c_uint8
    
    lib.mock_i2c_reset.argtypes = []
    lib.mock_i2c_reset.restype = None
    
    # Limpiar el mock de I2C antes de retornar la librería
    lib.mock_i2c_reset()
    
    return lib

@pytest.fixture
def lib_real():
    """
    Fixture que carga la librería compilada con el driver I2C real de Linux.
    Solo debe usarse si se corre el test en la Raspberry Pi.
    """
    lib_path = os.path.abspath(os.path.join(os.path.dirname(__file__), '../lib/libpca9685.so'))
    
    try:
        lib = ctypes.CDLL(lib_path)
    except OSError:
        pytest.skip("Librería libpca9685.so no encontrada o no estamos en entorno compatible. Corre 'make test-lib' en Raspberry.")
    
    lib.pca9685_init.argtypes = [ctypes.POINTER(PCA9685), ctypes.c_char_p, ctypes.c_uint8]
    lib.pca9685_init.restype = ctypes.c_int
    
    lib.pca9685_set_pwm_freq.argtypes = [ctypes.POINTER(PCA9685), ctypes.c_float]
    lib.pca9685_set_pwm_freq.restype = ctypes.c_int
    
    lib.pca9685_set_channel.argtypes = [ctypes.POINTER(PCA9685), ctypes.c_uint8, ctypes.c_uint16, ctypes.c_uint16]
    lib.pca9685_set_channel.restype = ctypes.c_int
    
    lib.pca9685_read_channel_config.argtypes = [ctypes.POINTER(PCA9685), ctypes.c_uint8, ctypes.POINTER(ctypes.c_uint16), ctypes.POINTER(ctypes.c_uint16)]
    lib.pca9685_read_channel_config.restype = ctypes.c_int
    
    lib.pca9685_set_duty_cycle.argtypes = [ctypes.POINTER(PCA9685), ctypes.c_uint8, ctypes.c_float]
    lib.pca9685_set_duty_cycle.restype = ctypes.c_int
    
    lib.pca9685_set_full_on.argtypes = [ctypes.POINTER(PCA9685), ctypes.c_uint8]
    lib.pca9685_set_full_on.restype = ctypes.c_int
    
    lib.pca9685_set_full_off.argtypes = [ctypes.POINTER(PCA9685), ctypes.c_uint8]
    lib.pca9685_set_full_off.restype = ctypes.c_int
    
    lib.pca9685_set_all_channels.argtypes = [ctypes.POINTER(PCA9685), ctypes.c_uint16, ctypes.c_uint16]
    lib.pca9685_set_all_channels.restype = ctypes.c_int
    
    lib.pca9685_close.argtypes = [ctypes.POINTER(PCA9685)]
    lib.pca9685_close.restype = ctypes.c_int
    
    return lib
