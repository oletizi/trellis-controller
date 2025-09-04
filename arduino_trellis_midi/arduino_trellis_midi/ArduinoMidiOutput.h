#ifndef ARDUINO_MIDI_OUTPUT_H
#define ARDUINO_MIDI_OUTPUT_H

#include "IMidiOutput.h"
#include <MIDIUSB.h>

class ArduinoMidiOutput : public IMidiOutput {
public:
    ArduinoMidiOutput() : connected_(true) {}
    
    void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) override {
        if (!connected_) return;
        
        midiEventPacket_t noteOn = {0x09, static_cast<uint8_t>(0x90 | channel), note, velocity};
        MidiUSB.sendMIDI(noteOn);
        MidiUSB.flush();
    }
    
    void sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) override {
        if (!connected_) return;
        
        midiEventPacket_t noteOff = {0x08, static_cast<uint8_t>(0x80 | channel), note, velocity};
        MidiUSB.sendMIDI(noteOff);
        MidiUSB.flush();
    }
    
    void sendControlChange(uint8_t channel, uint8_t control, uint8_t value) override {
        if (!connected_) return;
        
        midiEventPacket_t controlChange = {0x0B, static_cast<uint8_t>(0xB0 | channel), control, value};
        MidiUSB.sendMIDI(controlChange);
        MidiUSB.flush();
    }
    
    void sendProgramChange(uint8_t channel, uint8_t program) override {
        if (!connected_) return;
        
        midiEventPacket_t programChange = {0x0C, static_cast<uint8_t>(0xC0 | channel), program, 0};
        MidiUSB.sendMIDI(programChange);
        MidiUSB.flush();
    }
    
    void sendClock() override {
        if (!connected_) return;
        
        midiEventPacket_t clockMsg = {0x0F, 0xF8, 0, 0};
        MidiUSB.sendMIDI(clockMsg);
        MidiUSB.flush();
    }
    
    void sendStart() override {
        if (!connected_) return;
        
        midiEventPacket_t startMsg = {0x0F, 0xFA, 0, 0};
        MidiUSB.sendMIDI(startMsg);
        MidiUSB.flush();
    }
    
    void sendStop() override {
        if (!connected_) return;
        
        midiEventPacket_t stopMsg = {0x0F, 0xFC, 0, 0};
        MidiUSB.sendMIDI(stopMsg);
        MidiUSB.flush();
    }
    
    void sendContinue() override {
        if (!connected_) return;
        
        midiEventPacket_t continueMsg = {0x0F, 0xFB, 0, 0};
        MidiUSB.sendMIDI(continueMsg);
        MidiUSB.flush();
    }
    
    bool isConnected() const override {
        return connected_;
    }
    
    void flush() override {
        MidiUSB.flush();
    }

private:
    bool connected_;
};

#endif