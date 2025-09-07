#include "AdaptiveButtonTracker.h"
#include <algorithm>  // For std::min, std::max

AdaptiveButtonTracker::AdaptiveButtonTracker()
    : profile_()
    , stats_()
    , lastUpdateTime_(0)
    , holdDurationIndex_(0)
    , holdSampleCount_(0) {
    
    // Initialize button states
    for (uint8_t i = 0; i < MAX_BUTTONS; ++i) {
        states_[i] = ButtonState();
    }
    
    // Initialize hold duration buffer
    for (uint8_t i = 0; i < 16; ++i) {
        holdDurations_[i] = 0;
    }
}

void AdaptiveButtonTracker::update(uint32_t buttonMask, uint32_t currentTime) {
    lastUpdateTime_ = currentTime;
    
    // Update each button state
    for (uint8_t i = 0; i < MAX_BUTTONS; ++i) {
        bool pressed = (buttonMask & (1UL << i)) != 0;
        updateButtonState(i, pressed, currentTime);
    }
    
    // Periodically update threshold based on learning
    if (profile_.learningEnabled && stats_.samples > 10) {
        static uint32_t lastThresholdUpdate = 0;
        if (currentTime - lastThresholdUpdate > 5000) {  // Update every 5 seconds
            updateThreshold();
            lastThresholdUpdate = currentTime;
        }
    }
}

bool AdaptiveButtonTracker::wasPressed(uint8_t button) {
    if (!isValidButton(button)) return false;
    
    bool result = states_[button].wasPressed;
    states_[button].wasPressed = false;  // Clear event flag
    return result;
}

bool AdaptiveButtonTracker::wasReleased(uint8_t button) {
    if (!isValidButton(button)) return false;
    
    bool result = states_[button].wasReleased;
    states_[button].wasReleased = false;  // Clear event flag
    return result;
}

bool AdaptiveButtonTracker::isHeld(uint8_t button) {
    if (!isValidButton(button)) return false;
    return states_[button].isHeld;
}

bool AdaptiveButtonTracker::isPressed(uint8_t button) {
    if (!isValidButton(button)) return false;
    return states_[button].pressed;
}

uint8_t AdaptiveButtonTracker::getHeldButton() {
    for (uint8_t i = 0; i < MAX_BUTTONS; ++i) {
        if (states_[i].isHeld) {
            return i;
        }
    }
    return 0xFF;  // No button held
}

uint32_t AdaptiveButtonTracker::getHoldDuration(uint8_t button, uint32_t currentTime) {
    if (!isValidButton(button) || !states_[button].pressed) {
        return 0;
    }
    
    return currentTime - states_[button].pressTime;
}

void AdaptiveButtonTracker::recordSuccessfulActivation(uint8_t button, uint32_t holdDuration) {
    if (!isValidButton(button)) return;
    
    ++stats_.totalActivations;
    addHoldSample(holdDuration);
    updateStats();
}

void AdaptiveButtonTracker::recordFalseActivation(uint8_t button, uint32_t holdDuration) {
    if (!isValidButton(button)) return;
    
    ++stats_.falseActivations;
    updateStats();
}

void AdaptiveButtonTracker::recordMissedActivation(uint8_t button, uint32_t holdDuration) {
    if (!isValidButton(button)) return;
    
    ++stats_.missedActivations;
    addHoldSample(holdDuration);
    updateStats();
}

void AdaptiveButtonTracker::updateThreshold() {
    if (!profile_.learningEnabled || stats_.samples < 5) {
        return;
    }
    
    uint32_t newThreshold = calculateOptimalThreshold();
    
    // Apply learning rate
    float oldThreshold = static_cast<float>(profile_.threshold);
    float targetThreshold = static_cast<float>(newThreshold);
    float adjusted = oldThreshold + profile_.adaptationRate * (targetThreshold - oldThreshold);
    
    // Clamp to valid range
    profile_.threshold = static_cast<uint32_t>(std::max(static_cast<float>(profile_.minThreshold),
                                              std::min(static_cast<float>(profile_.maxThreshold), adjusted)));
}

void AdaptiveButtonTracker::setProfile(const HoldProfile& profile) {
    if (profile.isValid()) {
        profile_ = profile;
    }
}

void AdaptiveButtonTracker::resetLearning() {
    stats_ = LearningStats();
    holdDurationIndex_ = 0;
    holdSampleCount_ = 0;
    
    for (uint8_t i = 0; i < 16; ++i) {
        holdDurations_[i] = 0;
    }
}

const AdaptiveButtonTracker::ButtonState& AdaptiveButtonTracker::getButtonState(uint8_t button) const {
    static ButtonState invalidState;
    
    if (!isValidButton(button)) {
        return invalidState;
    }
    
    return states_[button];
}

void AdaptiveButtonTracker::forceButtonState(uint8_t button, bool pressed, uint32_t currentTime) {
    if (!isValidButton(button)) return;
    
    updateButtonState(button, pressed, currentTime);
}

void AdaptiveButtonTracker::markHoldProcessed(uint8_t button) {
    if (!isValidButton(button)) return;
    
    states_[button].holdProcessed = true;
}

void AdaptiveButtonTracker::updateButtonState(uint8_t button, bool pressed, uint32_t currentTime) {
    ButtonState& state = states_[button];
    
    // Clear event flags
    state.wasPressed = false;
    state.wasReleased = false;
    
    // Handle press/release transitions
    if (pressed && !state.pressed) {
        // Button just pressed
        state.pressed = true;
        state.wasPressed = true;
        state.pressTime = currentTime;
        state.isHeld = false;
        state.holdProcessed = false;
        state.holdDuration = 0;
        
    } else if (!pressed && state.pressed) {
        // Button just released
        state.pressed = false;
        state.wasReleased = true;
        state.releaseTime = currentTime;
        state.holdDuration = currentTime - state.pressTime;
        
        // Reset hold state
        state.isHeld = false;
        state.holdProcessed = false;
        
    } else if (pressed && state.pressed) {
        // Button continues to be pressed - check for hold
        if (shouldBeHeld(button, currentTime) && !state.isHeld) {
            state.isHeld = true;
            state.holdProcessed = false;  // Allow hold event to be processed
        }
    }
}

bool AdaptiveButtonTracker::shouldBeHeld(uint8_t button, uint32_t currentTime) {
    const ButtonState& state = states_[button];
    
    if (!state.pressed || state.isHeld) {
        return state.isHeld;  // Already held or not pressed
    }
    
    uint32_t holdTime = currentTime - state.pressTime;
    return holdTime >= profile_.threshold;
}

void AdaptiveButtonTracker::addHoldSample(uint32_t duration) {
    holdDurations_[holdDurationIndex_] = duration;
    holdDurationIndex_ = (holdDurationIndex_ + 1) % 16;
    
    if (holdSampleCount_ < 16) {
        ++holdSampleCount_;
    }
    
    ++stats_.samples;
}

uint32_t AdaptiveButtonTracker::calculateOptimalThreshold() const {
    if (holdSampleCount_ < 3) {
        return profile_.threshold;  // Not enough data
    }
    
    // Calculate average of successful hold durations
    uint32_t sum = 0;
    for (uint8_t i = 0; i < holdSampleCount_; ++i) {
        sum += holdDurations_[i];
    }
    
    uint32_t avgDuration = sum / holdSampleCount_;
    
    // Target threshold at 80% of average successful hold duration
    // This reduces false positives while maintaining usability
    uint32_t targetThreshold = static_cast<uint32_t>(avgDuration * 0.8f);
    
    // Clamp to valid range
    return std::max(profile_.minThreshold, std::min(profile_.maxThreshold, targetThreshold));
}

void AdaptiveButtonTracker::updateStats() {
    uint32_t totalEvents = stats_.totalActivations + stats_.falseActivations + stats_.missedActivations;
    
    if (totalEvents > 0) {
        stats_.successRate = static_cast<float>(stats_.totalActivations) / totalEvents;
        stats_.hasData = totalEvents >= 5;
    }
    
    // Calculate average hold duration from recent samples
    if (holdSampleCount_ > 0) {
        uint32_t sum = 0;
        for (uint8_t i = 0; i < holdSampleCount_; ++i) {
            sum += holdDurations_[i];
        }
        stats_.avgHoldDuration = sum / holdSampleCount_;
    }
}

bool AdaptiveButtonTracker::validateState() const {
    // Check profile validity
    if (!profile_.isValid()) {
        return false;
    }
    
    // Check button states are consistent
    for (uint8_t i = 0; i < MAX_BUTTONS; ++i) {
        const ButtonState& state = states_[i];
        
        // If held, must be pressed
        if (state.isHeld && !state.pressed) {
            return false;
        }
        
        // Press time should be before release time
        if (state.releaseTime > 0 && state.pressTime > state.releaseTime) {
            return false;
        }
    }
    
    // Check buffer bounds
    if (holdDurationIndex_ >= 16 || holdSampleCount_ > 16) {
        return false;
    }
    
    return true;
}