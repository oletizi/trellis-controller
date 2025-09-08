#include "InputStateProcessor.h"
#include <algorithm>

InputStateProcessor::InputStateProcessor(Dependencies deps) 
    : dependencies_(std::move(deps)) {
}

std::vector<ControlMessage::Message> InputStateProcessor::translateState(
    const InputState& currentState, 
    const InputState& previousState,
    uint32_t timestamp) {
    
    std::vector<ControlMessage::Message> messages;
    
    // Priority 1: Check for parameter lock exit
    if (isParameterLockExit(currentState, previousState)) {
        messages.push_back(ControlMessage::Message::exitParamLock(timestamp));
        debugLog("Parameter lock exit detected");
        return messages;  // Exit takes priority
    }
    
    // Priority 2: Check for parameter lock entry
    if (isParameterLockEntry(currentState, previousState)) {
        uint32_t changedButtons = findChangedButtons(currentState, previousState);
        uint8_t buttonId = findFirstChangedButton(changedButtons);
        uint8_t track, step;
        getTrackStep(buttonId, track, step);
        messages.push_back(ControlMessage::Message::enterParamLock(track, step, timestamp));
        debugLog("Parameter lock entry detected for button " + std::to_string(buttonId));
        return messages;
    }
    
    // Priority 3: Parameter adjustments (while in parameter lock)
    if (isParameterAdjustment(currentState, previousState)) {
        uint32_t changedButtons = findChangedButtons(currentState, previousState);
        uint8_t buttonId = findFirstChangedButton(changedButtons);
        
        // Map button to parameter type and delta
        uint8_t track, step;
        getTrackStep(buttonId, track, step);
        
        // Simple mapping: different tracks = different parameters
        uint8_t paramType = track + 1;  // Parameter types 1-4
        int8_t delta = (step < 4) ? -1 : +1;  // Left half = decrease, right half = increase
        
        messages.push_back(ControlMessage::Message::adjustParameter(paramType, delta, timestamp));
        debugLog("Parameter adjustment: type=" + std::to_string(paramType) + " delta=" + std::to_string(delta));
        return messages;
    }
    
    // Priority 4: Normal step toggles
    if (isStepToggle(currentState, previousState)) {
        uint32_t changedButtons = findChangedButtons(currentState, previousState);
        
        // Generate toggle for each changed button
        for (uint8_t buttonId = 0; buttonId < 32; ++buttonId) {
            if (changedButtons & (1U << buttonId)) {
                if (isButtonRelease(buttonId, currentState, previousState)) {
                    uint8_t track, step;
                    getTrackStep(buttonId, track, step);
                    messages.push_back(ControlMessage::Message::toggleStep(track, step, timestamp));
                    debugLog("Step toggle: track=" + std::to_string(track) + " step=" + std::to_string(step));
                }
            }
        }
    }
    
    return messages;
}

bool InputStateProcessor::isParameterLockExit(const InputState& current, const InputState& previous) const {
    // Must be in parameter lock mode in BOTH states (not just entering)
    if (!current.isParameterLockActive() || !previous.isParameterLockActive()) return false;
    
    // Get the button that triggered parameter lock
    uint8_t lockButtonId = current.getLockButtonId();
    
    // Exit happens when lock button is released (transition from pressed to not pressed)
    // This handles both continuous hold-release and tap scenarios
    bool wasPressed = previous.isButtonPressed(lockButtonId);
    bool nowReleased = !current.isButtonPressed(lockButtonId);
    
    return wasPressed && nowReleased;
}

bool InputStateProcessor::isParameterLockEntry(const InputState& current, const InputState& previous) const {
    // Parameter lock entry is when we transition from not-locked to locked
    if (!current.isParameterLockActive() || previous.isParameterLockActive()) return false;
    
    // The state encoder should have set the parameter lock flag when appropriate
    // We just detect the transition
    return true;
}

bool InputStateProcessor::isStepToggle(const InputState& current, const InputState& previous) const {
    // Step toggles only happen when NOT in parameter lock mode
    if (current.isParameterLockActive()) return false;
    
    // Any button release with short duration is a step toggle
    uint32_t changedButtons = findChangedButtons(current, previous);
    
    for (uint8_t buttonId = 0; buttonId < 32; ++buttonId) {
        if (changedButtons & (1U << buttonId)) {
            if (isButtonRelease(buttonId, current, previous)) {
                // Check timing - short press = toggle
                uint8_t timingBucket = current.timingInfo;
                uint32_t approxDurationMs = timingBucket * 20;
                if (approxDurationMs < holdThresholdMs_) {
                    return true;
                }
            }
        }
    }
    
    return false;
}

bool InputStateProcessor::isParameterAdjustment(const InputState& current, const InputState& previous) const {
    // Parameter adjustments only happen IN parameter lock mode
    if (!current.isParameterLockActive()) return false;
    
    // Any button press (not the lock button) is a parameter adjustment
    uint32_t changedButtons = findChangedButtons(current, previous);
    uint8_t lockButtonId = current.getLockButtonId();
    
    for (uint8_t buttonId = 0; buttonId < 32; ++buttonId) {
        if (buttonId == lockButtonId) continue;  // Skip lock button
        
        if (changedButtons & (1U << buttonId)) {
            if (isButtonPress(buttonId, current, previous)) {
                return true;
            }
        }
    }
    
    return false;
}

uint32_t InputStateProcessor::findChangedButtons(const InputState& current, const InputState& previous) const {
    return current.buttonStates ^ previous.buttonStates;
}

uint8_t InputStateProcessor::findFirstChangedButton(uint32_t changedMask) const {
    if (changedMask == 0) return 32;  // No changed buttons
    
    // Find first set bit (least significant)
    for (uint8_t i = 0; i < 32; ++i) {
        if (changedMask & (1U << i)) {
            return i;
        }
    }
    
    return 32;
}

void InputStateProcessor::getTrackStep(uint8_t buttonId, uint8_t& track, uint8_t& step) const {
    // 4x8 grid layout: 4 tracks, 8 steps per track
    track = buttonId / 8;
    step = buttonId % 8;
}

bool InputStateProcessor::isButtonPress(uint8_t buttonId, const InputState& current, const InputState& previous) const {
    return current.isButtonPressed(buttonId) && !previous.isButtonPressed(buttonId);
}

bool InputStateProcessor::isButtonRelease(uint8_t buttonId, const InputState& current, const InputState& previous) const {
    return !current.isButtonPressed(buttonId) && previous.isButtonPressed(buttonId);
}

void InputStateProcessor::debugLog(const std::string& message) const {
    if (dependencies_.debugOutput) {
        dependencies_.debugOutput->log("InputStateProcessor: " + message);
    }
}