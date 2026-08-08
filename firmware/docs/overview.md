# ESP32 Air Mouse

This firmware ports the repository's MPU6050 air-mouse concept to an ESP32-S3 DevKitM-1. It samples the MPU6050 over I2C, calibrates gyro bias at startup, converts hand rotation into relative mouse movement, and exposes a BLE HID mouse.

## Wiring

- MPU6050 VCC: 3.3 V
- MPU6050 GND: GND
- MPU6050 SDA: GPIO4
- MPU6050 SCL: GPIO5
- I2C: 400 kHz; use external pull-ups if the module does not provide them

## Use

Build with ESP-IDF 5.5, flash the project, then pair with the BLE device named `ESP32 Air Mouse`. Keep the sensor still during the startup calibration. Movement is sent after a BLE connection is established. The original repository's capacitive touch click pins are not ported because their physical GPIO mapping is not documented.

Application code is in `firmware/app`, the MPU6050 adapter is in `firmware/platforms/esp32/mpu6050.c`, BLE HID is in `firmware/platforms/esp32/ble_hid.c`, and motion processing is in `firmware/services/motion_service.c`.
