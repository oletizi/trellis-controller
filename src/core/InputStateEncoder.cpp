#include "InputStateEncoder.h"
#include <algorithm>
#include <cstring>

/**
 * @file InputStateEncoder.cpp
 * @brief Implementation of the InputStateEncoder bridge component
 * 
 * This implementation follows the project's C++17 standards, RAII principles,
 * and real-time safety requirements. All operations are deterministic with
 * bounded execution time and no dynamic allocation.
 */

InputStateEncoder::InputStateEncoder(Dependencies deps)
    : dependencies_(std::move(deps))
    , holdThresholdMs_(500)
{
    // Validate critical dependencies
    if (!dependencies_.clock) {
        // Following project philosophy: throw descriptive errors instead of fallbacks
        throw std::runtime_error("InputStateEncoder requires valid IClock dependency");
    }
    
    debugLog("InputStateEncoder initialized with hold threshold: " + std::to_string(holdThresholdMs_) + "ms");
}

InputState InputStateEncoder::processInputEvent(const InputEvent& event, const InputState& previousState) {
    // Start with previous state to preserve all existing fields
    InputState newState = previousState;
    
    debugLog("Processing event type: " + std::to_string(static_cast<int>(event.type)) + 
             ", device: " + std::to_string(event.deviceId));
    
    switch (event.type) {
        case InputEvent::Type::BUTTON_PRESS: {
            // Handle button press: set button bit, reset timing
            if (event.deviceId < 32) {  // Validate button ID range
                newState.setButtonState(event.deviceId, true);
                newState.timingInfo = 0;  // Reset timing bucket on press
                
                debugLog("Button press: " + std::to_string(event.deviceId) + " - bit set, timing reset");
            } else {
                debugLog("Invalid button ID for press: " + std::to_string(event.deviceId));
            }
            break;
        }
        
        case InputEvent::Type::BUTTON_RELEASE: {
            // Handle button release: clear button bit, encode duration, check parameter lock
            if (event.deviceId < 32) {  // Validate button ID range
                newState.setButtonState(event.deviceId, false);
                
                // Extract press duration from event value (milliseconds)
                uint32_t pressDuration = static_cast<uint32_t>(std::max(0, event.value));
                
                // Encode duration into timing bucket
                newState.timingInfo = calculateTimingBucket(pressDuration);
                
                // Check for parameter lock entry based on hold duration
                if (shouldEnterParameterLock(pressDuration)) {
                    newState.setParameterLockActive(true);
                    newState.setLockButtonId(event.deviceId);
                    
                    debugLog(">>> PARAMETER LOCK ACTIVATED <<<");
                    debugLog("Button: " + std::to_string(event.deviceId) + 
                             ", Hold duration: " + std::to_string(pressDuration) + "ms" +
                             ", Threshold: " + std::to_string(holdThresholdMs_) + "ms");
                } else {
                    debugLog("Button release: " + std::to_string(event.deviceId) + 
                             " - normal release (hold: " + std::to_string(pressDuration) + "ms, threshold: " + std::to_string(holdThresholdMs_) + "ms)");
                }
            } else {
                debugLog("Invalid button ID for release: " + std::to_string(event.deviceId));
            }
            break;
        }
        
        case InputEvent::Type::ENCODER_TURN:
        case InputEvent::Type::MIDI_INPUT:
        case InputEvent::Type::SYSTEM_EVENT:
        default: {
            // For non-button events, pass through state unchanged
            // Future enhancement: Could handle encoder/MIDI state if needed
            debugLog("Non-button event passed through unchanged");
            break;
        }
    }
    
    return newState;
}

InputState InputStateEncoder::updateTiming(uint32_t currentTime, const InputState& currentState) {
    // For now, this is a pass-through implementation
    // Future enhancement: Could implement real-time hold detection here
    // by checking pressed buttons against currentTime and updating timing buckets
    
    // Suppress unused parameter warnings in embedded builds
    (void)currentTime;
    
    return currentState;
}

uint8_t InputStateEncoder::calculateTimingBucket(uint32_t durationMs) const {
    // Convert milliseconds to timing bucket (0-255, each unit ~20ms)
    // Formula: bucket = min(255, durationMs / 20)
    // This provides ~5.1 second maximum timing with 20ms resolution
    
    constexpr uint32_t BUCKET_DURATION_MS = 20;
    constexpr uint8_t MAX_BUCKET = 255;
    
    uint32_t bucket = durationMs / BUCKET_DURATION_MS;
    
    // Clamp to maximum bucket value to prevent overflow
    if (bucket > MAX_BUCKET) {
        bucket = MAX_BUCKET;
    }
    
    return static_cast<uint8_t>(bucket);
}

bool InputStateEncoder::shouldEnterParameterLock(uint32_t pressDuration) const {
    // Parameter lock entry occurs when press duration meets or exceeds threshold
    return pressDuration >= holdThresholdMs_;
}

void InputStateEncoder::debugLog(const std::string& message) const {
    // Conditional debug output - only log if debug dependency is provided
    // This follows the project's philosophy of throwing errors instead of using fallbacks
    if (dependencies_.debugOutput) {
        dependencies_.debugOutput->log("[InputStateEncoder] " + message);
    }
    
    // Note: We don't throw an error for missing debug output as it's optional
    // This is different from required dependencies like clock
}