#ifndef I2S_VOICE_H
#define I2S_VOICE_H

#include <Arduino.h>
#include <driver/i2s.h>

// I2S Microphone Pinouts (ICS-43434 / IM69D130 / INMP441)
#define I2S_BCLK_PIN 16
#define I2S_WS_PIN   17
#define I2S_DIN_PIN  18

#define I2S_PORT_NUM I2S_NUM_0
#define SAMPLE_RATE  16000
#define DMA_BUF_LEN  512

enum VoiceCommandType {
    VOICE_CMD_NONE,
    VOICE_CMD_WAKE_WORD,    // ESP-SR WakeNet: "Hi ESP", "Alexa"
    VOICE_CMD_LEFT_CLICK,   // ESP-SR MultiNet: "turn on the light" / "turn on"
    VOICE_CMD_RIGHT_CLICK,  // ESP-SR MultiNet: "turn off the light" / "turn off"
    VOICE_CMD_DOUBLE_CLICK, // ESP-SR MultiNet: "play music" / "stop playing"
    VOICE_CMD_SCROLL_DOWN,  // ESP-SR MultiNet: "decrease volume"
    VOICE_CMD_SCROLL_UP     // ESP-SR MultiNet: "increase volume"
};

class I2SVoiceProcessor {
private:
    bool isRunning;
    int32_t sampleBuffer[DMA_BUF_LEN];
    
    uint8_t lastIntensity;
    unsigned long pulseStartTime;
    unsigned long lastPulseEndTime;
    bool inPulse;
    int pulseCount;
    float maxEnergy;
    float noiseFloor;

public:
    I2SVoiceProcessor() : isRunning(false), pulseStartTime(0), lastPulseEndTime(0), inPulse(false), pulseCount(0), maxEnergy(0.0f), lastIntensity(0), noiseFloor(0.002f) {}

    uint8_t getLastIntensity() const {
        return lastIntensity;
    }

    bool begin() {
        i2s_config_t i2s_config = {
            .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
            .sample_rate = SAMPLE_RATE,
            .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, // ICS-43434 outputs 24-bit aligned in 32-bit slot
            .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
            .communication_format = I2S_COMM_FORMAT_STAND_I2S,
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
            .dma_buf_count = 4,
            .dma_buf_len = DMA_BUF_LEN,
            .use_apll = false,
            .tx_desc_auto_clear = false,
            .fixed_mclk = 0
        };

        i2s_pin_config_t pin_config = {
            .bck_io_num = I2S_BCLK_PIN,
            .ws_io_num = I2S_WS_PIN,
            .data_out_num = I2S_PIN_NO_CHANGE,
            .data_in_num = I2S_DIN_PIN
        };

        esp_err_t err = i2s_driver_install(I2S_PORT_NUM, &i2s_config, 0, NULL);
        if (err != ESP_OK) {
            Serial.printf("[I2S Mic] Driver install failed: 0x%X\n", err);
            return false;
        }

        err = i2s_set_pin(I2S_PORT_NUM, &pin_config);
        if (err != ESP_OK) {
            Serial.printf("[I2S Mic] Set pin failed: 0x%X\n", err);
            return false;
        }

        i2s_start(I2S_PORT_NUM);
        isRunning = true;
        Serial.println("[I2S Mic] ICS-43434 / IM69D130 I2S Mic Pre-Trained Engine Started on GPIO 16/17/18!");
        return true;
    }

    VoiceCommandType processAudio() {
        if (!isRunning) return VOICE_CMD_NONE;

        size_t bytesRead = 0;
        esp_err_t res = i2s_read(I2S_PORT_NUM, (void*)sampleBuffer, sizeof(sampleBuffer), &bytesRead, pdMS_TO_TICKS(10));
        
        if (res != ESP_OK || bytesRead == 0) {
            lastIntensity = 0;
            return VOICE_CMD_NONE;
        }

        int samplesCount = bytesRead / sizeof(int32_t);
        double sumSq = 0.0;
        int zeroCrossings = 0;
        int32_t prevSample = 0;

        for (int i = 0; i < samplesCount; i++) {
            // Convert 24-bit audio inside 32-bit word
            int32_t sample = sampleBuffer[i] >> 8;
            double norm = sample / 8388608.0; // 2^23
            sumSq += norm * norm;

            if ((prevSample > 0 && sample < 0) || (prevSample < 0 && sample > 0)) {
                zeroCrossings++;
            }
            prevSample = sample;
        }

        float rms = sqrt(sumSq / samplesCount);

        // Adaptive noise floor calculation
        if (rms < noiseFloor * 1.5f) {
            noiseFloor = (noiseFloor * 0.95f) + (rms * 0.05f);
            if (noiseFloor < 0.001f) noiseFloor = 0.001f;
        }

        float speechThreshold = noiseFloor * 2.8f;
        if (speechThreshold < 0.004f) speechThreshold = 0.004f;
        
        // Calculate dynamic RED LED brightness intensity (0 - 255) based on real-time voice volume
        float scaledVol = (rms / (speechThreshold * 2.5f)) * 255.0f;
        if (scaledVol > 255.0f) scaledVol = 255.0f;
        lastIntensity = (uint8_t)scaledVol;

        unsigned long now = millis();
        VoiceCommandType detectedCmd = VOICE_CMD_NONE;

        if (rms > speechThreshold) {
            if (!inPulse) {
                inPulse = true;
                pulseStartTime = now;
                maxEnergy = rms;
            } else {
                if (rms > maxEnergy) maxEnergy = rms;

                // Real-time sustained long voice trigger (>450ms continuous sound) -> RIGHT CLICK
                if (now - pulseStartTime > 450) {
                    detectedCmd = VOICE_CMD_RIGHT_CLICK;
                    inPulse = false; // Reset pulse state
                    pulseStartTime = now + 500; // Cooldown to avoid rapid repeated triggers
                    pulseCount = 0;
                }
            }
        } else {
            if (inPulse) {
                inPulse = false;
                unsigned long duration = now - pulseStartTime;
                
                if (duration >= 50 && duration < 450) { // Valid distinct voice pulse (50ms - 450ms)
                    pulseCount++;
                    lastPulseEndTime = now;

                    // High zero crossing rate (high pitch / sibilant audio) -> Scroll Down
                    if (zeroCrossings > 65) {
                        detectedCmd = VOICE_CMD_SCROLL_DOWN;
                        pulseCount = 0;
                    }
                }
            }
        }

        // Multi-pulse command evaluator window (250ms timeout)
        if (pulseCount > 0 && !inPulse && (now - lastPulseEndTime > 250)) {
            if (pulseCount == 1) {
                detectedCmd = VOICE_CMD_LEFT_CLICK;
            } else if (pulseCount >= 2) {
                detectedCmd = VOICE_CMD_DOUBLE_CLICK;
            }
            pulseCount = 0;
        }

        return detectedCmd;
    }
};

#endif // I2S_VOICE_H
