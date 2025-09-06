/*
 * NeoTrellisM4.cpp - Implementation of NeoTrellis M4 I2C driver
 */

#include "NeoTrellisM4.h"

NeoTrellisM4::NeoTrellisM4(TwoWire &wire) 
    : _wire(&wire), 
      _i2cAddr(0), 
      _initialized(false),
      _currentButtonState(0),
      _previousButtonState(0),
      _buttonPressEvents(0),
      _buttonReleaseEvents(0),
      _brightness(50),
      _buttonCallback(nullptr) {
    
    // Initialize pixel buffer
    for (int i = 0; i < 32; i++) {
        _pixelBuffer[i] = 0;
    }
}

bool NeoTrellisM4::begin(uint8_t addr) {
    _i2cAddr = addr;
    
    // Test communication with ping
    if (!ping()) {
        return false;
    }
    
    // Clear all LEDs
    clear();
    show();
    
    _initialized = true;
    return true;
}

bool NeoTrellisM4::ping() {
    uint8_t response;
    
    if (!writeCommand(CMD_PING)) {
        return false;
    }
    
    delay(1); // Small delay for peripheral to prepare response
    
    _wire->requestFrom(_i2cAddr, (uint8_t)1);
    if (_wire->available()) {
        response = _wire->read();
        return (response == 0xAA);
    }
    
    return false;
}

void NeoTrellisM4::getVersion(uint8_t &major, uint8_t &minor) {
    if (!writeCommand(CMD_GET_VERSION)) {
        major = 0;
        minor = 0;
        return;
    }
    
    delay(1);
    
    _wire->requestFrom(_i2cAddr, (uint8_t)2);
    if (_wire->available() >= 2) {
        major = _wire->read();
        minor = _wire->read();
    } else {
        major = 0;
        minor = 0;
    }
}

void NeoTrellisM4::setPixel(uint8_t index, uint32_t color) {
    if (index >= 32) return;
    
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    
    setPixel(index, r, g, b);
}

void NeoTrellisM4::setPixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (index >= 32) return;
    
    _pixelBuffer[index] = Color(r, g, b);
    
    uint8_t data[5];
    data[0] = CMD_SET_PIXEL;
    data[1] = index;
    data[2] = r;
    data[3] = g;
    data[4] = b;
    
    _wire->beginTransmission(_i2cAddr);
    _wire->write(data, 5);
    _wire->endTransmission();
}

void NeoTrellisM4::setAllPixels(uint32_t color) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    
    setAllPixels(r, g, b);
}

void NeoTrellisM4::setAllPixels(uint8_t r, uint8_t g, uint8_t b) {
    uint8_t data[97]; // 1 command byte + 96 RGB bytes
    data[0] = CMD_SET_ALL_PIXELS;
    
    for (int i = 0; i < 32; i++) {
        data[1 + (i * 3)] = r;
        data[2 + (i * 3)] = g;
        data[3 + (i * 3)] = b;
        _pixelBuffer[i] = Color(r, g, b);
    }
    
    // I2C has a 32-byte buffer limit on many platforms
    // Send in chunks if needed
    _wire->beginTransmission(_i2cAddr);
    _wire->write(CMD_SET_ALL_PIXELS);
    
    // Send RGB data in chunks
    for (int i = 0; i < 96; i += 30) {
        int chunkSize = min(30, 96 - i);
        _wire->write(&data[1 + i], chunkSize);
    }
    
    _wire->endTransmission();
}

void NeoTrellisM4::setBrightness(uint8_t brightness) {
    _brightness = brightness;
    
    uint8_t data[2];
    data[0] = CMD_SET_BRIGHTNESS;
    data[1] = brightness;
    
    _wire->beginTransmission(_i2cAddr);
    _wire->write(data, 2);
    _wire->endTransmission();
}

void NeoTrellisM4::show() {
    writeCommand(CMD_SHOW_PIXELS);
}

void NeoTrellisM4::clear() {
    setAllPixels(0, 0, 0);
}

void NeoTrellisM4::update() {
    // Request button events
    if (!writeCommand(CMD_GET_EVENTS)) {
        return;
    }
    
    delay(1); // Small delay for peripheral to prepare response
    
    // Read 8 bytes: 4 for press events, 4 for release events
    _wire->requestFrom(_i2cAddr, (uint8_t)8);
    
    if (_wire->available() >= 8) {
        // Read press events
        _buttonPressEvents = 0;
        _buttonPressEvents |= ((uint32_t)_wire->read() << 0);
        _buttonPressEvents |= ((uint32_t)_wire->read() << 8);
        _buttonPressEvents |= ((uint32_t)_wire->read() << 16);
        _buttonPressEvents |= ((uint32_t)_wire->read() << 24);
        
        // Read release events
        _buttonReleaseEvents = 0;
        _buttonReleaseEvents |= ((uint32_t)_wire->read() << 0);
        _buttonReleaseEvents |= ((uint32_t)_wire->read() << 8);
        _buttonReleaseEvents |= ((uint32_t)_wire->read() << 16);
        _buttonReleaseEvents |= ((uint32_t)_wire->read() << 24);
        
        // Update current state based on events
        _currentButtonState |= _buttonPressEvents;
        _currentButtonState &= ~_buttonReleaseEvents;
        
        // Call callbacks for events
        if (_buttonCallback) {
            for (uint8_t i = 0; i < 32; i++) {
                if (_buttonPressEvents & (1UL << i)) {
                    _buttonCallback(i, BUTTON_PRESSED);
                }
                if (_buttonReleaseEvents & (1UL << i)) {
                    _buttonCallback(i, BUTTON_RELEASED);
                }
            }
        }
    }
}

bool NeoTrellisM4::isPressed(uint8_t key) {
    if (key >= 32) return false;
    return (_currentButtonState & (1UL << key)) != 0;
}

bool NeoTrellisM4::wasPressed(uint8_t key) {
    if (key >= 32) return false;
    bool pressed = (_buttonPressEvents & (1UL << key)) != 0;
    if (pressed) {
        _buttonPressEvents &= ~(1UL << key); // Clear the event
    }
    return pressed;
}

bool NeoTrellisM4::wasReleased(uint8_t key) {
    if (key >= 32) return false;
    bool released = (_buttonReleaseEvents & (1UL << key)) != 0;
    if (released) {
        _buttonReleaseEvents &= ~(1UL << key); // Clear the event
    }
    return released;
}

uint32_t NeoTrellisM4::getButtonState() {
    return _currentButtonState;
}

uint32_t NeoTrellisM4::Color(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

uint32_t NeoTrellisM4::Wheel(uint8_t wheelPos) {
    wheelPos = 255 - wheelPos;
    if (wheelPos < 85) {
        return Color(255 - wheelPos * 3, 0, wheelPos * 3);
    }
    if (wheelPos < 170) {
        wheelPos -= 85;
        return Color(0, wheelPos * 3, 255 - wheelPos * 3);
    }
    wheelPos -= 170;
    return Color(wheelPos * 3, 255 - wheelPos * 3, 0);
}

bool NeoTrellisM4::writeCommand(uint8_t cmd) {
    _wire->beginTransmission(_i2cAddr);
    _wire->write(cmd);
    return (_wire->endTransmission() == 0);
}

bool NeoTrellisM4::writeCommand(uint8_t cmd, uint8_t data) {
    _wire->beginTransmission(_i2cAddr);
    _wire->write(cmd);
    _wire->write(data);
    return (_wire->endTransmission() == 0);
}

bool NeoTrellisM4::writeCommand(uint8_t cmd, uint8_t *data, uint8_t len) {
    _wire->beginTransmission(_i2cAddr);
    _wire->write(cmd);
    _wire->write(data, len);
    return (_wire->endTransmission() == 0);
}

uint8_t NeoTrellisM4::readResponse(uint8_t *buffer, uint8_t maxLen) {
    uint8_t bytesRead = 0;
    
    _wire->requestFrom(_i2cAddr, maxLen);
    
    while (_wire->available() && bytesRead < maxLen) {
        buffer[bytesRead++] = _wire->read();
    }
    
    return bytesRead;
}