#include <Arduino.h>
#include <BleMouse.h>
#include "rgb_led.h"
#include "mpu_airmouse.h"
#include "i2s_voice.h"

// Define Global Objects
BleMouse bleMouse("Assistive Air Mouse", "Web-Accessibility-Tech", 100);
RGBController rgbLed;
AirMouseMotion airMouse;
I2SVoiceProcessor voiceProc;

// FreeRTOS Task Handles
TaskHandle_t TaskBLEMotionHandle = NULL;
TaskHandle_t TaskVoiceCommandHandle = NULL;

// Multi-task 1: MPU6050 Motion Tracking & BLE HID Mouse Handler (50Hz)
void TaskBLEMotion(void *pvParameters) {
    (void) pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(20); // 50 Hz refresh rate

    int8_t deltaX = 0;
    int8_t deltaY = 0;

    for (;;) {
        if (bleMouse.isConnected()) {
            // Read MPU6050 gyro tilt and send mouse move report
            if (airMouse.getDeltaMotion(deltaX, deltaY)) {
                bleMouse.move(deltaX, deltaY);
            }
        }
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Multi-task 2: I2S Audio Sampling & Voice Command Recognition Task
void TaskVoiceCommand(void *pvParameters) {
    (void) pvParameters;

    for (;;) {
        VoiceCommandType cmd = voiceProc.processAudio();
        uint8_t intensity = voiceProc.getLastIntensity();

        // Real-time audio flicker on GPIO 48 RED LED when speaking into the mic
        if (intensity > 4) {
            rgbLed.setVoiceFlickerRed(intensity);
        }

        if (cmd != VOICE_CMD_NONE && bleMouse.isConnected()) {
            switch (cmd) {
                case VOICE_CMD_WAKE_WORD:
                    Serial.println("[ESP-SR WakeNet] Wake Word Recognized ('Hi ESP' / 'Alexa')!");
                    rgbLed.setState(LED_STATE_LISTENING); // Yellow LED Active Listening
                    vTaskDelay(pdMS_TO_TICKS(400));
                    break;

                case VOICE_CMD_LEFT_CLICK:
                    Serial.println("[ESP-SR MultiNet] Command Recognized: 'turn on the light' -> Executing LEFT CLICK!");
                    bleMouse.click(MOUSE_LEFT);
                    rgbLed.setState(LED_STATE_LEFT_CLICK); // Flash Green
                    vTaskDelay(pdMS_TO_TICKS(300));
                    break;

                case VOICE_CMD_RIGHT_CLICK:
                    Serial.println("[ESP-SR MultiNet] Command Recognized: 'turn off the light' -> Executing RIGHT CLICK!");
                    bleMouse.click(MOUSE_RIGHT);
                    rgbLed.setState(LED_STATE_RIGHT_CLICK); // Flash Purple
                    vTaskDelay(pdMS_TO_TICKS(300));
                    break;

                case VOICE_CMD_DOUBLE_CLICK:
                    Serial.println("[ESP-SR MultiNet] Command Recognized: 'play music' -> Executing DOUBLE CLICK!");
                    bleMouse.click(MOUSE_LEFT);
                    vTaskDelay(pdMS_TO_TICKS(100));
                    bleMouse.click(MOUSE_LEFT);
                    rgbLed.setState(LED_STATE_DOUBLE_CLICK); // Flash Cyan
                    vTaskDelay(pdMS_TO_TICKS(300));
                    break;

                case VOICE_CMD_SCROLL_DOWN:
                    Serial.println("[ESP-SR MultiNet] Command Recognized: 'decrease volume' -> Executing SCROLL DOWN!");
                    bleMouse.move(0, 0, -3); // Scroll wheel down
                    rgbLed.setState(LED_STATE_SCROLL_DOWN); // Flash Magenta
                    vTaskDelay(pdMS_TO_TICKS(300));
                    break;

                case VOICE_CMD_SCROLL_UP:
                    Serial.println("[ESP-SR MultiNet] Command Recognized: 'increase volume' -> Executing SCROLL UP!");
                    bleMouse.move(0, 0, 3); // Scroll wheel up
                    rgbLed.setState(LED_STATE_SCROLL_UP); // Flash Orange
                    vTaskDelay(pdMS_TO_TICKS(300));
                    break;

                default:
                    break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("=================================================");
    Serial.println(" Wearable Web Accessibility Air-Mouse + Voice   ");
    Serial.println(" Pre-Trained Voice Commands & BLE HID Mouse     ");
    Serial.println("=================================================");

    // 1. Initialize WS2812 RGB LED on GPIO 48
    rgbLed.begin();
    rgbLed.setState(LED_STATE_PAIRING); // Blue indication while advertising

    // 2. Start BLE Mouse HID Server
    bleMouse.begin();
    Serial.println("[BLE] BLE HID Mouse initialized. Advertising 'Assistive Air Mouse'...");

    // 3. Initialize MPU6050 on I2C GPIO 4 (SDA) & GPIO 5 (SCL)
    if (!airMouse.begin()) {
        Serial.println("[ERROR] Failed to start MPU6050!");
        rgbLed.setState(LED_STATE_ERROR);
    }

    // 4. Initialize ICS-43434 / IM69D130 I2S Mic on GPIO 16 (BCLK), 17 (WS), 18 (DIN)
    if (!voiceProc.begin()) {
        Serial.println("[ERROR] Failed to start I2S Microphone!");
        rgbLed.setState(LED_STATE_ERROR);
    }

    // 5. Create FreeRTOS Tasks for Concurrent Execution
    xTaskCreatePinnedToCore(
        TaskBLEMotion,
        "TaskBLEMotion",
        4096,
        NULL,
        2, // High priority for smooth cursor tracking
        &TaskBLEMotionHandle,
        1
    );

    xTaskCreatePinnedToCore(
        TaskVoiceCommand,
        "TaskVoiceCommand",
        8192,
        NULL,
        1, // Normal priority for audio processing
        &TaskVoiceCommandHandle,
        0
    );

    Serial.println("[System] All background tasks initialized successfully!");
}

void loop() {
    static bool prevConnectStatus = false;
    bool currConnectStatus = bleMouse.isConnected();

    // Check BLE Connection State transitions for LED Feedback
    if (currConnectStatus != prevConnectStatus) {
        if (currConnectStatus) {
            Serial.println("[BLE] Bluetooth Connected to Host Device!");
            rgbLed.setState(LED_STATE_CONNECTED); // Solid Blue
        } else {
            Serial.println("[BLE] Bluetooth Disconnected. Re-advertising...");
            rgbLed.setState(LED_STATE_PAIRING); // Pulsing Blue
        }
        prevConnectStatus = currConnectStatus;
    }

    // Update LED state machine
    rgbLed.update();

    vTaskDelay(pdMS_TO_TICKS(50));
}
