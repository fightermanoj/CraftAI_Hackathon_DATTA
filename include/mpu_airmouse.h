#ifndef MPU_AIRMOUSE_H
#define MPU_AIRMOUSE_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define MPU_SDA_PIN 1
#define MPU_SCL_PIN 2

class AirMouseMotion {
private:
    Adafruit_MPU6050 mpu;
    bool isInitialized;
    float smoothX;
    float smoothY;
    float deadzone;
    float sensitivity;

public:
    AirMouseMotion() : isInitialized(false), smoothX(0), smoothY(0), deadzone(0.35f), sensitivity(12.0f) {}

    bool begin() {
        Wire.begin(MPU_SDA_PIN, MPU_SCL_PIN, 400000); // Fast 400kHz I2C bus
        
        // Try default I2C address 0x68 first, fallback to 0x69
        if (!mpu.begin(0x68, &Wire)) {
            Serial.println("[MPU6050] Address 0x68 not found, trying 0x69...");
            if (!mpu.begin(0x69, &Wire)) {
                Serial.println("[MPU6050] Could not find MPU6050 sensor at 0x68 or 0x69 on GPIO 4/5!");
                return false;
            }
            Serial.println("[MPU6050] Connected at address 0x69!");
        } else {
            Serial.println("[MPU6050] Connected at address 0x68!");
        }

        mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ); // Low-pass filter for smooth motion
        
        isInitialized = true;
        Serial.println("[MPU6050] MPU6050 Air Mouse Motion Tracking Initialized Successfully!");
        return true;
    }

    void setSensitivity(float sens) {
        sensitivity = sens;
    }

    void setDeadzone(float dz) {
        deadzone = dz;
    }

    bool getDeltaMotion(int8_t &outDeltaX, int8_t &outDeltaY) {
        if (!isInitialized) return false;

        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);

        // Gyroscope angular velocity around Z (Yaw/Roll) and X (Pitch) in rad/s
        float rawGyroZ = g.gyro.z; // Horizontal tilt
        float rawGyroX = g.gyro.x; // Vertical tilt

        // Apply deadzone filtering to remove minor hand tremors / steady resting jitter
        if (abs(rawGyroZ) < deadzone) rawGyroZ = 0.0f;
        if (abs(rawGyroX) < deadzone) rawGyroX = 0.0f;

        // Exponential smoothing (alpha = 0.35)
        smoothX = (smoothX * 0.65f) + (rawGyroZ * sensitivity * 0.35f);
        smoothY = (smoothY * 0.65f) + (rawGyroX * sensitivity * 0.35f);

        // Clamp delta values within int8 bounds (-127 to 127) for HID report
        int deltaX = (int)smoothX;
        int deltaY = (int)smoothY;

        deltaX = constrain(deltaX, -120, 120);
        deltaY = constrain(deltaY, -120, 120);

        outDeltaX = (int8_t)deltaX;
        outDeltaY = (int8_t)deltaY;

        return (outDeltaX != 0 || outDeltaY != 0);
    }
};

#endif // MPU_AIRMOUSE_H
