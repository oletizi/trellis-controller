#include "core/StepSequencer.h"
#include <cstring>

StepSequencer::StepSequencer()
    : bpm_(120)
    , stepCount_(MAX_STEPS)
    , currentStep_(0)
    , playing_(false)
    , midiSyncEnabled_(false)
    , ticksPerStep_(0)
    , tickCounter_(0)
    , lastStepTime_(0)
    , clock_(nullptr)
    , midiOutput_(nullptr)
    , midiInput_(nullptr)
    , display_(nullptr)
    , input_(nullptr)
    , ownsClock_(false)
    , paramEngine_(&lockPool_)
    , lastUpdateTime_(0) {
    
    initializePatternData();
    calculateTicksPerStep();
}

StepSequencer::StepSequencer(Dependencies deps)
    : bpm_(120)
    , stepCount_(MAX_STEPS)
    , currentStep_(0)
    , playing_(false)
    , midiSyncEnabled_(false)
    , ticksPerStep_(0)
    , tickCounter_(0)
    , lastStepTime_(0)
    , clock_(deps.clock)
    , midiOutput_(deps.midiOutput)
    , midiInput_(deps.midiInput)
    , display_(deps.display)
    , input_(deps.input)
    , ownsClock_(false)
    , paramEngine_(&lockPool_, deps.clock)
    , lastUpdateTime_(0) {
    
    initializePatternData();
    calculateTicksPerStep();
}

StepSequencer::~StepSequencer() {
    if (ownsClock_ && clock_) {
        delete clock_;
    }
}

void StepSequencer::init(uint16_t bpm, uint8_t steps) {
    bpm_ = bpm;
    stepCount_ = (steps > MAX_STEPS) ? MAX_STEPS : steps;
    calculateTicksPerStep();
    reset();
}

void StepSequencer::tick() {
    uint32_t currentTime = clock_ ? clock_->millis() : 0;
    
    // Update parameter lock system
    if (currentTime != lastUpdateTime_) {
        updateButtonStates(currentTime);
        stateManager_.update(currentTime);
        
        // Check for new hold events in normal mode
        if (!isInParameterLockMode()) {
            checkForHoldEvents();
        }
        
        lastUpdateTime_ = currentTime;
    }
    
    if (!playing_) return;
    
    // Check if it's time for next step
    if (currentTime - lastStepTime_ >= ticksPerStep_) {
        advanceStep();
        lastStepTime_ = currentTime;
    }
}

void StepSequencer::start() {
    playing_ = true;
    lastStepTime_ = clock_ ? clock_->millis() : 0;
    
    if (midiOutput_) {
        midiOutput_->sendStart();
    }
}

void StepSequencer::stop() {
    playing_ = false;
    
    if (midiOutput_) {
        midiOutput_->sendStop();
    }
}

void StepSequencer::reset() {
    currentStep_ = 0;
    tickCounter_ = 0;
    lastStepTime_ = clock_ ? clock_->millis() : 0;
}

void StepSequencer::toggleStep(uint8_t track, uint8_t step) {
    if (track >= MAX_TRACKS || step >= MAX_STEPS) return;
    
    patternData_[track][step].active = !patternData_[track][step].active;
    
    // Also update legacy array for compatibility
    if (track < 4 && step < 8) {
        // Legacy pattern array doesn't exist anymore, but we keep the interface
    }
}

bool StepSequencer::isStepActive(uint8_t track, uint8_t step) const {
    if (track >= MAX_TRACKS || step >= MAX_STEPS) return false;
    return patternData_[track][step].active;
}

void StepSequencer::setTempo(uint16_t bpm) {
    bpm_ = bpm;
    calculateTicksPerStep();
}

void StepSequencer::setTrackVolume(uint8_t track, uint8_t volume) {
    if (track >= MAX_TRACKS) return;
    
    trackVolumes_[track] = volume;
    trackDefaults_[track].velocity = volume; // Sync with parameter system
}

void StepSequencer::setTrackMute(uint8_t track, bool mute) {
    if (track >= MAX_TRACKS) return;
    
    trackMutes_[track] = mute;
    trackDefaults_[track].muted = mute; // Sync with parameter system
}

uint8_t StepSequencer::getTrackVolume(uint8_t track) const {
    return (track < MAX_TRACKS) ? trackVolumes_[track] : 0;
}

bool StepSequencer::isTrackMuted(uint8_t track) const {
    return (track < MAX_TRACKS) ? trackMutes_[track] : false;
}

void StepSequencer::setTrackMidiNote(uint8_t track, uint8_t note) {
    if (track >= MAX_TRACKS) return;
    
    trackMidiNotes_[track] = note;
    trackDefaults_[track].note = note; // Sync with parameter system
}

void StepSequencer::setTrackMidiChannel(uint8_t track, uint8_t channel) {
    if (track >= MAX_TRACKS) return;
    
    trackMidiChannels_[track] = channel;
    trackDefaults_[track].channel = channel; // Sync with parameter system
}

uint8_t StepSequencer::getTrackMidiNote(uint8_t track) const {
    return (track < MAX_TRACKS) ? trackMidiNotes_[track] : 60;
}

uint8_t StepSequencer::getTrackMidiChannel(uint8_t track) const {
    return (track < MAX_TRACKS) ? trackMidiChannels_[track] : 0;
}

void StepSequencer::setMidiSync(bool enabled) {
    midiSyncEnabled_ = enabled;
}

StepSequencer::TrackTrigger StepSequencer::getTriggeredTracks() {
    // This is for testing - return empty trigger
    return {0, 0, false};
}

// Parameter Lock Interface Implementation

void StepSequencer::handleButton(uint8_t button, bool pressed, uint32_t currentTime) {
    // Maintain full button state for proper hold detection
    static uint32_t currentButtonState = 0;
    
    // Update the button state mask
    if (pressed) {
        currentButtonState |= (1UL << button);
    } else {
        currentButtonState &= ~(1UL << button);
    }
    
    // Update button tracker with full state
    buttonTracker_.update(currentButtonState, currentTime);
    
    // DEBUG: Add button state tracking
    #ifdef ARDUINO
    if (pressed) {
        Serial.print("DEBUG: Button ");
        Serial.print(button);
        Serial.print(" pressed, current mask: 0x");
        Serial.println(currentButtonState, HEX);
    }
    #endif
    
    // Record usage for learning
    if (pressed) {
        controlGrid_.recordButtonUsage(button);
    }
    
    // Handle based on current mode
    if (stateManager_.isInParameterLockMode()) {
        handleParameterLockInput(button, pressed);
    } else {
        handleNormalModeButton(button, pressed);
    }
    
    // Update display
    updateVisualFeedback();
}

bool StepSequencer::enterParameterLockMode(uint8_t track, uint8_t step) {
    auto result = stateManager_.enterParameterLockMode(track, step);
    return result == SequencerStateManager::TransitionResult::SUCCESS;
}

bool StepSequencer::exitParameterLockMode() {
    auto result = stateManager_.exitParameterLockMode();
    return result == SequencerStateManager::TransitionResult::SUCCESS;
}

bool StepSequencer::isInParameterLockMode() const {
    return stateManager_.isInParameterLockMode();
}

bool StepSequencer::adjustParameter(ParameterLockPool::ParameterType paramType, int8_t delta) {
    if (!isInParameterLockMode()) return false;
    
    const auto& context = stateManager_.getParameterLockContext();
    uint8_t track = context.heldTrack;
    uint8_t step = context.heldStep;
    
    if (track >= MAX_TRACKS || step >= MAX_STEPS) return false;
    
    // Get or create parameter lock
    StepData& stepData = patternData_[track][step];
    uint8_t lockIndex = stepData.getLockIndex();
    
    if (lockIndex == ParameterLockPool::INVALID_INDEX) {
        // Create new lock
        lockIndex = lockPool_.allocate(track, step);
        if (lockIndex == ParameterLockPool::INVALID_INDEX) {
            return false; // Pool full
        }
        stepData.setLockIndex(lockIndex);
    }
    
    // Adjust parameter
    auto& lock = lockPool_.getLock(lockIndex);
    lock.setParameter(paramType, true);
    
    switch (paramType) {
        case ParameterLockPool::ParameterType::NOTE:
            lock.noteOffset += delta;
            lock.noteOffset = std::max(-64, std::min(63, static_cast<int>(lock.noteOffset)));
            break;
        case ParameterLockPool::ParameterType::VELOCITY:
            lock.velocity = std::max(0, std::min(127, static_cast<int>(lock.velocity) + delta));
            break;
        case ParameterLockPool::ParameterType::LENGTH:
            lock.length = std::max(1, std::min(255, static_cast<int>(lock.length) + delta));
            break;
        default:
            return false;
    }
    
    // Invalidate cached parameters for this step
    paramEngine_.invalidateStep(track, step);
    
    return true;
}

void StepSequencer::clearStepLocks(uint8_t track, uint8_t step) {
    if (track >= MAX_TRACKS || step >= MAX_STEPS) return;
    
    StepData& stepData = patternData_[track][step];
    uint8_t lockIndex = stepData.getLockIndex();
    
    if (lockIndex != ParameterLockPool::INVALID_INDEX) {
        lockPool_.deallocate(lockIndex);
        stepData.clearLock();
        paramEngine_.invalidateStep(track, step);
    }
}

void StepSequencer::clearAllLocks() {
    for (uint8_t track = 0; track < MAX_TRACKS; ++track) {
        for (uint8_t step = 0; step < MAX_STEPS; ++step) {
            clearStepLocks(track, step);
        }
    }
}

void StepSequencer::updateDisplay() {
    updateVisualFeedback();
}

void StepSequencer::setHandPreference(ControlGrid::HandPreference preference) {
    controlGrid_.setHandPreference(preference);
}

// Private Methods

void StepSequencer::initializePatternData() {
    // Initialize pattern data
    for (uint8_t track = 0; track < MAX_TRACKS; ++track) {
        for (uint8_t step = 0; step < MAX_STEPS; ++step) {
            patternData_[track][step] = StepData();
        }
        
        // Initialize track defaults
        trackDefaults_[track] = TrackDefaults();
        trackDefaults_[track].note = 36 + track;  // C2, C#2, D2, D#2
        trackDefaults_[track].channel = 9;        // Channel 10 (drums)
        trackDefaults_[track].velocity = 100;
        
        // Initialize legacy arrays for compatibility
        trackVolumes_[track] = 100;
        trackMutes_[track] = false;
        trackMidiNotes_[track] = 36 + track;
        trackMidiChannels_[track] = 9;
    }
}

void StepSequencer::calculateTicksPerStep() {
    if (bpm_ > 0) {
        ticksPerStep_ = 60000 / (bpm_ * 2); // 16th note timing
    } else {
        ticksPerStep_ = 125; // Default 120 BPM
    }
}

void StepSequencer::advanceStep() {
    currentStep_ = (currentStep_ + 1) % stepCount_;
    ++tickCounter_;
    
    // Pre-calculate parameters for next step
    uint8_t nextStep = (currentStep_ + 1) % stepCount_;
    paramEngine_.prepareNextStep(nextStep, patternData_, trackDefaults_);
    
    sendMidiTriggers();
}

void StepSequencer::sendMidiTriggers() {
    if (!midiOutput_) return;
    
    for (uint8_t track = 0; track < MAX_TRACKS; ++track) {
        if (trackMutes_[track]) continue;
        
        const StepData& stepData = patternData_[track][currentStep_];
        if (!stepData.active) continue;
        
        // Get calculated parameters (pre-computed for performance)
        const CalculatedParameters& params = paramEngine_.getParameters(track, currentStep_);
        
        sendMidiWithParameters(track, params);
    }
}

void StepSequencer::handleMidiInput() {
    // Handle MIDI input if available
    if (midiInput_) {
        // Implementation for MIDI input handling
    }
}

void StepSequencer::updateButtonStates(uint32_t currentTime) {
    // The button tracker is updated in handleButton()
    // This is for any additional state updates needed
}

void StepSequencer::handleNormalModeButton(uint8_t button, bool pressed) {
    // Only process button releases (quick taps) for step toggling
    if (pressed) return;
    
    uint8_t track, step;
    if (!buttonToTrackStep(button, track, step)) return;
    
    // Check if this was a quick tap (not a hold)
    const auto& buttonState = buttonTracker_.getButtonState(button);
    if (buttonState.holdDuration < 500) {  // Less than hold threshold
        toggleStep(track, step);
    }
}

void StepSequencer::handleParameterLockInput(uint8_t button, bool pressed) {
    if (!pressed) return;
    
    const auto& context = stateManager_.getParameterLockContext();
    ControlGrid::ControlMapping mapping = controlGrid_.getMapping(context.heldStep, context.heldTrack);
    
    if (!mapping.isInControlArea(button)) {
        // Button outside control area - exit parameter lock mode
        exitParameterLockMode();
        return;
    }
    
    // Get parameter type and adjustment
    ParameterLockPool::ParameterType paramType = controlGrid_.getParameterType(button, mapping);
    int8_t adjustment = controlGrid_.getParameterAdjustment(button, mapping);
    
    if (paramType != ParameterLockPool::ParameterType::NONE && adjustment != 0) {
        adjustParameter(paramType, adjustment);
    }
}

bool StepSequencer::buttonToTrackStep(uint8_t button, uint8_t& track, uint8_t& step) const {
    if (button >= 32) return false;
    
    track = button / 8;
    step = button % 8;
    
    return track < MAX_TRACKS && step < MAX_STEPS;
}

void StepSequencer::updateVisualFeedback() {
    if (!display_) return;
    
    if (isInParameterLockMode()) {
        // Show parameter lock mode visuals
        const auto& context = stateManager_.getParameterLockContext();
        ControlGrid::ControlMapping mapping = controlGrid_.getMapping(context.heldStep, context.heldTrack);
        
        uint32_t colors[32] = {0};
        controlGrid_.getControlColors(mapping, colors);
        
        // Set held step to bright white
        uint8_t heldButton = context.heldTrack * 8 + context.heldStep;
        colors[heldButton] = 0xFFFFFF;
        
        // Set control buttons to their parameter colors
        for (uint8_t i = 0; i < 32; ++i) {
            display_->setPixel(i, colors[i]);
        }
    } else {
        // Show normal sequencer state
        for (uint8_t track = 0; track < MAX_TRACKS; ++track) {
            for (uint8_t step = 0; step < MAX_STEPS; ++step) {
                uint8_t button = track * 8 + step;
                uint32_t color = 0;
                
                if (patternData_[track][step].active) {
                    if (step == currentStep_ && playing_) {
                        color = 0xFFFFFF; // White for current playing step
                    } else if (patternData_[track][step].hasLock) {
                        color = 0xFF8000; // Orange for steps with locks
                    } else {
                        color = 0x00FF00; // Green for active steps
                    }
                }
                
                display_->setPixel(button, color);
            }
        }
    }
    
    display_->show();
}

void StepSequencer::sendMidiWithParameters(uint8_t track, const CalculatedParameters& params) {
    if (midiOutput_ && params.isValid()) {
        midiOutput_->sendNoteOn(params.channel, params.note, params.velocity);
        
        // Schedule note off based on gate length
        // In full implementation, would use timer for note off
    }
}

void StepSequencer::checkForHoldEvents() {
    // Check all buttons for new hold events
    for (uint8_t button = 0; button < 32; ++button) {
        const auto& buttonState = buttonTracker_.getButtonState(button);
        
        // If button is held but hold hasn't been processed yet
        if (buttonState.isHeld && !buttonState.holdProcessed) {
            uint8_t track, step;
            if (buttonToTrackStep(button, track, step)) {
                // DEBUG: Add some serial output
                #ifdef ARDUINO
                Serial.print("DEBUG: Hold detected for button ");
                Serial.print(button);
                Serial.print(" (track ");
                Serial.print(track);
                Serial.print(", step ");
                Serial.print(step);
                Serial.println(")");
                #endif
                
                // Enter parameter lock mode
                if (enterParameterLockMode(track, step)) {
                    #ifdef ARDUINO
                    Serial.println("DEBUG: Successfully entered parameter lock mode");
                    #endif
                    // Mark hold as processed to avoid multiple entries
                    // Note: We need to mark this in the button tracker
                    // For now, rely on state manager to prevent duplicate entries
                }
            }
        }
    }
}