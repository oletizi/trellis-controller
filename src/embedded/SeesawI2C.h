#ifndef SEESAW_I2C_H
#define SEESAW_I2C_H

#include <stdint.h>
#include <stdbool.h>
#include "SeesawProtocol.h"

// Simple I2C implementation for Seesaw protocol
// This is a minimal implementation focusing on NeoTrellis needs
class SeesawI2C {
public:
    enum Result {
        SUCCESS = 0,
        TIMEOUT = 1,
        NACK = 2,
        BUS_ERROR = 3
    };
    
    SeesawI2C(uint8_t address = Seesaw::NEOTRELLIS_ADDR);
    
    bool begin();
    
    // Core Seesaw protocol methods
    Result writeRegister(Seesaw::ModuleBase module, uint8_t reg, uint8_t value);
    Result writeRegister(Seesaw::ModuleBase module, uint8_t reg, const uint8_t* data, uint8_t len);
    Result readRegister(Seesaw::ModuleBase module, uint8_t reg, uint8_t* value);
    Result readRegister(Seesaw::ModuleBase module, uint8_t reg, uint8_t* data, uint8_t len);
    
    // NeoTrellis specific convenience methods
    Result setNeoPixelPin(uint8_t pin);
    Result setNeoPixelLength(uint8_t length);
    Result setNeoPixelBuffer(uint8_t offset, const uint8_t* data, uint8_t len);
    Result showNeoPixels();
    
    Result readKeypadCount(uint8_t* count);
    Result readKeypadFIFO(uint8_t* data, uint8_t maxLen);
    Result enableKeypadInterrupt();
    
private:
    uint8_t address_;
    bool initialized_;
    
    // Low-level I2C operations (stubs for now - will implement with SAMD51 SERCOM)
    Result i2cWrite(const uint8_t* data, uint8_t len);
    Result i2cRead(uint8_t* data, uint8_t len);
    Result i2cWriteRead(const uint8_t* writeData, uint8_t writeLen, 
                        uint8_t* readData, uint8_t readLen);
};

#endif