#ifndef ARDUINO_MIDI_INPUT_H
#define ARDUINO_MIDI_INPUT_H

#include "IMidiInput.h"
#include <MIDIUSB.h>

class ArduinoMidiInput : public IMidiInput {
public:
    ArduinoMidiInput() 
        : noteOnCallback_(nullptr)
        , noteOffCallback_(nullptr)
        , controlChangeCallback_(nullptr)
        , programChangeCallback_(nullptr)
        , clockCallback_(nullptr)
        , startCallback_(nullptr)
        , stopCallback_(nullptr)
        , continueCallback_(nullptr) {}
    
    void setNoteOnCallback(NoteCallback callback) override {
        noteOnCallback_ = callback;
    }
    
    void setNoteOffCallback(NoteCallback callback) override {
        noteOffCallback_ = callback;
    }
    
    void setControlChangeCallback(ControlCallback callback) override {
        controlChangeCallback_ = callback;
    }
    
    void setProgramChangeCallback(ProgramCallback callback) override {
        programChangeCallback_ = callback;
    }
    
    void setClockCallback(ClockCallback callback) override {
        clockCallback_ = callback;
    }
    
    void setStartCallback(TransportCallback callback) override {
        startCallback_ = callback;
    }
    
    void setStopCallback(TransportCallback callback) override {
        stopCallback_ = callback;
    }
    
    void setContinueCallback(TransportCallback callback) override {
        continueCallback_ = callback;
    }
    
    void processMidiInput() override {
        midiEventPacket_t rx;
        do {
            rx = MidiUSB.read();
            if (rx.header != 0) {
                processMidiPacket(rx);
            }
        } while (rx.header != 0);
    }
    
    bool available() const override {
        // MidiUSB doesn't provide a direct available() method
        return true; // Always process in processMidiInput()
    }
    
    MidiMessage readMessage() override {
        // Not implemented for Arduino version - use callbacks instead
        return {MidiMessage::NOTE_ON, 0, 0, 0, 0};
    }

private:
    void processMidiPacket(const midiEventPacket_t& packet) {
        uint8_t command = packet.byte1 & 0xF0;
        uint8_t channel = packet.byte1 & 0x0F;
        
        switch (command) {
            case 0x90: // Note On
                if (packet.byte3 > 0) {
                    if (noteOnCallback_) {
                        noteOnCallback_(channel, packet.byte2, packet.byte3);
                    }
                } else {
                    // Note on with velocity 0 is note off
                    if (noteOffCallback_) {
                        noteOffCallback_(channel, packet.byte2, packet.byte3);
                    }
                }
                break;
                
            case 0x80: // Note Off
                if (noteOffCallback_) {
                    noteOffCallback_(channel, packet.byte2, packet.byte3);
                }
                break;
                
            case 0xB0: // Control Change
                if (controlChangeCallback_) {
                    controlChangeCallback_(channel, packet.byte2, packet.byte3);
                }
                break;
                
            case 0xC0: // Program Change
                if (programChangeCallback_) {
                    programChangeCallback_(channel, packet.byte2);
                }
                break;
                
            case 0xF0: // System messages
                switch (packet.byte1) {
                    case 0xF8: // Clock
                        if (clockCallback_) {
                            clockCallback_();
                        }
                        break;
                        
                    case 0xFA: // Start
                        if (startCallback_) {
                            startCallback_();
                        }
                        break;
                        
                    case 0xFC: // Stop
                        if (stopCallback_) {
                            stopCallback_();
                        }
                        break;
                        
                    case 0xFB: // Continue
                        if (continueCallback_) {
                            continueCallback_();
                        }
                        break;
                }
                break;
        }
    }
    
    NoteCallback noteOnCallback_;
    NoteCallback noteOffCallback_;
    ControlCallback controlChangeCallback_;
    ProgramCallback programChangeCallback_;
    ClockCallback clockCallback_;
    TransportCallback startCallback_;
    TransportCallback stopCallback_;
    TransportCallback continueCallback_;
};

#endif