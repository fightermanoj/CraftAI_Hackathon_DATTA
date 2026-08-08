# ♿ DATTA — Digital Assistive Technology for Touch-free Access

> **Hackathon Pitch:** *"75 million people globally cannot use a traditional keyboard or mouse due to ALS, cerebral palsy, spinal cord injuries, or stroke recovery. We built their complete computer access lifeline in 2 hours using **FirmGen**."*

An all-in-one wearable assistive technology device designed to provide complete, hands-free **Web Accessibility**. It combines an **MPU6050 Motion-based Bluetooth BLE Air Mouse** with **Offline ESP Voice Command Recognition** (ICS-43434 I2S Microphone) and **GPIO 48 WS2812 RGB Visual Feedback**.

---

## 💥 Impact

**75M people can't use a keyboard. This is their replacement.**

* 💵 **$10 Hardware**: Existing assistive mice cost $500+.
* ⚡ **Zero Install**: Standard Bluetooth HID, works on any PC, Mac, Android, or iOS device natively.
* 🔒 **Offline Voice**: No cloud, no WiFi required, zero privacy leaks.
* 🎯 **Tremor-Safe**: Exponential filter cleans involuntary hand tremors and extracts true motion intent.
* 🎙️ **Voice = Clicks**: Speak to left-click, right-click, double-click, and scroll seamlessly.
* 🎨 **RGB Feedback**: Instant visual confirmation light for every voice command and mic input.
* 🚀 **Built in 2 Hours with FirmGen**: Months of complex embedded firmware engineering, collapsed into 2 hours.

---

## 🚀 What FirmGen Generated

*I described my hardware pinouts and desired behavior to **FirmGen**, which generated the production-grade ESP-IDF firmware:*

1. **MPU6050 I2C Driver & Filter Layer**: Hardware initialization on GPIO 1 (SDA) & GPIO 2 (SCL), I2C communication, complementary filter math, and hand-tremor deadzone protection for smooth 2D cursor control.
2. **BLE HID Mouse Profile**: Complete GATT server configuration, HID report descriptors, mouse report structures, and seamless Bluetooth Low Energy pairing.
3. **I2S DMA Audio Capture Pipeline**: ICS-43434 / IM69D130 I2S digital microphone driver setup on GPIO 16 (BCLK), 17 (WS), 18 (DIN), 24-bit audio normalization, and real-time audio amplitude/pitch processing.
4. **Offline ESP Voice Command Recognition**: Pre-trained offline acoustic command parser mapping voice triggers directly to web accessibility actions (**Left Click**, **Right Click**, **Double Click**, and **Scroll Down**).
5. **Real-Time RGB LED Feedback**: WS2812 RGB LED controller on GPIO 48 providing live voice volume flickering (RED) and state-machine indication.
6. **FreeRTOS Task Architecture**: Multi-task orchestration (`TaskBLEMotion` at 50Hz and `TaskVoiceCommand` at 100Hz) ensuring concurrent motion tracking, audio capture, and BLE data transmission without latency.

---

## ⚡ Hardware Wiring Pinout

| Module / Sensor | Signal | ESP32 GPIO Pin | Function |
|---|---|---|---|
| **MPU6050 IMU** | `SDA` | **GPIO 1** | I2C Data Line (Wrist / Head Motion) |
| **MPU6050 IMU** | `SCL` | **GPIO 2** | I2C Clock Line |
| **MPU6050 IMU** | `VCC` / `GND` | `3.3V` / `GND` | Power Supply |
| **ICS-43434 Mic** | `BCLK` | **GPIO 16** | I2S Bit Clock |
| **ICS-43434 Mic** | `WS` | **GPIO 17** | I2S Word Select (Frame Sync) |
| **ICS-43434 Mic** | `DIN` / `SD` | **GPIO 18** | I2S Data Input |
| **ICS-43434 Mic** | `L/R` | `GND` | Left Channel Select |
| **ICS-43434 Mic** | `VDD` / `GND` | `3.3V` / `GND` | Power Supply |
| **WS2812 RGB LED** | `DATA` | **GPIO 48** | Visual RGB Status Indicator |

---

## 🎨 ESP-SR / MultiNet Voice Commands & RGB LED Feedback (GPIO 48)

| ESP-SR Module | Recognized Voice Command | Action Executed | LED Visual Effect |
|---|---|---|---|
| **Live Audio Input** | Speaking into Mic | Real-Time Mic Feedback | 🔴 **Dynamic RED Volume Flicker** |
| **WakeNet Engine** | `"Hi ESP"` / `"Alexa"` | Hands-Free Wake Trigger | 🟨 **Solid Yellow / Amber** |
| **MultiNet v7** | `"turn on the light"` / `"turn on"` | **Left Click** | 🟩 **Flash Bright Green** |
| **MultiNet v7** | `"turn off the light"` / `"turn off"` | **Right Click** | 🟪 **Flash Bright Purple** |
| **MultiNet v7** | `"play music"` / `"stop playing"` | **Double Click** | 🩵 **Flash Cyan** |
| **MultiNet v7** | `"increase volume"` | **Scroll Up** | 🟧 **Flash Orange** |
| **MultiNet v7** | `"decrease volume"` | **Scroll Down** | 🩷 **Flash Pink / Magenta** |

---

## 📹 Video Demonstration

Check out the full video demonstration and hardware walk-through:
🔗 **[Watch Hardware & Firmware Video Demo on Google Drive](https://drive.google.com/drive/folders/1w-h7adIn-JvgCB0AcG4T-cyu8TiBb393)**

---

## 🔮 Future Aspects: Adaptive Assistive HMI

To expand this project into a comprehensive **Adaptive Assistive Human-Machine Interface (HMI)** for the disabled population:

1. **On-Device Adaptive Intent Calibration**: Implement real-time continuous recalibration to automatically adjust gyro deadzones and voice sensitivity based on progressive motor conditions (e.g. advanced ALS or muscle fatigue).
2. **Multi-Sensor Fusion & Eye-Gaze Integration**: Integrate optical eye-gaze tracking and EMG muscle-impulse triggers for ventilator-dependent and non-verbal users.
3. **Multi-Lingual Offline Command Engine**: Expand offline ESP-SR MultiNet dictionary to support 15+ global languages natively without cloud dependency.
4. **Universal Assistive Accessibility Ecosystem**: Build open-source modular firmware extensions so any assistive hardware manufacturer can deploy custom wearable accessibility devices using FirmGen in minutes.
