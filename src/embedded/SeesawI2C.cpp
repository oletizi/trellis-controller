#include "SeesawI2C.h"

SeesawI2C::SeesawI2C(uint8_t address) 
    : address_(address), initialized_(false) {
}

bool SeesawI2C::begin() {
    if (initialized_) return true;
    
    // TODO: Initialize SAMD51 I2C/SERCOM hardware
    // For now, just mark as initialized
    initialized_ = true;
    
    return true;
}

SeesawI2C::Result SeesawI2C::writeRegister(Seesaw::ModuleBase module, uint8_t reg, uint8_t value) {
    return writeRegister(module, reg, &value, 1);
}

SeesawI2C::Result SeesawI2C::writeRegister(Seesaw::ModuleBase module, uint8_t reg, const uint8_t* data, uint8_t len) {
    if (!initialized_) return BUS_ERROR;
    
    // Seesaw protocol: [MODULE_BASE, REGISTER, ...DATA]
    uint8_t buffer[32]; // Max reasonable packet size
    if (len + 2 > sizeof(buffer)) return BUS_ERROR;
    
    buffer[0] = static_cast<uint8_t>(module);
    buffer[1] = reg;
    for (uint8_t i = 0; i < len; i++) {
        buffer[2 + i] = data[i];
    }
    
    return i2cWrite(buffer, 2 + len);
}

SeesawI2C::Result SeesawI2C::readRegister(Seesaw::ModuleBase module, uint8_t reg, uint8_t* value) {
    return readRegister(module, reg, value, 1);
}

SeesawI2C::Result SeesawI2C::readRegister(Seesaw::ModuleBase module, uint8_t reg, uint8_t* data, uint8_t len) {
    if (!initialized_) return BUS_ERROR;
    
    // Seesaw protocol: Write [MODULE_BASE, REGISTER], then read response
    uint8_t writeBuffer[2] = {
        static_cast<uint8_t>(module),
        reg
    };
    
    return i2cWriteRead(writeBuffer, 2, data, len);
}

// NeoPixel convenience methods
SeesawI2C::Result SeesawI2C::setNeoPixelPin(uint8_t pin) {
    return writeRegister(Seesaw::NEOPIXEL, Seesaw::NEOPIXEL_PIN, pin);
}

SeesawI2C::Result SeesawI2C::setNeoPixelLength(uint8_t length) {
    return writeRegister(Seesaw::NEOPIXEL, Seesaw::NEOPIXEL_BUF_LENGTH, length);
}

SeesawI2C::Result SeesawI2C::setNeoPixelBuffer(uint8_t offset, const uint8_t* data, uint8_t len) {
    // For buffer writes, we need to include the offset in the data
    uint8_t buffer[64]; // Reasonable max for LED data
    if (len + 1 > sizeof(buffer)) return BUS_ERROR;
    
    buffer[0] = offset;
    for (uint8_t i = 0; i < len; i++) {
        buffer[1 + i] = data[i];
    }
    
    return writeRegister(Seesaw::NEOPIXEL, Seesaw::NEOPIXEL_BUF, buffer, len + 1);
}

SeesawI2C::Result SeesawI2C::showNeoPixels() {
    return writeRegister(Seesaw::NEOPIXEL, Seesaw::NEOPIXEL_SHOW, 0x00);
}

// Keypad convenience methods
SeesawI2C::Result SeesawI2C::readKeypadCount(uint8_t* count) {
    return readRegister(Seesaw::KEYPAD, Seesaw::KEYPAD_COUNT, count);
}

SeesawI2C::Result SeesawI2C::readKeypadFIFO(uint8_t* data, uint8_t maxLen) {
    return readRegister(Seesaw::KEYPAD, Seesaw::KEYPAD_FIFO, data, maxLen);
}

SeesawI2C::Result SeesawI2C::enableKeypadInterrupt() {
    uint8_t mask = 0x01; // Enable keypad interrupt
    return writeRegister(Seesaw::KEYPAD, Seesaw::KEYPAD_INTENSET, mask);
}

// Low-level I2C stubs (to be implemented with SAMD51 SERCOM)
SeesawI2C::Result SeesawI2C::i2cWrite(const uint8_t* data, uint8_t len) {
    // TODO: Implement actual I2C write using SAMD51 SERCOM
    (void)data;
    (void)len;
    return SUCCESS;
}

SeesawI2C::Result SeesawI2C::i2cRead(uint8_t* data, uint8_t len) {
    // TODO: Implement actual I2C read using SAMD51 SERCOM
    (void)data;
    (void)len;
    return SUCCESS;
}

SeesawI2C::Result SeesawI2C::i2cWriteRead(const uint8_t* writeData, uint8_t writeLen, 
                                          uint8_t* readData, uint8_t readLen) {
    // TODO: Implement actual I2C write-read using SAMD51 SERCOM
    (void)writeData;
    (void)writeLen;
    (void)readData;
    (void)readLen;
    return SUCCESS;
}