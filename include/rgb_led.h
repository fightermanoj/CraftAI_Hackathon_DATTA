#ifndef RGB_LED_H
#define RGB_LED_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// GPIO 48 WS2812 RGB LED configuration
#define LED_PIN 48
#define NUM_LEDS 1

enum SystemLEDState {
    LED_STATE_OFF,
    LED_STATE_PAIRING,      // Pulsing Blue - Bluetooth Pairing
    LED_STATE_CONNECTED,    // Solid Blue - Air Mouse Active
    LED_STATE_LISTENING,    // Solid Yellow - Mic Active / Voice Listening
    LED_STATE_LEFT_CLICK,   // Green Pulse - Left Click Executed
    LED_STATE_RIGHT_CLICK,  // Purple Pulse - Right Click Executed
    LED_STATE_DOUBLE_CLICK, // Cyan Pulse - Double Click Executed
    LED_STATE_SCROLL_UP,    // Orange Pulse - Scroll Up
    LED_STATE_SCROLL_DOWN,  // Magenta Pulse - Scroll Down
    LED_STATE_ERROR         // Red Flash - System Error
};

class RGBController {
private:
    Adafruit_NeoPixel strip;
    SystemLEDState currentState;
    unsigned long stateTimer;

public:
    RGBController() : strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800), currentState(LED_STATE_OFF), stateTimer(0) {}

    void begin() {
        strip.begin();
        strip.setBrightness(40); // Comfortable brightness
        setColor(0, 0, 0);
    }

    void setColor(uint8_t r, uint8_t g, uint8_t b) {
        strip.setPixelColor(0, strip.Color(r, g, b));
        strip.show();
    }

    void setVoiceFlickerRed(uint8_t brightness) {
        // Dynamically flicker RED LED proportionally to voice amplitude/volume
        strip.setPixelColor(0, strip.Color(brightness, 0, 0));
        strip.show();
    }

    void setState(SystemLEDState state) {
        currentState = state;
        stateTimer = millis();

        switch (currentState) {
            case LED_STATE_PAIRING:
                setColor(0, 50, 255); // Blue
                break;
            case LED_STATE_CONNECTED:
                setColor(0, 100, 255); // Solid Bright Blue
                break;
            case LED_STATE_LISTENING:
                setColor(255, 200, 0); // Solid Yellow
                break;
            case LED_STATE_LEFT_CLICK:
                setColor(0, 255, 60); // Bright Green
                break;
            case LED_STATE_RIGHT_CLICK:
                setColor(180, 0, 255); // Bright Purple
                break;
            case LED_STATE_DOUBLE_CLICK:
                setColor(0, 255, 255); // Cyan
                break;
            case LED_STATE_SCROLL_UP:
                setColor(255, 120, 0); // Orange
                break;
            case LED_STATE_SCROLL_DOWN:
                setColor(255, 0, 150); // Pink/Magenta
                break;
            case LED_STATE_ERROR:
                setColor(255, 0, 0); // Bright Red
                break;
            case LED_STATE_OFF:
            default:
                setColor(0, 0, 0);
                break;
        }
    }

    void update() {
        // Auto-revert transient click/scroll feedback states back to connected blue state after 400ms
        if ((currentState == LED_STATE_LEFT_CLICK || 
             currentState == LED_STATE_RIGHT_CLICK || 
             currentState == LED_STATE_DOUBLE_CLICK || 
             currentState == LED_STATE_SCROLL_UP || 
             currentState == LED_STATE_SCROLL_DOWN) && 
            (millis() - stateTimer > 400)) {
            setState(LED_STATE_CONNECTED);
        }
    }
};

extern RGBController rgbLed;

#endif // RGB_LED_H
