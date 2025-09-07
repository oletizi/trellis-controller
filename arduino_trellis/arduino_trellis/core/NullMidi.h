#ifndef NULL_MIDI_H
#define NULL_MIDI_H

#include "IMidiOutput.h"
#include "IMidiInput.h"

class NullMidiOutput : public IMidiOutput {
public:
    void sendNoteOn(uint8_t, uint8_t, uint8_t) override {}
    void sendNoteOff(uint8_t, uint8_t, uint8_t) override {}
    void sendControlChange(uint8_t, uint8_t, uint8_t) override {}
    void sendProgramChange(uint8_t, uint8_t) override {}
    
    void sendClock() override {}
    void sendStart() override {}
    void sendStop() override {}
    void sendContinue() override {}
    
    bool isConnected() const override { return false; }
    void flush() override {}
};

class NullMidiInput : public IMidiInput {
public:
    void setNoteOnCallback(NoteCallback) override {}
    void setNoteOffCallback(NoteCallback) override {}
    void setControlChangeCallback(ControlCallback) override {}
    void setProgramChangeCallback(ProgramCallback) override {}
    
    void setClockCallback(ClockCallback) override {}
    void setStartCallback(TransportCallback) override {}
    void setStopCallback(TransportCallback) override {}
    void setContinueCallback(TransportCallback) override {}
    
    void processMidiInput() override {}
    bool available() const override { return false; }
    MidiMessage readMessage() override { 
        return {MidiMessage::NOTE_ON, 0, 0, 0, 0}; 
    }
};

#endif