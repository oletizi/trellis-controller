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
    
    // Additional convenience methods for parameter locks
    virtual void setPixel(uint8_t index, uint32_t color) {
        // Default implementation converts index to row/col and color to RGB
        uint8_t row = index / getCols();
        uint8_t col = index % getCols();
        if (row < getRows()) {
            uint8_t r = (color >> 16) & 0xFF;
            uint8_t g = (color >> 8) & 0xFF;
            uint8_t b = color & 0xFF;
            setLED(row, col, r, g, b);
        }
    }
    
    virtual void show() {
        // Alias for refresh() for parameter lock compatibility
        refresh();
    }
};

#endif