#include "NeoTrellis.h"
#include <cstring>

NeoPixels::NeoPixels() : brightness_(50) {
    clear();
}

void NeoPixels::setBrightness(uint8_t brightness) {
    brightness_ = brightness;
}

void NeoPixels::setPixelColor(uint8_t index, uint32_t color) {
    if (index < NUM_PIXELS) {
        pixels_[index] = color;
    }
}

void NeoPixels::clear() {
    std::memset(pixels_, 0, sizeof(pixels_));
}

void NeoPixels::show() {
    updateHardware();
}

void NeoPixels::updateHardware() {
    
}

NeoTrellis::NeoTrellis() : i2cAddr_(0x2E) {
    std::memset(keyStates_, 0, sizeof(keyStates_));
    for (auto& callback : callbacks_) {
        callback = nullptr;
    }
}

bool NeoTrellis::begin(uint8_t addr) {
    i2cAddr_ = addr;
    
    
    return true;
}

void NeoTrellis::read() {
    pollKeys();
}

void NeoTrellis::activateKey(uint8_t key, uint8_t edge) {
    if (key < NUM_KEYS) {
        
    }
}

void NeoTrellis::registerCallback(uint8_t key, KeyCallback callback) {
    if (key < NUM_KEYS) {
        callbacks_[key] = callback;
    }
}

void NeoTrellis::pollKeys() {
    static uint8_t lastStates[NUM_KEYS] = {0};
    
    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        uint8_t currentState = keyStates_[i];
        
        if (currentState != lastStates[i]) {
            if (callbacks_[i]) {
                keyEvent evt;
                evt.bit.NUM = i;
                
                if (currentState && !lastStates[i]) {
                    evt.bit.EDGE = SEESAW_KEYPAD_EDGE_RISING;
                    callbacks_[i](evt);
                } else if (!currentState && lastStates[i]) {
                    evt.bit.EDGE = SEESAW_KEYPAD_EDGE_FALLING;
                    callbacks_[i](evt);
                }
            }
            lastStates[i] = currentState;
        }
    }
}

bool NeoTrellis::readKeypad() {
    
    return true;
}

void NeoTrellis::processKeyEvent(uint8_t key, uint8_t state) {
    if (key < NUM_KEYS) {
        keyStates_[key] = state;
    }
}

bool NeoTrellis::writeI2C(uint8_t reg, uint8_t* data, uint8_t len) {
    
    return true;
}

bool NeoTrellis::readI2C(uint8_t reg, uint8_t* data, uint8_t len) {
    
    return true;
}