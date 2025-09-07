#include "SequencerStateManager.h"

// Mode transition matrix - defines which transitions are allowed
// Rows are current modes, columns are target modes
const bool SequencerStateManager::transitionMatrix_[static_cast<uint8_t>(Mode::MODE_COUNT)]
                                                   [static_cast<uint8_t>(Mode::MODE_COUNT)] = {
    // TO:   NORMAL, PARAM_LOCK, PATTERN_SEL, SHIFT_CTRL, SETTINGS
    /*NORMAL*/      { true,  true,        true,        true,       true  },
    /*PARAM_LOCK*/  { true,  false,       false,       false,      false },
    /*PATTERN_SEL*/ { true,  false,       true,        false,      false },
    /*SHIFT_CTRL*/  { true,  true,        true,        true,       true  },
    /*SETTINGS*/    { true,  false,       false,       false,      true  }
};

SequencerStateManager::SequencerStateManager()
    : currentMode_(Mode::NORMAL)
    , previousMode_(Mode::NORMAL)
    , modeStartTime_(0)
    , parameterLockContext_() {
}

void SequencerStateManager::update(uint32_t currentTime) {
    // Check for timeouts
    if (hasTimedOut(currentTime)) {
        switch (currentMode_) {
            case Mode::PARAMETER_LOCK:
                exitParameterLockMode();
                break;
            default:
                // Other modes may have different timeout behavior
                break;
        }
    }
    
    // Update mode-specific state
    switch (currentMode_) {
        case Mode::PARAMETER_LOCK:
            // Validate parameter lock context is still valid
            if (!validateParameterLockContext()) {
                forceExitToNormal();
            }
            break;
        default:
            break;
    }
}

bool SequencerStateManager::canTransitionTo(Mode newMode) const {
    uint8_t currentIdx = static_cast<uint8_t>(currentMode_);
    uint8_t newIdx = static_cast<uint8_t>(newMode);
    
    if (currentIdx >= static_cast<uint8_t>(Mode::MODE_COUNT) ||
        newIdx >= static_cast<uint8_t>(Mode::MODE_COUNT)) {
        return false;
    }
    
    return transitionMatrix_[currentIdx][newIdx];
}

SequencerStateManager::TransitionResult 
SequencerStateManager::enterParameterLockMode(uint8_t track, uint8_t step) {
    // Validate parameters
    if (track >= 4 || step >= 8) {
        return TransitionResult::INVALID_MODE;
    }
    
    // Check if transition is allowed
    if (!canTransitionTo(Mode::PARAMETER_LOCK)) {
        return TransitionResult::TRANSITION_BLOCKED;
    }
    
    // Set up parameter lock context
    parameterLockContext_.active = true;
    parameterLockContext_.heldStep = step;
    parameterLockContext_.heldTrack = track;
    parameterLockContext_.holdStartTime = 0; // Will be set by calling code
    parameterLockContext_.previousMode = currentMode_;
    parameterLockContext_.calculateControlGrid();
    
    // Validate context
    if (!validateParameterLockContext()) {
        parameterLockContext_.active = false;
        return TransitionResult::ERROR;
    }
    
    // Perform transition
    return internalTransition(Mode::PARAMETER_LOCK);
}

SequencerStateManager::TransitionResult SequencerStateManager::exitParameterLockMode() {
    if (currentMode_ != Mode::PARAMETER_LOCK) {
        return TransitionResult::INVALID_MODE;
    }
    
    Mode targetMode = parameterLockContext_.previousMode;
    parameterLockContext_.active = false;
    
    return internalTransition(targetMode);
}

void SequencerStateManager::forceExitToNormal() {
    parameterLockContext_.active = false;
    internalTransition(Mode::NORMAL, true); // Force transition
}

SequencerStateManager::TransitionResult SequencerStateManager::transitionToMode(Mode newMode) {
    if (!canTransitionTo(newMode)) {
        return TransitionResult::TRANSITION_BLOCKED;
    }
    
    return internalTransition(newMode);
}

bool SequencerStateManager::isInControlGrid(uint8_t buttonIndex) const {
    if (!isInParameterLockMode()) {
        return false;
    }
    
    // Button layout: 4x8 grid
    // Buttons 0-7 are row 0, 8-15 are row 1, etc.
    uint8_t col = buttonIndex % 8;
    
    // Control grid is either columns 0-3 or 4-7
    if (parameterLockContext_.controlGridStart == 0) {
        return col < 4;  // Columns 0-3
    } else {
        return col >= 4; // Columns 4-7
    }
}

uint8_t SequencerStateManager::getControlGridButtons(uint8_t* controlButtons, uint8_t maxButtons) const {
    if (!isInParameterLockMode() || controlButtons == nullptr || maxButtons == 0) {
        return 0;
    }
    
    uint8_t count = 0;
    uint8_t startCol = parameterLockContext_.controlGridStart;
    
    // Fill control grid buttons (4x4 grid)
    for (uint8_t row = 0; row < 4 && count < maxButtons; ++row) {
        for (uint8_t col = 0; col < 4 && count < maxButtons; ++col) {
            controlButtons[count] = row * 8 + startCol + col;
            ++count;
        }
    }
    
    return count;
}

bool SequencerStateManager::hasTimedOut(uint32_t currentTime) const {
    switch (currentMode_) {
        case Mode::PARAMETER_LOCK:
            if (parameterLockContext_.timeoutMs > 0) {
                uint32_t elapsed = currentTime - parameterLockContext_.holdStartTime;
                return elapsed >= parameterLockContext_.timeoutMs;
            }
            return false;
        default:
            return false;
    }
}

const char* SequencerStateManager::getModeName(Mode mode) {
    switch (mode) {
        case Mode::NORMAL:         return "NORMAL";
        case Mode::PARAMETER_LOCK: return "PARAMETER_LOCK";
        case Mode::PATTERN_SELECT: return "PATTERN_SELECT";
        case Mode::SHIFT_CONTROL:  return "SHIFT_CONTROL";
        case Mode::SETTINGS:       return "SETTINGS";
        default:                   return "UNKNOWN";
    }
}

bool SequencerStateManager::validateState() const {
    // Check mode is valid
    if (static_cast<uint8_t>(currentMode_) >= static_cast<uint8_t>(Mode::MODE_COUNT)) {
        return false;
    }
    
    // Check mode-specific state
    switch (currentMode_) {
        case Mode::PARAMETER_LOCK:
            return parameterLockContext_.active && validateParameterLockContext();
        default:
            return true;
    }
}

SequencerStateManager::TransitionResult 
SequencerStateManager::internalTransition(Mode newMode, bool forced) {
    // Validate transition unless forced
    if (!forced && !canTransitionTo(newMode)) {
        return TransitionResult::TRANSITION_BLOCKED;
    }
    
    Mode oldMode = currentMode_;
    
    // Exit current mode
    onExitMode(oldMode);
    
    // Update state
    previousMode_ = oldMode;
    currentMode_ = newMode;
    modeStartTime_ = 0; // Will be set by calling code if needed
    
    // Enter new mode
    onEnterMode(newMode);
    
    return TransitionResult::SUCCESS;
}

void SequencerStateManager::onEnterMode(Mode mode) {
    switch (mode) {
        case Mode::PARAMETER_LOCK:
            // Parameter lock context should already be set up
            break;
        case Mode::NORMAL:
            // Clear any mode-specific state
            parameterLockContext_.active = false;
            break;
        default:
            break;
    }
}

void SequencerStateManager::onExitMode(Mode mode) {
    switch (mode) {
        case Mode::PARAMETER_LOCK:
            // Clean up parameter lock context
            parameterLockContext_.active = false;
            break;
        default:
            break;
    }
}

bool SequencerStateManager::validateParameterLockContext() const {
    if (!parameterLockContext_.active) {
        return false;
    }
    
    return parameterLockContext_.isValid();
}