#ifndef MOCK_CLOCK_H
#define MOCK_CLOCK_H

#include "IClock.h"

/**
 * @file MockClock.h
 * @brief Mock implementation of IClock for deterministic testing
 */
class MockClock : public IClock {
public:
    explicit MockClock(uint32_t initialTime = 0) : currentTime_(initialTime) {}

    uint32_t getCurrentTime() const override { return currentTime_; }
    void delay(uint32_t milliseconds) override { currentTime_ += milliseconds; }
    void reset() override { currentTime_ = 0; }

    void setCurrentTime(uint32_t time) { currentTime_ = time; }
    void advanceTime(uint32_t milliseconds) { currentTime_ += milliseconds; }

private:
    uint32_t currentTime_;
};

#endif // MOCK_CLOCK_H