#ifndef IDISPLAY_H
#define IDISPLAY_H

#include <stdint.h>

class IDisplay {
public:
    virtual ~IDisplay() = default;
    
    virtual void init() = 0;
    virtual void shutdown() = 0;
    
    virtual void setLED(uint8_t row, uint8_t col, uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual void clear() = 0;
    virtual void refresh() = 0;
    
    virtual uint8_t getRows() const = 0;
    virtual uint8_t getCols() const = 0;
};

#endif