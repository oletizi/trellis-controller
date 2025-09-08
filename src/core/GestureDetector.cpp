#include "GestureDetector.h"
#include <algorithm>

GestureDetector::GestureDetector(const InputSystemConfiguration& config,
                                 IClock* clock,
                                 IDebugOutput* debug)
    : config_(config), clock_(clock), debug_(debug) {
    reset();
}

uint8_t GestureDetector::processInputEvent(const InputEvent& inputEvent, 
                                          std::vector<ControlMessage::Message>& controlMessages) {
    if (inputEvent.type == InputEvent::Type::BUTTON_PRESS) {
        return processButtonPress(inputEvent.deviceId, inputEvent.timestamp, controlMessages);
    } else if (inputEvent.type == InputEvent::Type::BUTTON_RELEASE) {
        uint32_t pressDuration = static_cast<uint32_t>(inputEvent.value);
        return processButtonRelease(inputEvent.deviceId, inputEvent.timestamp, pressDuration, controlMessages);
    }
    
    return 0; // Other event types not handled by gesture detector
}

uint8_t GestureDetector::updateTiming(uint32_t currentTime,
                                     std::vector<ControlMessage::Message>& controlMessages) {
    uint8_t messagesGenerated = 0;
    
    // Check for hold detection on pressed buttons
    messagesGenerated += checkForHoldDetection(currentTime, controlMessages);
    
    // Check for parameter lock timeout
    messagesGenerated += checkForParameterLockTimeout(currentTime, controlMessages);
    
    return messagesGenerated;
}

void GestureDetector::reset() {
    // Reset all button states
    for (auto& state : buttonStates_) {
        state = ButtonState{};
    }
    
    // Reset parameter lock state
    paramLockState_ = ParameterLockState{};
    
    debugLog("GestureDetector reset");
}

uint8_t GestureDetector::getCurrentButtonStates(bool* buttonStates, uint8_t maxButtons) const {
    if (!buttonStates) return 0;
    
    uint8_t count = std::min(static_cast<uint8_t>(32), maxButtons);
    for (uint8_t i = 0; i < count; ++i) {
        buttonStates[i] = buttonStates_[i].pressed;
    }
    
    return count;
}

bool GestureDetector::isInParameterLockMode() const {
    return paramLockState_.active;
}

void GestureDetector::setConfiguration(const InputSystemConfiguration& config) {
    config_ = config;
    debugLog("GestureDetector configuration updated");
}

InputSystemConfiguration GestureDetector::getConfiguration() const {
    return config_;
}

uint8_t GestureDetector::processButtonPress(uint8_t buttonId, uint32_t timestamp,
                                           std::vector<ControlMessage::Message>& controlMessages) {
    if (buttonId >= 32) return 0;
    
    // Update button state
    ButtonState& state = buttonStates_[buttonId];
    state.pressed = true;
    state.pressStartTime = timestamp;
    state.lastEventTime = timestamp;
    state.holdDetected = false;
    state.holdMessageSent = false;
    
    debugLog("Button " + std::to_string(buttonId) + " pressed");
    
    // In parameter lock mode, button presses have special behavior
    if (paramLockState_.active) {
        paramLockState_.lastActivityTime = timestamp;
        
        // Check if this is the lock button - if so, exit parameter lock on next release
        uint8_t lockButtonId = trackStepToButtonIndex(paramLockState_.lockedTrack, paramLockState_.lockedStep);
        if (buttonId == lockButtonId) {
            debugLog("Lock button pressed in parameter lock mode - will exit on release");
            // Don't generate parameter adjustment for lock button
            return 0;
        } else {
            // Other buttons adjust parameters
            controlMessages.push_back(createParameterAdjustMessage(buttonId, timestamp));
            debugLog("Parameter adjustment for button " + std::to_string(buttonId));
            return 1;
        }
    }
    
    // In normal mode, press starts hold detection (release will determine tap vs hold)
    return 0;
}

uint8_t GestureDetector::processButtonRelease(uint8_t buttonId, uint32_t timestamp, uint32_t pressDuration,
                                             std::vector<ControlMessage::Message>& controlMessages) {
    if (buttonId >= 32) return 0;
    
    ButtonState& state = buttonStates_[buttonId];
    
    // Special case: In parameter lock mode, allow release of lock button even if not currently pressed
    bool isLockButtonRelease = false;
    if (paramLockState_.active) {
        uint8_t lockButtonId = trackStepToButtonIndex(paramLockState_.lockedTrack, paramLockState_.lockedStep);
        isLockButtonRelease = (buttonId == lockButtonId);
    }
    
    if (!state.pressed && !isLockButtonRelease) {
        return 0; // Ignore release for unpressed button (except lock button in parameter lock mode)
    }
    
    state.pressed = false;
    state.lastEventTime = timestamp;
    
    debugLog("Button " + std::to_string(buttonId) + " released (duration: " + std::to_string(pressDuration) + "ms)");
    
    // If this was the parameter lock button, exit parameter lock mode
    if (paramLockState_.active) {
        uint8_t lockButtonId = trackStepToButtonIndex(paramLockState_.lockedTrack, paramLockState_.lockedStep);
        if (buttonId == lockButtonId) {
            controlMessages.push_back(createParameterLockExitMessage(timestamp));
            paramLockState_.active = false;
            debugLog("Exited parameter lock mode");
            return 1;
        }
    }
    
    // Check if this was a hold (exceeded hold threshold)
    bool wasHold = pressDuration >= config_.timing.holdThresholdMs;
    
    if (wasHold) {
        // Hold detected - enter parameter lock if not already active
        if (!paramLockState_.active) {
            uint8_t track, step;
            buttonIndexToTrackStep(buttonId, track, step);
            
            paramLockState_.active = true;
            paramLockState_.lockedTrack = track;
            paramLockState_.lockedStep = step;
            paramLockState_.lockStartTime = timestamp;
            paramLockState_.lastActivityTime = timestamp;
            
            controlMessages.push_back(createParameterLockEntryMessage(buttonId, timestamp));
            debugLog("Entered parameter lock mode for track " + std::to_string(track) + " step " + std::to_string(step));
            return 1;
        }
    } else {
        // Short press - toggle step if not in parameter lock mode
        if (!paramLockState_.active) {
            controlMessages.push_back(createStepToggleMessage(buttonId, timestamp));
            debugLog("Step toggle for button " + std::to_string(buttonId));
            return 1;
        }
    }
    
    return 0;
}

uint8_t GestureDetector::checkForHoldDetection(uint32_t currentTime,
                                              std::vector<ControlMessage::Message>& /* controlMessages */) {
    uint8_t messagesGenerated = 0;
    
    for (uint8_t i = 0; i < 32; ++i) {
        ButtonState& state = buttonStates_[i];
        
        if (state.pressed && !state.holdMessageSent) {
            uint32_t holdDuration = currentTime - state.pressStartTime;
            
            if (holdDuration >= config_.timing.holdThresholdMs) {
                state.holdDetected = true;
                state.holdMessageSent = true;
                
                // Generate hold indication if needed for UI feedback
                // (actual parameter lock entry happens on release)
                debugLog("Hold detected for button " + std::to_string(i));
            }
        }
    }
    
    return messagesGenerated;
}

uint8_t GestureDetector::checkForParameterLockTimeout(uint32_t currentTime,
                                                     std::vector<ControlMessage::Message>& controlMessages) {
    if (!paramLockState_.active) return 0;
    
    // Check for timeout (configurable, default 5 seconds)
    uint32_t timeoutMs = 5000; // Could be made configurable
    uint32_t inactiveTime = currentTime - paramLockState_.lastActivityTime;
    
    if (inactiveTime >= timeoutMs) {
        controlMessages.push_back(createParameterLockExitMessage(currentTime));
        paramLockState_.active = false;
        debugLog("Parameter lock mode timed out");
        return 1;
    }
    
    return 0;
}

ControlMessage::Message GestureDetector::createStepToggleMessage(uint8_t buttonId, uint32_t timestamp) {
    uint8_t track, step;
    buttonIndexToTrackStep(buttonId, track, step);
    return ControlMessage::Message::toggleStep(track, step, timestamp);
}

ControlMessage::Message GestureDetector::createParameterLockEntryMessage(uint8_t buttonId, uint32_t timestamp) {
    uint8_t track, step;
    buttonIndexToTrackStep(buttonId, track, step);
    return ControlMessage::Message::enterParamLock(track, step, timestamp);
}

ControlMessage::Message GestureDetector::createParameterLockExitMessage(uint32_t timestamp) {
    return ControlMessage::Message::exitParamLock(timestamp);
}

ControlMessage::Message GestureDetector::createParameterAdjustMessage(uint8_t buttonId, uint32_t timestamp) {
    uint8_t paramType = mapButtonToParameterType(buttonId);
    int8_t delta = calculateParameterDelta(buttonId);
    return ControlMessage::Message::adjustParameter(paramType, delta, timestamp);
}

void GestureDetector::buttonIndexToTrackStep(uint8_t buttonId, uint8_t& track, uint8_t& step) const {
    track = buttonId / 8; // 8 steps per track
    step = buttonId % 8;
}

uint8_t GestureDetector::trackStepToButtonIndex(uint8_t track, uint8_t step) const {
    return track * 8 + step;
}

uint8_t GestureDetector::mapButtonToParameterType(uint8_t buttonId) const {
    // Simple mapping - could be more sophisticated
    uint8_t track, step;
    buttonIndexToTrackStep(buttonId, track, step);
    
    // Different tracks control different parameters
    switch (track) {
        case 0: return 1; // Note/pitch
        case 1: return 2; // Velocity  
        case 2: return 3; // Length
        case 3: return 4; // Probability
        default: return 1;
    }
}

int8_t GestureDetector::calculateParameterDelta(uint8_t buttonId) const {
    uint8_t track, step;
    buttonIndexToTrackStep(buttonId, track, step);
    
    // Lower steps = negative delta, upper steps = positive delta
    return (step < 4) ? -1 : +1;
}

void GestureDetector::debugLog(const std::string& message) const {
    if (debug_) {
        debug_->log("GestureDetector: " + message);
    }
}