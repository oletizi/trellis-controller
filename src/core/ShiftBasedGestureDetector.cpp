#include "ShiftBasedGestureDetector.h"
#include <cstring>
#include <algorithm>

/**
 * @file ShiftBasedGestureDetector.cpp
 * @brief Implementation of SHIFT-based parameter lock gesture detection
 * 
 * Provides deterministic, bank-aware parameter lock functionality using
 * explicit SHIFT button patterns instead of timing-based detection.
 * Integrates with the 64-bit InputState system for reliable state management.
 */

ShiftBasedGestureDetector::ShiftBasedGestureDetector(InputStateProcessor* stateProcessor)
    : currentState_(0, false, 0, 0)  // Initialize empty state
    , stateProcessor_(stateProcessor)
    , parameterLockActive_(false)
    , lockButtonId_(255)             // Invalid button ID
    , activeBankId_(BankId::LEFT_BANK)
    , buttonStates_(0)
{
    // Initialize with default configuration
    config_ = InputSystemConfiguration::forNeoTrellis();
    
    // No timing tracking needed for SHIFT-based detection
}

uint8_t ShiftBasedGestureDetector::processInputEvent(const InputEvent& inputEvent, 
                                                    std::vector<ControlMessage::Message>& controlMessages) {
    uint8_t messagesGenerated = 0;
    
    // Update button state tracking for all button events
    if (inputEvent.type == InputEvent::Type::BUTTON_PRESS || 
        inputEvent.type == InputEvent::Type::SHIFT_BUTTON_PRESS) {
        updateButtonState(inputEvent.deviceId, inputEvent.value != 0);
    } else if (inputEvent.type == InputEvent::Type::BUTTON_RELEASE || 
               inputEvent.type == InputEvent::Type::SHIFT_BUTTON_RELEASE) {
        updateButtonState(inputEvent.deviceId, false);
    }
    
    // Process based on event type and current state
    switch (inputEvent.type) {
        case InputEvent::Type::SHIFT_BUTTON_PRESS:
            if (inputEvent.value != 0) {  // Press, not release
                messagesGenerated += processShiftButtonPress(inputEvent, controlMessages);
            }
            break;
            
        case InputEvent::Type::BUTTON_PRESS:
            if (inputEvent.value != 0) {  // Press, not release
                if (parameterLockActive_) {
                    messagesGenerated += processParameterLockButton(inputEvent, controlMessages);
                } else {
                    messagesGenerated += processStepToggle(inputEvent, controlMessages);
                }
                // SHIFT-based detection doesn't need timing tracking
            }
            break;
            
        case InputEvent::Type::BUTTON_RELEASE:
        case InputEvent::Type::SHIFT_BUTTON_RELEASE:
            // No special handling needed for releases in SHIFT-based detection
            break;
            
        default:
            // Other event types (encoder, MIDI, system) pass through unchanged
            break;
    }
    
    return messagesGenerated;
}

uint8_t ShiftBasedGestureDetector::updateTiming(uint32_t currentTime,
                                               std::vector<ControlMessage::Message>& controlMessages) {
    // SHIFT-based gesture detection doesn't rely on timing,
    // so this is essentially a no-op for our implementation
    (void)currentTime;  // Suppress unused parameter warning
    (void)controlMessages;
    return 0;
}

void ShiftBasedGestureDetector::reset() {
    // Clear parameter lock state
    parameterLockActive_ = false;
    lockButtonId_ = 255;
    activeBankId_ = BankId::LEFT_BANK;
    
    // Clear button states
    buttonStates_ = 0;
    currentState_ = InputState(0, false, 0, 0);
    
    // No timing tracking to clear for SHIFT-based detection
    
    // Reset state processor if available
    if (stateProcessor_) {
        // State processor doesn't have a reset method in the interface,
        // so we just clear our local state
    }
}

uint8_t ShiftBasedGestureDetector::getCurrentButtonStates(bool* buttonStates, uint8_t maxButtons) const {
    if (!buttonStates || maxButtons == 0) {
        return 0;
    }
    
    uint8_t numButtons = (maxButtons > 32) ? 32 : maxButtons;
    
    for (uint8_t i = 0; i < numButtons; ++i) {
        buttonStates[i] = (buttonStates_ & (1U << i)) != 0;
    }
    
    return numButtons;
}

bool ShiftBasedGestureDetector::isInParameterLockMode() const {
    return parameterLockActive_;
}

void ShiftBasedGestureDetector::setConfiguration(const struct InputSystemConfiguration& config) {
    config_ = config;
    
    // Update state processor configuration if available
    if (stateProcessor_) {
        // State processor configuration would be updated through its own interface
        // For now, we just store the configuration locally
    }
}

uint8_t ShiftBasedGestureDetector::processShiftButtonPress(const InputEvent& inputEvent,
                                                         std::vector<ControlMessage::Message>& controlMessages) {
    // Determine which bank this button belongs to
    bool isLeftBank = BankMapping::isLeftBankButton(inputEvent.deviceId);
    bool isRightBank = BankMapping::isRightBankButton(inputEvent.deviceId);
    
    if (!isLeftBank && !isRightBank) {
        // Button doesn't belong to either bank, ignore
        return 0;
    }
    
    // Enter parameter lock mode
    parameterLockActive_ = true;
    lockButtonId_ = inputEvent.deviceId;
    
    // Bank-aware control mapping:
    // Left bank triggers → Right bank controls
    // Right bank triggers → Left bank controls  
    activeBankId_ = isLeftBank ? BankId::RIGHT_BANK : BankId::LEFT_BANK;
    
    // Update 64-bit state encoding
    currentState_.setParameterLockActive(true);
    currentState_.setLockButtonId(inputEvent.deviceId);
    
    // Generate ENTER_PARAM_LOCK message
    uint8_t track, step;
    buttonToTrackStep(inputEvent.deviceId, track, step);
    controlMessages.push_back(createMessage(ControlMessage::Type::ENTER_PARAM_LOCK, track, step));
    
    return 1;
}

uint8_t ShiftBasedGestureDetector::processParameterLockButton(const InputEvent& inputEvent,
                                                            std::vector<ControlMessage::Message>& controlMessages) {
    // Check if this is the lock button (exit parameter lock)
    if (inputEvent.deviceId == lockButtonId_) {
        return exitParameterLock(controlMessages);
    }
    
    // Check if this is a control button for the active bank
    ParameterType paramType;
    int8_t delta;
    
    if (BankMapping::getParameterAdjustment(inputEvent.deviceId, activeBankId_, paramType, delta)) {
        // Generate ADJUST_PARAMETER message
        controlMessages.push_back(createMessage(ControlMessage::Type::ADJUST_PARAMETER, 
                                               static_cast<uint8_t>(paramType), 
                                               static_cast<uint32_t>(delta)));
        return 1;
    }
    
    // Button is not a control button for this bank, ignore
    return 0;
}

uint8_t ShiftBasedGestureDetector::processStepToggle(const InputEvent& inputEvent,
                                                   std::vector<ControlMessage::Message>& controlMessages) {
    // Generate TOGGLE_STEP message for normal button press
    uint8_t track, step;
    buttonToTrackStep(inputEvent.deviceId, track, step);
    
    controlMessages.push_back(createMessage(ControlMessage::Type::TOGGLE_STEP, track, step));
    return 1;
}

uint8_t ShiftBasedGestureDetector::exitParameterLock(std::vector<ControlMessage::Message>& controlMessages) {
    // Clear parameter lock state
    parameterLockActive_ = false;
    lockButtonId_ = 255;
    
    // Update 64-bit state encoding
    currentState_.setParameterLockActive(false);
    currentState_.setLockButtonId(0);
    
    // Generate EXIT_PARAM_LOCK message
    controlMessages.push_back(createMessage(ControlMessage::Type::EXIT_PARAM_LOCK));
    
    return 1;
}

void ShiftBasedGestureDetector::updateButtonState(uint8_t buttonId, bool pressed) {
    if (buttonId >= 32) {
        return;  // Invalid button ID
    }
    
    // Update local button state tracking
    if (pressed) {
        buttonStates_ |= (1U << buttonId);
    } else {
        buttonStates_ &= ~(1U << buttonId);
    }
    
    // Update 64-bit state encoding
    currentState_.setButtonState(buttonId, pressed);
}

void ShiftBasedGestureDetector::buttonToTrackStep(uint8_t buttonId, uint8_t& track, uint8_t& step) const {
    // Assume 4x8 grid layout (4 tracks, 8 steps)
    // Button layout: 
    // Track 0: buttons 0-7   (steps 0-7)
    // Track 1: buttons 8-15  (steps 0-7)  
    // Track 2: buttons 16-23 (steps 0-7)
    // Track 3: buttons 24-31 (steps 0-7)
    
    if (buttonId >= 32) {
        track = 0;
        step = 0;
        return;
    }
    
    track = buttonId / 8;
    step = buttonId % 8;
}

ControlMessage::Message ShiftBasedGestureDetector::createMessage(ControlMessage::Type type, 
                                                               uint32_t param1, uint32_t param2) const {
    // Use current time as timestamp (could get from clock dependency if available)
    uint32_t timestamp = 0;  // For now, use 0 - could be enhanced with actual timing
    
    return ControlMessage::Message(type, timestamp, param1, param2);
}

// BankMapping static method implementations

bool ShiftBasedGestureDetector::BankMapping::isLeftBankButton(uint8_t buttonId) {
    // Left bank includes buttons: 1-4, qwer, asdf, zxcv
    // For NeoTrellis 4x8 layout, this translates to specific button indices
    
    // Row 0: buttons 0-3 (1-4 keys)
    if (buttonId >= 0 && buttonId <= 3) {
        return true;
    }
    
    // Row 1: buttons 8-11 (qwer keys)  
    if (buttonId >= 8 && buttonId <= 11) {
        return true;
    }
    
    // Row 2: buttons 16-19 (asdf keys)
    if (buttonId >= 16 && buttonId <= 19) {
        return true;
    }
    
    // Row 3: buttons 24-27 (zxcv keys)
    if (buttonId >= 24 && buttonId <= 27) {
        return true;
    }
    
    return false;
}

bool ShiftBasedGestureDetector::BankMapping::isRightBankButton(uint8_t buttonId) {
    // Right bank includes buttons: 5678, tyui, ghjk, bnm,
    // For NeoTrellis 4x8 layout, this translates to specific button indices
    
    // Row 0: buttons 4-7 (5678 keys)
    if (buttonId >= 4 && buttonId <= 7) {
        return true;
    }
    
    // Row 1: buttons 12-15 (tyui keys)
    if (buttonId >= 12 && buttonId <= 15) {
        return true;
    }
    
    // Row 2: buttons 20-23 (ghjk keys)  
    if (buttonId >= 20 && buttonId <= 23) {
        return true;
    }
    
    // Row 3: buttons 28-31 (bnm, keys)
    if (buttonId >= 28 && buttonId <= 31) {
        return true;
    }
    
    return false;
}

bool ShiftBasedGestureDetector::BankMapping::getParameterAdjustment(uint8_t buttonId, BankId activeBankId,
                                                                   ParameterType& paramType, int8_t& delta) {
    if (activeBankId == BankId::LEFT_BANK) {
        // Left bank controls: z=note-, a=note+, x=velocity-, s=velocity+
        switch (buttonId) {
            case LEFT_NOTE_MINUS:   // 'z' key (button 24)
                paramType = ParameterType::NOTE_MINUS;
                delta = -1;
                return true;
            case LEFT_NOTE_PLUS:    // 'a' key (button 16) 
                paramType = ParameterType::NOTE_PLUS;
                delta = 1;
                return true;
            case LEFT_VEL_MINUS:    // 'x' key (button 25)
                paramType = ParameterType::VELOCITY_MINUS;
                delta = -10;
                return true;
            case LEFT_VEL_PLUS:     // 's' key (button 17)
                paramType = ParameterType::VELOCITY_PLUS;
                delta = 10;
                return true;
            default:
                return false;
        }
    } else {
        // Right bank controls: b=note-, g=note+, n=velocity-, h=velocity+
        switch (buttonId) {
            case RIGHT_NOTE_MINUS:  // 'b' key (button 28)
                paramType = ParameterType::NOTE_MINUS;
                delta = -1;
                return true;
            case RIGHT_NOTE_PLUS:   // 'g' key (button 20)
                paramType = ParameterType::NOTE_PLUS;
                delta = 1;
                return true;
            case RIGHT_VEL_MINUS:   // 'n' key (button 29)
                paramType = ParameterType::VELOCITY_MINUS;
                delta = -10;
                return true;
            case RIGHT_VEL_PLUS:    // 'h' key (button 21)
                paramType = ParameterType::VELOCITY_PLUS;
                delta = 10;
                return true;
            default:
                return false;
        }
    }
}