#ifndef IINPUT_H
#define IINPUT_H

#include <stdint.h>

struct ButtonEvent {
    uint8_t row;
    uint8_t col;
    bool pressed;  // true for press, false for release
    uint32_t timestamp;
};

class IInput {
public:
    virtual ~IInput() = default;
    
    virtual void init() = 0;
    virtual void shutdown() = 0;
    
    virtual bool pollEvents() = 0;
    virtual bool getNextEvent(ButtonEvent& event) = 0;
    virtual bool isButtonPressed(uint8_t row, uint8_t col) const = 0;
    
    virtual uint8_t getRows() const = 0;
    virtual uint8_t getCols() const = 0;
};

#endif