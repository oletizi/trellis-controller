#ifndef ISHIFTCONTROLS_H
#define ISHIFTCONTROLS_H

#include "IInput.h"

class IShiftControls {
public:
    virtual ~IShiftControls() = default;
    
    virtual void handleShiftInput(const ButtonEvent& event) = 0;
    virtual bool isShiftActive() const = 0;
    virtual bool shouldHandleAsControl(uint8_t row, uint8_t col) const = 0;
};

#endif