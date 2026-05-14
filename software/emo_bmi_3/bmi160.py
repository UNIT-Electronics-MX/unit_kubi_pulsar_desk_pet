"""BMI160 constants in one file (all-in-one).

This file merges constants from:
- BMI160/commands.py
- BMI160/definitions.py
- BMI160/registers.py
"""

try:
    from micropython import const
except ImportError:
    # Fallback for CPython/testing.
    def const(x):
        return x

from time import sleep_ms
from struct import unpack


# command definitions
START_FOC = const(0x03)
ACC_MODE_NORMAL = const(0x11)
GYR_MODE_NORMAL = const(0x15)
FIFO_FLUSH = const(0xB0)
INT_RESET = const(0xB1)
STEP_CNT_CLR = const(0xB2)
SOFT_RESET = const(0xB6)


# bit field offsets and lengths
ACC_PMU_STATUS_BIT = const(4)
ACC_PMU_STATUS_LEN = const(2)
GYR_PMU_STATUS_BIT = const(2)
GYR_PMU_STATUS_LEN = const(2)
GYRO_RANGE_SEL_BIT = const(0)
GYRO_RANGE_SEL_LEN = const(3)
GYRO_RATE_SEL_BIT = const(0)
GYRO_RATE_SEL_LEN = const(4)
GYRO_DLPF_SEL_BIT = const(4)
GYRO_DLPF_SEL_LEN = const(2)
ACCEL_DLPF_SEL_BIT = const(4)
ACCEL_DLPF_SEL_LEN = const(3)
ACCEL_RANGE_SEL_BIT = const(0)
ACCEL_RANGE_SEL_LEN = const(4)


# Gyroscope sensitivity range options
GYRO_RANGE_2000 = const(0)  # +/- 2000 degrees/second
GYRO_RANGE_1000 = const(1)  # +/- 1000 degrees/second
GYRO_RANGE_500 = const(2)   # +/- 500 degrees/second
GYRO_RANGE_250 = const(3)   # +/- 250 degrees/second
GYRO_RANGE_125 = const(4)   # +/- 125 degrees/second


# Accelerometer sensitivity range options
ACCEL_RANGE_2G = const(0x03)   # +/- 2g range
ACCEL_RANGE_4G = const(0x05)   # +/- 4g range
ACCEL_RANGE_8G = const(0x08)   # +/- 8g range
ACCEL_RANGE_16G = const(0x0C)  # +/- 16g range

FOC_ACC_Z_BIT = const(0)
FOC_ACC_Z_LEN = const(2)
FOC_ACC_Y_BIT = const(2)
FOC_ACC_Y_LEN = const(2)
FOC_ACC_X_BIT = const(4)
FOC_ACC_X_LEN = const(2)
FOC_GYR_EN = const(6)


# register definitions
CHIP_ID = const(0x00)
PMU_STATUS = const(0x03)
GYRO_X_L = const(0x0C)
GYRO_X_H = const(0x0D)
GYRO_Y_L = const(0x0E)
GYRO_Y_H = const(0x0F)
GYRO_Z_L = const(0x10)
GYRO_Z_H = const(0x11)
ACCEL_X_L = const(0x12)
ACCEL_X_H = const(0x13)
ACCEL_Y_L = const(0x14)
ACCEL_Y_H = const(0x15)
ACCEL_Z_L = const(0x16)
ACCEL_Z_H = const(0x17)
STATUS = const(0x1B)
INT_STATUS_0 = const(0x1C)
INT_STATUS_1 = const(0x1D)
INT_STATUS_2 = const(0x1E)
INT_STATUS_3 = const(0x1F)
TEMP_L = const(0x20)
TEMP_H = const(0x21)
FIFO_LENGTH_0 = const(0x22)
FIFO_LENGTH_1 = const(0x23)
FIFO_DATA = const(0x24)
ACCEL_CONF = const(0x40)
ACCEL_RANGE = const(0x41)
GYRO_CONF = const(0x42)
GYRO_RANGE = const(0x43)
FIFO_CONFIG_0 = const(0x46)
FIFO_CONFIG_1 = const(0x47)
INT_EN_0 = const(0x50)
INT_EN_1 = const(0x51)
INT_EN_2 = const(0x52)
INT_OUT_CTRL = const(0x53)
INT_LATCH = const(0x54)
INT_MAP_0 = const(0x55)
INT_MAP_1 = const(0x56)
INT_MAP_2 = const(0x57)
INT_LOWHIGH_0 = const(0x5A)
INT_LOWHIGH_1 = const(0x5B)
INT_LOWHIGH_2 = const(0x5C)
INT_LOWHIGH_3 = const(0x5D)
INT_LOWHIGH_4 = const(0x5E)
INT_MOTION_0 = const(0x5F)
INT_MOTION_1 = const(0x60)
INT_MOTION_2 = const(0x61)
INT_MOTION_3 = const(0x62)
INT_TAP_0 = const(0x63)
INT_TAP_1 = const(0x64)
FOC_CONF = const(0x69)
OFFSET_0 = const(0x71)
OFFSET_1 = const(0x72)
OFFSET_2 = const(0x73)
OFFSET_3 = const(0x74)
OFFSET_4 = const(0x75)
OFFSET_5 = const(0x76)
OFFSET_6 = const(0x77)
STEP_CNT_L = const(0x78)
STEP_CNT_H = const(0x79)
STEP_CONF_0 = const(0x7A)
STEP_CONF_1 = const(0x7B)
STEP_CONF_0_NOR = const(0x15)
STEP_CONF_0_SEN = const(0x2D)
STEP_CONF_0_ROB = const(0x1D)
STEP_CONF_1_NOR = const(0x03)
STEP_CONF_1_SEN = const(0x00)
STEP_CONF_1_ROB = const(0x07)
CMD = const(0x7E)

DEFAULT_ADDR = const(0x69)


def init_bmi160(i2c, addr=DEFAULT_ADDR):
    i2c.writeto_mem(addr, CMD, bytes([SOFT_RESET]))
    sleep_ms(2)
    i2c.readfrom_mem(addr, 0x7F, 1)
    sleep_ms(2)
    i2c.writeto_mem(addr, CMD, bytes([ACC_MODE_NORMAL]))
    sleep_ms(50)
    i2c.writeto_mem(addr, CMD, bytes([GYR_MODE_NORMAL]))
    sleep_ms(100)
    i2c.writeto_mem(addr, GYRO_RANGE, bytes([GYRO_RANGE_250]))
    i2c.writeto_mem(addr, ACCEL_RANGE, bytes([ACCEL_RANGE_2G]))


def read_motion6(i2c, addr=DEFAULT_ADDR):
    gx, gy, gz, ax, ay, az = unpack("<hhhhhh", i2c.readfrom_mem(addr, GYRO_X_L, 12))
    return ax, ay, az, gx, gy, gz

