#ifndef NEOTRELLIS_H
#define NEOTRELLIS_H

#include <cstdint>
#include <functional>

#define SEESAW_KEYPAD_EDGE_RISING 0x01
#define SEESAW_KEYPAD_EDGE_FALLING 0x02

struct keyEvent {
    union {
        struct {
            uint8_t EDGE : 2;
            uint8_t NUM : 6;
        } bit;
        uint8_t reg;
    };
};

using KeyCallback = std::function<void(keyEvent)>;

class NeoPixels {
public:
    NeoPixels();
    
    void setBrightness(uint8_t brightness);
    void setPixelColor(uint8_t index, uint32_t color);
    void clear();
    void show();
    
private:
    static constexpr uint8_t NUM_PIXELS = 32;
    uint32_t pixels_[NUM_PIXELS];
    uint8_t brightness_;
    
    void updateHardware();
};

class NeoTrellis {
public:
    NeoTrellis();
    
    bool begin(uint8_t addr = 0x2E);
    void read();
    
    void activateKey(uint8_t key, uint8_t edge);
    void registerCallback(uint8_t key, KeyCallback callback);
    
    NeoPixels pixels;
    
private:
    static constexpr uint8_t NUM_KEYS = 32;
    
    uint8_t i2cAddr_;
    KeyCallback callbacks_[NUM_KEYS];
    uint8_t keyStates_[NUM_KEYS];
    
    void pollKeys();
    bool readKeypad();
    void processKeyEvent(uint8_t key, uint8_t state);
    
    bool writeI2C(uint8_t reg, uint8_t* data, uint8_t len);
    bool readI2C(uint8_t reg, uint8_t* data, uint8_t len);
};