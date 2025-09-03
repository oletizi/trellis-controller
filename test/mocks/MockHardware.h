#ifndef MOCK_HARDWARE_H
#define MOCK_HARDWARE_H

#include "IHardware.h"
#include <vector>

class MockHardware : public IHardware {
public:
    static constexpr uint8_t NUM_BUTTONS = 32;
    static constexpr uint8_t NUM_LEDS = 32;
    
    MockHardware() : systemTime_(0), brightness_(255) {
        buttonStates_.resize(NUM_BUTTONS, false);
        ledColors_.resize(NUM_LEDS, 0);
        buttonCallbacks_.resize(NUM_BUTTONS, nullptr);
        
        // Initialize call tracking
        clearCallHistory();
    }
    
    // IHardware implementation
    bool readButton(uint8_t index) override {
        if (index < NUM_BUTTONS) {
            readButtonCalls_++;
            lastButtonRead_ = index;
            return buttonStates_[index];
        }
        return false;
    }
    
    void setButtonCallback(uint8_t index, void (*callback)(uint8_t, bool)) override {
        if (index < NUM_BUTTONS) {
            buttonCallbacks_[index] = callback;
            callbackSetCalls_++;
        }
    }
    
    void setLED(uint8_t index, uint32_t color) override {
        if (index < NUM_LEDS) {
            ledColors_[index] = color;
            setLedCalls_++;
            lastLedIndex_ = index;
            lastLedColor_ = color;
        }
    }
    
    void updateLEDs() override {
        updateLedsCalls_++;
    }
    
    void setLEDBrightness(uint8_t brightness) override {
        brightness_ = brightness;
        setBrightnessCalls_++;
    }
    
    void clearLEDs() override {
        for (auto& color : ledColors_) {
            color = 0;
        }
        clearLedsCalls_++;
    }
    
    uint32_t getSystemTime() override {
        return systemTime_;
    }
    
    void delay(uint32_t milliseconds) override {
        systemTime_ += milliseconds;
        delayCalls_++;
        lastDelayMs_ = milliseconds;
    }
    
    bool initialize() override {
        initializeCalls_++;
        return initializeReturnValue_;
    }
    
    void poll() override {
        pollCalls_++;
        
        // Simulate button state changes
        for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
            if (buttonCallbacks_[i]) {
                bool currentState = buttonStates_[i];
                if (currentState != lastButtonStates_[i]) {
                    buttonCallbacks_[i](i, currentState);
                }
                lastButtonStates_[i] = currentState;
            }
        }
    }
    
    // Test interface methods
    void simulateButtonPress(uint8_t index) {
        if (index < NUM_BUTTONS) {
            buttonStates_[index] = true;
        }
    }
    
    void simulateButtonRelease(uint8_t index) {
        if (index < NUM_BUTTONS) {
            buttonStates_[index] = false;
        }
    }
    
    void setSystemTime(uint32_t time) {
        systemTime_ = time;
    }
    
    void advanceTime(uint32_t milliseconds) {
        systemTime_ += milliseconds;
    }
    
    void setInitializeReturnValue(bool value) {
        initializeReturnValue_ = value;
    }
    
    // Test verification methods
    uint32_t getLEDColor(uint8_t index) const {
        return (index < NUM_LEDS) ? ledColors_[index] : 0;
    }
    
    uint8_t getBrightness() const {
        return brightness_;
    }
    
    // Call tracking
    void clearCallHistory() {
        readButtonCalls_ = 0;
        setLedCalls_ = 0;
        updateLedsCalls_ = 0;
        clearLedsCalls_ = 0;
        setBrightnessCalls_ = 0;
        delayCalls_ = 0;
        initializeCalls_ = 0;
        pollCalls_ = 0;
        callbackSetCalls_ = 0;
        
        lastButtonRead_ = 255;
        lastLedIndex_ = 255;
        lastLedColor_ = 0;
        lastDelayMs_ = 0;
        
        lastButtonStates_.resize(NUM_BUTTONS, false);
    }
    
    uint32_t getReadButtonCalls() const { return readButtonCalls_; }
    uint32_t getSetLedCalls() const { return setLedCalls_; }
    uint32_t getUpdateLedsCalls() const { return updateLedsCalls_; }
    uint32_t getClearLedsCalls() const { return clearLedsCalls_; }
    uint32_t getSetBrightnessCalls() const { return setBrightnessCalls_; }
    uint32_t getDelayCalls() const { return delayCalls_; }
    uint32_t getInitializeCalls() const { return initializeCalls_; }
    uint32_t getPollCalls() const { return pollCalls_; }
    uint32_t getCallbackSetCalls() const { return callbackSetCalls_; }
    
    uint8_t getLastButtonRead() const { return lastButtonRead_; }
    uint8_t getLastLedIndex() const { return lastLedIndex_; }
    uint32_t getLastLedColor() const { return lastLedColor_; }
    uint32_t getLastDelayMs() const { return lastDelayMs_; }
    
private:
    std::vector<bool> buttonStates_;
    std::vector<uint32_t> ledColors_;
    std::vector<void (*)(uint8_t, bool)> buttonCallbacks_;
    std::vector<bool> lastButtonStates_;
    
    uint32_t systemTime_;
    uint8_t brightness_;
    bool initializeReturnValue_ = true;
    
    // Call tracking
    uint32_t readButtonCalls_;
    uint32_t setLedCalls_;
    uint32_t updateLedsCalls_;
    uint32_t clearLedsCalls_;
    uint32_t setBrightnessCalls_;
    uint32_t delayCalls_;
    uint32_t initializeCalls_;
    uint32_t pollCalls_;
    uint32_t callbackSetCalls_;
    
    uint8_t lastButtonRead_;
    uint8_t lastLedIndex_;
    uint32_t lastLedColor_;
    uint32_t lastDelayMs_;
};

#endif