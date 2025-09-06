/*
 * NeoTrellisM4.h - Driver library for NeoTrellis M4 I2C peripheral
 * 
 * Provides high-level interface for controlling NeoTrellis M4 hardware
 * from a Teensy 4.1 or other I2C host controller.
 */

#ifndef NEOTRELLIS_M4_H
#define NEOTRELLIS_M4_H

#include <Arduino.h>
#include <Wire.h>

class NeoTrellisM4 {
public:
    // I2C Commands
    enum Commands {
        CMD_SET_PIXEL = 0x01,
        CMD_SET_ALL_PIXELS = 0x02,
        CMD_SET_BRIGHTNESS = 0x03,
        CMD_SHOW_PIXELS = 0x04,
        CMD_GET_BUTTONS = 0x10,
        CMD_GET_BUTTON = 0x11,
        CMD_GET_EVENTS = 0x12,
        CMD_PING = 0x20,
        CMD_GET_VERSION = 0x21
    };
    
    // Button event types
    enum ButtonEvent {
        BUTTON_PRESSED = 1,
        BUTTON_RELEASED = 2
    };
    
    // Constructor
    NeoTrellisM4(TwoWire &wire = Wire);
    
    // Initialization
    bool begin(uint8_t addr = 0x60);
    bool ping();
    void getVersion(uint8_t &major, uint8_t &minor);
    
    // LED Control
    void setPixel(uint8_t index, uint32_t color);
    void setPixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
    void setPixelColor(uint8_t index, uint8_t r, uint8_t g, uint8_t b) { setPixel(index, r, g, b); }
    void setAllPixels(uint32_t color);
    void setAllPixels(uint8_t r, uint8_t g, uint8_t b);
    void setBrightness(uint8_t brightness);
    void show();
    void clear();
    
    // Button Reading
    void update(); // Call frequently to poll buttons
    bool isPressed(uint8_t key);
    bool wasPressed(uint8_t key);
    bool wasReleased(uint8_t key);
    uint32_t getButtonState();
    
    // Utility functions
    static uint32_t Color(uint8_t r, uint8_t g, uint8_t b);
    static uint32_t Wheel(uint8_t wheelPos);
    
    // Callbacks
    typedef void (*ButtonCallback)(uint8_t key, uint8_t event);
    void setButtonCallback(ButtonCallback callback) { _buttonCallback = callback; }
    
private:
    TwoWire *_wire;
    uint8_t _i2cAddr;
    bool _initialized;
    
    // Button state tracking
    uint32_t _currentButtonState;
    uint32_t _previousButtonState;
    uint32_t _buttonPressEvents;
    uint32_t _buttonReleaseEvents;
    
    // LED state
    uint8_t _brightness;
    uint32_t _pixelBuffer[32];
    
    // Callback
    ButtonCallback _buttonCallback;
    
    // I2C communication helpers
    bool writeCommand(uint8_t cmd);
    bool writeCommand(uint8_t cmd, uint8_t data);
    bool writeCommand(uint8_t cmd, uint8_t *data, uint8_t len);
    uint8_t readResponse(uint8_t *buffer, uint8_t maxLen);
    
    // Internal helpers
    void updateButtonEvents();
};

#endif // NEOTRELLIS_M4_H