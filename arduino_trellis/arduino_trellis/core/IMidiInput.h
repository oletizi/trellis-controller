#ifndef IMIDI_INPUT_H
#define IMIDI_INPUT_H

#include <stdint.h>

struct MidiMessage {
    enum Type {
        NOTE_ON,
        NOTE_OFF,
        CONTROL_CHANGE,
        PROGRAM_CHANGE,
        CLOCK,
        START,
        STOP,
        CONTINUE
    };
    
    Type type;
    uint8_t channel;
    uint8_t data1;
    uint8_t data2;
    uint32_t timestamp;
};

class IMidiInput {
public:
    virtual ~IMidiInput() = default;
    
    typedef void (*NoteCallback)(uint8_t channel, uint8_t note, uint8_t velocity);
    typedef void (*ControlCallback)(uint8_t channel, uint8_t control, uint8_t value);
    typedef void (*ProgramCallback)(uint8_t channel, uint8_t program);
    typedef void (*ClockCallback)();
    typedef void (*TransportCallback)();
    
    virtual void setNoteOnCallback(NoteCallback callback) = 0;
    virtual void setNoteOffCallback(NoteCallback callback) = 0;
    virtual void setControlChangeCallback(ControlCallback callback) = 0;
    virtual void setProgramChangeCallback(ProgramCallback callback) = 0;
    
    virtual void setClockCallback(ClockCallback callback) = 0;
    virtual void setStartCallback(TransportCallback callback) = 0;
    virtual void setStopCallback(TransportCallback callback) = 0;
    virtual void setContinueCallback(TransportCallback callback) = 0;
    
    virtual void processMidiInput() = 0;
    virtual bool available() const = 0;
    virtual MidiMessage readMessage() = 0;
};

#endif