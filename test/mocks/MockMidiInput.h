#ifndef MOCK_MIDI_INPUT_H
#define MOCK_MIDI_INPUT_H

#include "IMidiInput.h"
#include <vector>

class MockMidiInput : public IMidiInput {
public:
    MockMidiInput() 
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
        // Process any queued messages
        while (!messageQueue_.empty()) {
            MidiMessage msg = messageQueue_.front();
            messageQueue_.erase(messageQueue_.begin());
            
            switch (msg.type) {
                case MidiMessage::NOTE_ON:
                    if (noteOnCallback_) {
                        noteOnCallback_(msg.channel, msg.data1, msg.data2);
                    }
                    break;
                case MidiMessage::NOTE_OFF:
                    if (noteOffCallback_) {
                        noteOffCallback_(msg.channel, msg.data1, msg.data2);
                    }
                    break;
                case MidiMessage::CONTROL_CHANGE:
                    if (controlChangeCallback_) {
                        controlChangeCallback_(msg.channel, msg.data1, msg.data2);
                    }
                    break;
                case MidiMessage::PROGRAM_CHANGE:
                    if (programChangeCallback_) {
                        programChangeCallback_(msg.channel, msg.data1);
                    }
                    break;
                case MidiMessage::CLOCK:
                    if (clockCallback_) {
                        clockCallback_();
                    }
                    break;
                case MidiMessage::START:
                    if (startCallback_) {
                        startCallback_();
                    }
                    break;
                case MidiMessage::STOP:
                    if (stopCallback_) {
                        stopCallback_();
                    }
                    break;
                case MidiMessage::CONTINUE:
                    if (continueCallback_) {
                        continueCallback_();
                    }
                    break;
            }
        }
    }
    
    bool available() const override {
        return !messageQueue_.empty();
    }
    
    MidiMessage readMessage() override {
        if (!messageQueue_.empty()) {
            MidiMessage msg = messageQueue_.front();
            messageQueue_.erase(messageQueue_.begin());
            return msg;
        }
        return {MidiMessage::NOTE_ON, 0, 0, 0, 0};
    }
    
    // Test helpers
    void simulateNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
        messageQueue_.push_back({MidiMessage::NOTE_ON, channel, note, velocity, 0});
    }
    
    void simulateNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
        messageQueue_.push_back({MidiMessage::NOTE_OFF, channel, note, velocity, 0});
    }
    
    void simulateControlChange(uint8_t channel, uint8_t control, uint8_t value) {
        messageQueue_.push_back({MidiMessage::CONTROL_CHANGE, channel, control, value, 0});
    }
    
    void simulateClock() {
        messageQueue_.push_back({MidiMessage::CLOCK, 0, 0, 0, 0});
    }
    
    void simulateStart() {
        messageQueue_.push_back({MidiMessage::START, 0, 0, 0, 0});
    }
    
    void simulateStop() {
        messageQueue_.push_back({MidiMessage::STOP, 0, 0, 0, 0});
    }
    
private:
    NoteCallback noteOnCallback_;
    NoteCallback noteOffCallback_;
    ControlCallback controlChangeCallback_;
    ProgramCallback programChangeCallback_;
    ClockCallback clockCallback_;
    TransportCallback startCallback_;
    TransportCallback stopCallback_;
    TransportCallback continueCallback_;
    
    std::vector<MidiMessage> messageQueue_;
};

#endif