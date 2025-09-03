#ifndef SAMD51_I2C_H
#define SAMD51_I2C_H

#include <stdint.h>

#ifdef __SAMD51J19A__

// SAMD51 I2C Hardware Driver
// Implements I2C Master mode using SERCOM peripheral
class SAMD51_I2C {
public:
    enum Result {
        SUCCESS = 0,
        TIMEOUT,
        NACK,
        BUS_ERROR,
        DATA_ERROR
    };
    
    enum Speed {
        STANDARD_100KHZ = 100000,
        FAST_400KHZ = 400000
    };
    
    // Initialize I2C peripheral
    static Result init(Speed speed = FAST_400KHZ);
    
    // I2C Master operations
    static Result write(uint8_t address, const uint8_t* data, uint16_t length);
    static Result read(uint8_t address, uint8_t* data, uint16_t length);
    static Result writeRead(uint8_t address, const uint8_t* writeData, uint16_t writeLength,
                           uint8_t* readData, uint16_t readLength);
    
    // Low-level operations
    static Result start();
    static Result stop();
    static Result writeByte(uint8_t data);
    static Result readByte(uint8_t* data, bool ack);
    
    // Status and control
    static bool isBusy();
    static void reset();
    static void setTimeout(uint32_t timeoutMs);
    
private:
    static constexpr uint8_t SERCOM_NUM = 2; // Using SERCOM2 for I2C
    static constexpr uint32_t TIMEOUT_MS = 1000;
    
    static uint32_t timeoutMs_;
    static bool initialized_;
    
    // SERCOM register access
    static void enableClock();
    static void configurePins();
    static void configureSercom(Speed speed);
    static uint32_t calculateBaud(Speed speed);
    
    // Wait operations with timeout
    static Result waitForSync();
    static Result waitForFlag(uint32_t flag, bool state, uint32_t timeoutMs);
    static bool isTimeout(uint32_t startTime, uint32_t timeoutMs);
    
    // Hardware register definitions
    struct SercomI2c {
        volatile uint32_t CTRLA;
        volatile uint32_t CTRLB;
        volatile uint32_t CTRLC;
        volatile uint32_t BAUD;
        volatile uint32_t INTENCLR;
        volatile uint32_t INTENSET;
        volatile uint32_t INTFLAG;
        volatile uint32_t STATUS;
        volatile uint32_t SYNCBUSY;
        volatile uint32_t LENGTH;
        volatile uint32_t ADDR;
        volatile uint32_t DATA;
    };
    
    static SercomI2c* sercom();
    
    // Register bit definitions
    static constexpr uint32_t CTRLA_SWRST = (1 << 0);
    static constexpr uint32_t CTRLA_ENABLE = (1 << 1);
    static constexpr uint32_t CTRLA_MODE_I2C_MASTER = (5 << 2);
    static constexpr uint32_t CTRLA_SDAHOLD_300NS = (1 << 20);
    static constexpr uint32_t CTRLA_MEXTTOEN = (1 << 9);
    static constexpr uint32_t CTRLA_SEXTTOEN = (1 << 10);
    static constexpr uint32_t CTRLA_SPEED_STANDARD = (0 << 24);
    static constexpr uint32_t CTRLA_SPEED_FAST = (1 << 24);
    
    static constexpr uint32_t CTRLB_SMEN = (1 << 8);
    static constexpr uint32_t CTRLB_QCEN = (1 << 9);
    static constexpr uint32_t CTRLB_CMD_STOP = (3 << 16);
    static constexpr uint32_t CTRLB_ACKACT = (1 << 18);
    
    static constexpr uint32_t INTFLAG_MB = (1 << 0);
    static constexpr uint32_t INTFLAG_SB = (1 << 1);
    static constexpr uint32_t INTFLAG_ERROR = (1 << 7);
    
    static constexpr uint32_t STATUS_RXNACK = (1 << 2);
    static constexpr uint32_t STATUS_ARBLOST = (1 << 3);
    static constexpr uint32_t STATUS_BUSERR = (1 << 0);
    
    static constexpr uint32_t SYNCBUSY_SWRST = (1 << 0);
    static constexpr uint32_t SYNCBUSY_ENABLE = (1 << 1);
    static constexpr uint32_t SYNCBUSY_SYSOP = (1 << 2);
};

#endif // __SAMD51J19A__

#endif // SAMD51_I2C_H