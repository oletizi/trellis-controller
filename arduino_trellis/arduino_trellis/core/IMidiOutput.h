#ifndef IMIDI_OUTPUT_H
#define IMIDI_OUTPUT_H

#include <stdint.h>

class IMidiOutput {
public:
    virtual ~IMidiOutput() = default;
    
    virtual void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) = 0;
    virtual void sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) = 0;
    virtual void sendControlChange(uint8_t channel, uint8_t control, uint8_t value) = 0;
    virtual void sendProgramChange(uint8_t channel, uint8_t program) = 0;
    
    virtual void sendClock() = 0;
    virtual void sendStart() = 0;
    virtual void sendStop() = 0;
    virtual void sendContinue() = 0;
    
    virtual bool isConnected() const = 0;
    virtual void flush() = 0;
};

#endif