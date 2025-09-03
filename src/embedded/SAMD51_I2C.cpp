#include "SAMD51_I2C.h"

#ifdef __SAMD51J19A__

#include "DebugSerial.h"

// External system timer function
extern volatile uint32_t g_tickCount;

// Static member initialization
uint32_t SAMD51_I2C::timeoutMs_ = TIMEOUT_MS;
bool SAMD51_I2C::initialized_ = false;

// SERCOM2 register base address for SAMD51
static constexpr uint32_t SERCOM2_BASE = 0x41012000UL;

SAMD51_I2C::SercomI2c* SAMD51_I2C::sercom() {
    return reinterpret_cast<SercomI2c*>(SERCOM2_BASE);
}

SAMD51_I2C::Result SAMD51_I2C::init(Speed speed) {
    if (initialized_) return SUCCESS;
    
    DEBUG_PRINTLN("Initializing SAMD51 I2C hardware...");
    
    // Enable peripheral clocks
    enableClock();
    DEBUG_PRINTLN("I2C: Clocks enabled");
    
    // Configure I2C pins (SDA/SCL)
    configurePins();
    DEBUG_PRINTLN("I2C: Pins configured");
    
    // Reset SERCOM
    reset();
    
    // Configure SERCOM for I2C Master mode
    configureSercom(speed);
    DEBUG_PRINTF("I2C: SERCOM configured for %d Hz\n", static_cast<int>(speed));
    
    // Wait for sync and enable
    if (waitForSync() != SUCCESS) {
        DEBUG_PRINTLN("I2C: Sync timeout during init");
        return TIMEOUT;
    }
    
    sercom()->CTRLA |= CTRLA_ENABLE;
    
    if (waitForSync() != SUCCESS) {
        DEBUG_PRINTLN("I2C: Enable timeout");
        return TIMEOUT;
    }
    
    // Force bus into idle state
    sercom()->STATUS = 0x1; // Force idle state
    
    initialized_ = true;
    DEBUG_PRINTLN("I2C: Initialization complete");
    return SUCCESS;
}

SAMD51_I2C::Result SAMD51_I2C::write(uint8_t address, const uint8_t* data, uint16_t length) {
    if (!initialized_ || !data || length == 0) return DATA_ERROR;
    
    DEBUG_PRINTF("I2C: Writing %d bytes to 0x%02X\n", length, address);
    
    // Start condition
    Result result = start();
    if (result != SUCCESS) {
        DEBUG_PRINTF("I2C: Start failed: %d\n", result);
        return result;
    }
    
    // Send address with write bit (0)
    sercom()->ADDR = (address << 1) | 0;
    
    // Wait for address ACK
    result = waitForFlag(INTFLAG_MB, true, timeoutMs_);
    if (result != SUCCESS) {
        DEBUG_PRINTF("I2C: Address NACK: %d\n", result);
        stop();
        return result;
    }
    
    // Check for NACK
    if (sercom()->STATUS & STATUS_RXNACK) {
        DEBUG_PRINTLN("I2C: Address NACK received");
        stop();
        return NACK;
    }
    
    // Send data bytes
    for (uint16_t i = 0; i < length; i++) {
        result = writeByte(data[i]);
        if (result != SUCCESS) {
            DEBUG_PRINTF("I2C: Data write failed at byte %d: %d\n", i, result);
            stop();
            return result;
        }
    }
    
    // Stop condition
    result = stop();
    DEBUG_PRINTF("I2C: Write complete: %d\n", result);
    return result;
}

SAMD51_I2C::Result SAMD51_I2C::read(uint8_t address, uint8_t* data, uint16_t length) {
    if (!initialized_ || !data || length == 0) return DATA_ERROR;
    
    DEBUG_PRINTF("I2C: Reading %d bytes from 0x%02X\n", length, address);
    
    // Start condition
    Result result = start();
    if (result != SUCCESS) {
        DEBUG_PRINTF("I2C: Start failed: %d\n", result);
        return result;
    }
    
    // Send address with read bit (1)
    sercom()->ADDR = (address << 1) | 1;
    
    // Wait for address ACK
    result = waitForFlag(INTFLAG_SB, true, timeoutMs_);
    if (result != SUCCESS) {
        DEBUG_PRINTF("I2C: Address NACK: %d\n", result);
        stop();
        return result;
    }
    
    // Check for NACK
    if (sercom()->STATUS & STATUS_RXNACK) {
        DEBUG_PRINTLN("I2C: Address NACK received");
        stop();
        return NACK;
    }
    
    // Read data bytes
    for (uint16_t i = 0; i < length; i++) {
        bool lastByte = (i == length - 1);
        result = readByte(&data[i], !lastByte); // NACK on last byte
        if (result != SUCCESS) {
            DEBUG_PRINTF("I2C: Data read failed at byte %d: %d\n", i, result);
            stop();
            return result;
        }
    }
    
    // Stop condition
    result = stop();
    DEBUG_PRINTF("I2C: Read complete: %d\n", result);
    return result;
}

SAMD51_I2C::Result SAMD51_I2C::writeRead(uint8_t address, const uint8_t* writeData, uint16_t writeLength,
                                         uint8_t* readData, uint16_t readLength) {
    if (!initialized_ || !writeData || !readData || writeLength == 0 || readLength == 0) {
        return DATA_ERROR;
    }
    
    DEBUG_PRINTF("I2C: Write-Read to 0x%02X: %d write, %d read\n", address, writeLength, readLength);
    
    // Write phase
    Result result = write(address, writeData, writeLength);
    if (result != SUCCESS) {
        return result;
    }
    
    // Small delay between write and read
    uint32_t start = g_tickCount;
    while ((g_tickCount - start) < 1) { /* 1ms delay */ }
    
    // Read phase  
    return read(address, readData, readLength);
}

SAMD51_I2C::Result SAMD51_I2C::start() {
    // Bus should be in idle state
    if (isBusy()) {
        DEBUG_PRINTLN("I2C: Bus busy on start");
        return BUS_ERROR;
    }
    
    // Start condition is generated automatically when writing to ADDR register
    return SUCCESS;
}

SAMD51_I2C::Result SAMD51_I2C::stop() {
    // Issue stop command
    sercom()->CTRLB |= CTRLB_CMD_STOP;
    
    // Wait for stop to complete
    uint32_t startTime = g_tickCount;
    while (isBusy()) {
        if (isTimeout(startTime, timeoutMs_)) {
            DEBUG_PRINTLN("I2C: Stop timeout");
            return TIMEOUT;
        }
    }
    
    return SUCCESS;
}

SAMD51_I2C::Result SAMD51_I2C::writeByte(uint8_t data) {
    // Write data
    sercom()->DATA = data;
    
    // Wait for transmit complete
    Result result = waitForFlag(INTFLAG_MB, true, timeoutMs_);
    if (result != SUCCESS) {
        return result;
    }
    
    // Check for NACK
    if (sercom()->STATUS & STATUS_RXNACK) {
        return NACK;
    }
    
    return SUCCESS;
}

SAMD51_I2C::Result SAMD51_I2C::readByte(uint8_t* data, bool ack) {
    // Wait for receive complete
    Result result = waitForFlag(INTFLAG_SB, true, timeoutMs_);
    if (result != SUCCESS) {
        return result;
    }
    
    // Read data
    *data = sercom()->DATA;
    
    // Send ACK/NACK
    if (!ack) {
        // Send NACK (stop receiving)
        sercom()->CTRLB |= CTRLB_ACKACT;
    } else {
        // Send ACK (continue receiving)
        sercom()->CTRLB &= ~CTRLB_ACKACT;
    }
    
    return SUCCESS;
}

bool SAMD51_I2C::isBusy() {
    uint32_t status = sercom()->STATUS;
    return (status & (STATUS_BUSERR | STATUS_ARBLOST)) == 0 && 
           !(sercom()->INTFLAG & (INTFLAG_MB | INTFLAG_SB));
}

void SAMD51_I2C::reset() {
    DEBUG_PRINTLN("I2C: Resetting SERCOM");
    
    // Software reset
    sercom()->CTRLA = CTRLA_SWRST;
    
    // Wait for reset to complete
    while (sercom()->CTRLA & CTRLA_SWRST) {
        // Wait for reset
    }
    
    while (sercom()->SYNCBUSY & SYNCBUSY_SWRST) {
        // Wait for sync
    }
}

void SAMD51_I2C::setTimeout(uint32_t timeoutMs) {
    timeoutMs_ = timeoutMs;
}

void SAMD51_I2C::enableClock() {
    // Enable SERCOM2 peripheral clock
    // This is a simplified implementation - real SAMD51 would need:
    // 1. Enable MCLK for SERCOM2
    // 2. Configure GCLK for SERCOM2
    // 3. Set up proper clock sources
    
    // For now, assume clocks are enabled by bootloader/startup
    DEBUG_PRINTLN("I2C: Clock configuration (placeholder)");
}

void SAMD51_I2C::configurePins() {
    // Configure SDA/SCL pins for SERCOM2
    // This is a simplified implementation - real SAMD51 would need:
    // 1. Set pin function to SERCOM (PMUX registers)
    // 2. Enable input/output (PINCFG registers) 
    // 3. Set appropriate pin numbers for NeoTrellis M4
    
    // For now, assume pins are configured by bootloader
    DEBUG_PRINTLN("I2C: Pin configuration (placeholder)");
}

void SAMD51_I2C::configureSercom(Speed speed) {
    // Calculate baud rate
    uint32_t baudValue = calculateBaud(speed);
    
    // Configure SERCOM for I2C Master mode
    uint32_t ctrla = CTRLA_MODE_I2C_MASTER |
                     CTRLA_SDAHOLD_300NS |
                     CTRLA_MEXTTOEN |
                     CTRLA_SEXTTOEN;
    
    if (speed >= FAST_400KHZ) {
        ctrla |= CTRLA_SPEED_FAST;
    } else {
        ctrla |= CTRLA_SPEED_STANDARD;
    }
    
    sercom()->CTRLA = ctrla;
    sercom()->CTRLB = CTRLB_SMEN | CTRLB_QCEN; // Smart mode + Quick command enable
    sercom()->BAUD = baudValue;
    
    DEBUG_PRINTF("I2C: CTRLA=0x%08X, BAUD=%d\n", ctrla, baudValue);
}

uint32_t SAMD51_I2C::calculateBaud(Speed speed) {
    // Simplified baud calculation for SAMD51
    // Real implementation would use: BAUD = (GCLK_FREQ / (2 * speed)) - 1
    // Assuming 48MHz GCLK
    const uint32_t GCLK_FREQ = 48000000;
    
    if (speed == 0) speed = STANDARD_100KHZ; // Prevent divide by zero
    
    uint32_t baudValue = (GCLK_FREQ / (2 * speed)) - 1;
    
    // Clamp to valid range (BAUD is 8-bit for standard speed, 16-bit for fast)
    if (speed >= FAST_400KHZ) {
        if (baudValue > 0xFFFF) baudValue = 0xFFFF;
    } else {
        if (baudValue > 0xFF) baudValue = 0xFF;
    }
    
    return baudValue;
}

SAMD51_I2C::Result SAMD51_I2C::waitForSync() {
    uint32_t startTime = g_tickCount;
    while (sercom()->SYNCBUSY & (SYNCBUSY_ENABLE | SYNCBUSY_SWRST | SYNCBUSY_SYSOP)) {
        if (isTimeout(startTime, timeoutMs_)) {
            return TIMEOUT;
        }
    }
    return SUCCESS;
}

SAMD51_I2C::Result SAMD51_I2C::waitForFlag(uint32_t flag, bool state, uint32_t timeoutMs) {
    uint32_t startTime = g_tickCount;
    
    while (true) {
        bool flagSet = (sercom()->INTFLAG & flag) != 0;
        if (flagSet == state) {
            // Check for bus errors
            if (sercom()->STATUS & (STATUS_BUSERR | STATUS_ARBLOST)) {
                return BUS_ERROR;
            }
            return SUCCESS;
        }
        
        if (isTimeout(startTime, timeoutMs)) {
            return TIMEOUT;
        }
    }
}

bool SAMD51_I2C::isTimeout(uint32_t startTime, uint32_t timeoutMs) {
    return (g_tickCount - startTime) >= timeoutMs;
}


#endif // __SAMD51J19A__