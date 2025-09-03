#ifndef IHARDWARE_H
#define IHARDWARE_H

#include <stdint.h>

// Hardware abstraction interface for platform-agnostic code
class IHardware {
public:
    virtual ~IHardware() = default;
    
    // Button interface
    virtual bool readButton(uint8_t index) = 0;
    virtual void setButtonCallback(uint8_t index, void (*callback)(uint8_t, bool)) = 0;
    
    // LED interface
    virtual void setLED(uint8_t index, uint32_t color) = 0;
    virtual void updateLEDs() = 0;
    virtual void setLEDBrightness(uint8_t brightness) = 0;
    virtual void clearLEDs() = 0;
    
    // System interface
    virtual uint32_t getSystemTime() = 0;
    virtual void delay(uint32_t milliseconds) = 0;
    virtual bool initialize() = 0;
    virtual void poll() = 0;
};

#endif