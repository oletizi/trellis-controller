#ifndef ICLOCK_H
#define ICLOCK_H

#include <stdint.h>

// Clock abstraction interface
class IClock {
public:
    virtual ~IClock() = default;
    
    virtual uint32_t getCurrentTime() const = 0;
    virtual void delay(uint32_t milliseconds) = 0;
    virtual void reset() = 0;
};

#endif