#ifndef MOCK_MIDI_OUTPUT_H
#define MOCK_MIDI_OUTPUT_H

#include "IMidiOutput.h"
#include <vector>

struct MidiNoteMessage {
    uint8_t channel;
    uint8_t note;
    uint8_t velocity;
    bool isNoteOn;
};

struct MidiControlMessage {
    uint8_t channel;
    uint8_t control;
    uint8_t value;
};

struct MidiTransportMessage {
    enum Type { CLOCK, START, STOP, CONTINUE };
    Type type;
};

class MockMidiOutput : public IMidiOutput {
public:
    MockMidiOutput() : connected_(true) {}
    
    void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) override {
        noteMessages_.push_back({channel, note, velocity, true});
    }
    
    void sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) override {
        noteMessages_.push_back({channel, note, velocity, false});
    }
    
    void sendControlChange(uint8_t channel, uint8_t control, uint8_t value) override {
        controlMessages_.push_back({channel, control, value});
    }
    
    void sendProgramChange(uint8_t channel, uint8_t program) override {
        programChanges_.push_back({channel, program, 0});
    }
    
    void sendClock() override {
        transportMessages_.push_back({MidiTransportMessage::CLOCK});
    }
    
    void sendStart() override {
        transportMessages_.push_back({MidiTransportMessage::START});
    }
    
    void sendStop() override {
        transportMessages_.push_back({MidiTransportMessage::STOP});
    }
    
    void sendContinue() override {
        transportMessages_.push_back({MidiTransportMessage::CONTINUE});
    }
    
    bool isConnected() const override { return connected_; }
    void flush() override {}
    
    // Test helpers
    const std::vector<MidiNoteMessage>& getNoteMessages() const { return noteMessages_; }
    const std::vector<MidiControlMessage>& getControlMessages() const { return controlMessages_; }
    const std::vector<MidiTransportMessage>& getTransportMessages() const { return transportMessages_; }
    
    void setConnected(bool connected) { connected_ = connected; }
    void clearMessages() {
        noteMessages_.clear();
        controlMessages_.clear();
        transportMessages_.clear();
        programChanges_.clear();
    }
    
    size_t getNoteMessageCount() const { return noteMessages_.size(); }
    size_t getControlMessageCount() const { return controlMessages_.size(); }
    size_t getTransportMessageCount() const { return transportMessages_.size(); }
    
private:
    bool connected_;
    std::vector<MidiNoteMessage> noteMessages_;
    std::vector<MidiControlMessage> controlMessages_;
    std::vector<MidiControlMessage> programChanges_;
    std::vector<MidiTransportMessage> transportMessages_;
};

#endif