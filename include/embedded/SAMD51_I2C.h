#ifndef SAMD51_I2C_H
#define SAMD51_I2C_H

#include <stdint.h>
#include <stdbool.h>

// I2C Master driver for SAMD51
class SAMD51_I2C {
public:
    enum class Speed {
        STANDARD_100KHZ = 100000,
        FAST_400KHZ = 400000
    };
    
    enum class Result {
        SUCCESS = 0,
        TIMEOUT,
        NACK_ADDR,
        NACK_DATA,
        BUS_ERROR,
        ARBITRATION_LOST
    };
    
    SAMD51_I2C();
    
    // Initialize I2C master on SERCOM pins
    bool init(Speed speed = Speed::STANDARD_100KHZ);
    void deinit();
    
    // Synchronous I2C operations
    Result write(uint8_t addr, const uint8_t* data, uint8_t len);
    Result read(uint8_t addr, uint8_t* data, uint8_t len);
    Result writeRead(uint8_t addr, const uint8_t* writeData, uint8_t writeLen, 
                     uint8_t* readData, uint8_t readLen);
    
    // Register access helpers
    Result writeRegister(uint8_t addr, uint8_t reg, uint8_t value);
    Result readRegister(uint8_t addr, uint8_t reg, uint8_t* value);
    Result readRegisters(uint8_t addr, uint8_t reg, uint8_t* data, uint8_t len);
    
private:
    bool initialized_;
    uint32_t clockFreq_;
    
    // Hardware abstraction (will be implemented with actual SERCOM registers)
    void enableClock();
    void disableClock();
    void configurePins();
    void configureBaudRate(Speed speed);
    void reset();
    
    Result waitForIdle(uint32_t timeoutMs = 100);
    Result sendStart();
    Result sendStop();
    Result sendAddress(uint8_t addr, bool read);
    Result sendByte(uint8_t data);
    Result receiveByte(uint8_t* data, bool ack);
};

#endif