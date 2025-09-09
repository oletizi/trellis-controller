#ifndef MOCK_CLOCK_H
#define MOCK_CLOCK_H

#include "IClock.h"

class MockClock : public IClock {
public:
    MockClock() : currentTime_(0) {}
    
    uint32_t getCurrentTime() const override {
        return currentTime_;
    }
    
    void delay(uint32_t milliseconds) override {
        currentTime_ += milliseconds;
    }
    
    void reset() override {
        currentTime_ = 0;
    }
    
    // Test methods
    void setCurrentTime(uint32_t time) {
        currentTime_ = time;
    }
    
    void advanceTime(uint32_t milliseconds) {
        currentTime_ += milliseconds;
    }
    
    // Alias for compatibility with some tests
    void advance(uint32_t milliseconds) {
        advanceTime(milliseconds);
    }
    
private:
    uint32_t currentTime_;
};

#endif