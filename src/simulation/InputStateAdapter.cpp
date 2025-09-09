#include "InputStateAdapter.h"
#include "../core/InputEvent.h"
#include "../core/ControlMessage.h"
#include <algorithm>

InputStateAdapter::InputStateAdapter(Dependencies deps) 
    : dependencies_(std::move(deps)),
      processor_({deps.clock, deps.debugOutput}) {
    
    if (!dependencies_.cursesLayer || !dependencies_.clock) {
        throw std::invalid_argument("InputStateAdapter: Invalid dependencies");
    }
    
    processor_.setHoldThreshold(500);  // 500ms for parameter lock
}

std::vector<ControlMessage::Message> InputStateAdapter::poll() {
    if (!dependencies_.cursesLayer->poll()) {
        debugLog("CursesInputLayer poll failed");
        return {};
    }
    
    // Store previous state
    previousState_ = currentState_;
    
    // Build current state from CursesInputLayer events
    currentState_ = buildStateFromEvents();
    
    // Generate control messages from state transition
    uint32_t timestamp = dependencies_.clock->getCurrentTime();
    auto messages = processor_.translateState(currentState_, previousState_, timestamp);
    
    if (!messages.empty()) {
        debugLog("Generated " + std::to_string(messages.size()) + " control messages from state transition");
    }
    
    return messages;
}

InputState InputStateAdapter::buildStateFromEvents() {
    // Start with previous state
    InputState newState = currentState_;
    
    // Process all available events from CursesInputLayer
    InputEvent event;
    while (dependencies_.cursesLayer->getNextEvent(event)) {
        processInputEvent(event, newState);
    }
    
    return newState;
}

void InputStateAdapter::processInputEvent(const InputEvent& event, InputState& state) {
    if (event.deviceId >= 32) return;
    
    uint32_t timestamp = event.timestamp;
    uint8_t buttonId = event.deviceId;
    
    switch (event.type) {
        case InputEvent::Type::BUTTON_PRESS:
            debugLog("Button " + std::to_string(buttonId) + " pressed at " + std::to_string(timestamp));
            state.setButtonState(buttonId, true);
            break;
            
        case InputEvent::Type::BUTTON_RELEASE: {
            uint32_t pressDuration = static_cast<uint32_t>(event.value);
            debugLog("Button " + std::to_string(buttonId) + " released after " + std::to_string(pressDuration) + "ms");
            
            state.setButtonState(buttonId, false);
            
            // Set timing information
            uint8_t timingBucket = calculateTimingBucket(pressDuration);
            state.timingInfo = timingBucket;
            
            // Check if this should enter parameter lock
            if (shouldEnterParameterLock(buttonId, pressDuration)) {
                state.setParameterLockActive(true);
                state.setLockButtonId(buttonId);
                debugLog("State encoded for parameter lock entry: button " + std::to_string(buttonId));
            }
            break;
        }
        
        case InputEvent::Type::SYSTEM_EVENT:
            // Handle raw keyboard events from CursesInputLayer
            // These use SYSTEM_EVENT with buttonId as deviceId, keyCode as value, uppercase flag as context
            if (event.deviceId < 32 && event.value != 1) {  // Not a quit event (value=1 is quit)
                // This is a raw keyboard event from CursesInputLayer
                bool isUppercase = (event.context == 1);
                
                if (isUppercase) {
                    // Uppercase = button press semantic
                    debugLog("Raw keyboard uppercase -> Button " + std::to_string(buttonId) + " PRESS");
                    state.setButtonState(buttonId, true);
                } else {
                    // Lowercase = button release semantic
                    debugLog("Raw keyboard lowercase -> Button " + std::to_string(buttonId) + " RELEASE");
                    state.setButtonState(buttonId, false);
                    
                    // For releases, we don't have duration info from raw events
                    // Duration tracking would need to be handled by tracking press timestamps
                }
            }
            // Pass through other system events unchanged
            break;
            
        default:
            // Ignore other event types
            break;
    }
}

uint8_t InputStateAdapter::calculateTimingBucket(uint32_t durationMs) const {
    // Each bucket represents ~20ms, up to 255 buckets (5.1 seconds)
    return std::min(255U, durationMs / 20);
}

bool InputStateAdapter::shouldEnterParameterLock(uint8_t buttonId, uint32_t durationMs) const {
    // Enter parameter lock if hold duration exceeds threshold and not already in param lock
    return durationMs >= processor_.getHoldThreshold() && !currentState_.isParameterLockActive();
}

void InputStateAdapter::debugLog(const std::string& message) const {
    if (dependencies_.debugOutput) {
        dependencies_.debugOutput->log("InputStateAdapter: " + message);
    }
}