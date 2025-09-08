#include "StepSequencer.h"
#include <cstring>
#include <algorithm>
#include <cstdarg>
#include <cstdio>

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
    , debugOutput_(nullptr)
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
    , debugOutput_(deps.debugOutput)
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
    uint32_t currentTime = clock_ ? clock_->getCurrentTime() : 0;
    
    // Update parameter lock system state
    if (currentTime != lastUpdateTime_) {
        stateManager_.update(currentTime);
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
    lastStepTime_ = clock_ ? clock_->getCurrentTime() : 0;
    
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
    lastStepTime_ = clock_ ? clock_->getCurrentTime() : 0;
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

// handleButton method removed - button handling moved to input layer
// Use processMessage() with semantic ControlMessage objects instead

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
    if (!isInParameterLockMode()) {
        debugLog("PARAM_LOCK: adjustParameter failed - not in parameter lock mode");
        return false;
    }
    
    const auto& context = stateManager_.getParameterLockContext();
    uint8_t track = context.heldTrack;
    uint8_t step = context.heldStep;
    
    debugLog("PARAM_LOCK: adjustParameter called - track=%d, step=%d, paramType=%d, delta=%d", 
             track, step, (int)paramType, delta);
    
    if (track >= MAX_TRACKS || step >= MAX_STEPS) {
        debugLog("PARAM_LOCK: adjustParameter failed - invalid track/step");
        return false;
    }
    
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
        case ParameterLockPool::ParameterType::NOTE: {
            int8_t oldValue = lock.noteOffset;
            lock.noteOffset += delta;
            lock.noteOffset = std::max(-64, std::min(63, static_cast<int>(lock.noteOffset)));
            debugLog("PARAM_LOCK: NOTE adjustment - old=%d, new=%d (delta=%d)", oldValue, lock.noteOffset, delta);
            break;
        }
        case ParameterLockPool::ParameterType::VELOCITY: {
            uint8_t oldValue = lock.velocity;
            lock.velocity = std::max(0, std::min(127, static_cast<int>(lock.velocity) + delta));
            debugLog("PARAM_LOCK: VELOCITY adjustment - old=%d, new=%d (delta=%d)", oldValue, lock.velocity, delta);
            break;
        }
        case ParameterLockPool::ParameterType::LENGTH: {
            uint8_t oldValue = lock.length;
            lock.length = std::max(1, std::min(255, static_cast<int>(lock.length) + delta));
            debugLog("PARAM_LOCK: LENGTH adjustment - old=%d, new=%d (delta=%d)", oldValue, lock.length, delta);
            break;
        }
        default:
            debugLog("PARAM_LOCK: Unknown parameter type %d", (int)paramType);
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

// setHandPreference method removed - hand preference should be handled by input layer

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

// updateButtonStates method removed - button state handling moved to input layer

// handleNormalModeButton method removed - use TOGGLE_STEP ControlMessage instead

// handleParameterLockInput method removed - use ENTER_PARAM_LOCK/EXIT_PARAM_LOCK/ADJUST_PARAMETER ControlMessages instead

bool StepSequencer::buttonToTrackStep(uint8_t button, uint8_t& track, uint8_t& step) const {
    if (button >= 32) return false;
    
    track = button / 8;
    step = button % 8;
    
    return track < MAX_TRACKS && step < MAX_STEPS;
}

void StepSequencer::updateVisualFeedback() {
    if (!display_) return;
    
    if (isInParameterLockMode()) {
        // Show parameter lock mode visuals (simplified - control colors handled by input layer)
        const auto& context = stateManager_.getParameterLockContext();
        
        // Set held step to bright white to indicate parameter lock mode
        uint8_t heldButton = context.heldTrack * 8 + context.heldStep;
        
        // Show normal pattern but highlight the held step
        // Track colors for inactive steps (dim)
        uint32_t trackColors[4] = {0x200000, 0x002000, 0x000020, 0x202000}; // Dim red, green, blue, yellow
        
        for (uint8_t track = 0; track < MAX_TRACKS; ++track) {
            for (uint8_t step = 0; step < MAX_STEPS; ++step) {
                uint8_t button = track * 8 + step;
                uint32_t color = trackColors[track]; // Default to dim track color
                
                if (button == heldButton) {
                    color = 0xFFFFFF; // White for held step in parameter lock mode
                } else if (patternData_[track][step].active) {
                    if (patternData_[track][step].hasLock) {
                        color = 0xFF8000; // Orange for steps with locks
                    } else {
                        // Bright track colors for active steps
                        uint32_t brightColors[4] = {0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00}; // Red, green, blue, yellow
                        color = brightColors[track];
                    }
                }
                
                display_->setPixel(button, color);
            }
        }
    } else {
        // Show normal sequencer state
        // Track colors for inactive steps (dim)
        uint32_t trackColors[4] = {0x200000, 0x002000, 0x000020, 0x202000}; // Dim red, green, blue, yellow
        
        for (uint8_t track = 0; track < MAX_TRACKS; ++track) {
            for (uint8_t step = 0; step < MAX_STEPS; ++step) {
                uint8_t button = track * 8 + step;
                uint32_t color = trackColors[track]; // Default to dim track color
                
                if (patternData_[track][step].active) {
                    if (step == currentStep_ && playing_) {
                        color = 0xFFFFFF; // White for current playing step
                    } else if (patternData_[track][step].hasLock) {
                        color = 0xFF8000; // Orange for steps with locks
                    } else {
                        // Bright track colors for active steps
                        uint32_t brightColors[4] = {0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00}; // Red, green, blue, yellow
                        color = brightColors[track];
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

// checkForHoldEvents and checkForHoldRelease methods removed - 
// Input layer should send ENTER_PARAM_LOCK/EXIT_PARAM_LOCK ControlMessages based on hold detection

// State restore methods for JSON serialization
void StepSequencer::restorePatternData(const PatternData& pattern) {
    // Use memcpy for direct memory copy of the pattern array
    memcpy(patternData_, pattern, sizeof(PatternData));
}

void StepSequencer::restoreTrackDefaults(const TrackDefaults defaults[MAX_TRACKS]) {
    // Copy track defaults
    for (uint8_t i = 0; i < MAX_TRACKS; ++i) {
        trackDefaults_[i] = defaults[i];
        // Update legacy arrays for compatibility
        trackVolumes_[i] = defaults[i].volume;
        trackMutes_[i] = defaults[i].muted;
        trackMidiNotes_[i] = defaults[i].note;
        trackMidiChannels_[i] = defaults[i].channel;
    }
}

void StepSequencer::restoreLockPool(const ParameterLockPool& pool) {
    // Clear existing locks
    lockPool_.clearAll();
    
    // Reconstruct locks from snapshot data
    // Note: This is a simplified restoration that recreates locks
    // but may not preserve internal pool state perfectly
    for (uint8_t i = 0; i < ParameterLockPool::MAX_LOCKS; ++i) {
        if (pool.isValidIndex(i)) {
            const auto& srcLock = pool.getLock(i);
            if (srcLock.inUse && srcLock.isValid()) {
                uint8_t newIndex = lockPool_.allocate(srcLock.trackIndex, srcLock.stepIndex);
                if (newIndex != ParameterLockPool::INVALID_INDEX) {
                    auto& destLock = lockPool_.getLock(newIndex);
                    destLock.activeLocks = srcLock.activeLocks;
                    destLock.noteOffset = srcLock.noteOffset;
                    destLock.velocity = srcLock.velocity;
                    destLock.length = srcLock.length;
                    // trackIndex and stepIndex are already set by allocate()
                }
            }
        }
    }
}

// restoreButtonTracker method removed - button tracking moved to input layer

void StepSequencer::restoreStateManager(const SequencerStateManager& manager) {
    // Note: State manager restoration may not perfectly restore timing-based
    // state like hold start times. For testing, the mode and parameter lock
    // context is the most important state to restore.
    stateManager_ = manager;
}

bool StepSequencer::processMessage(const ControlMessage::Message& message) {
    debugLog("CONTROL_MSG: Processing message type %d", static_cast<int>(message.type));
    
    switch (message.type) {
        case ControlMessage::Type::TOGGLE_STEP: {
            uint8_t track = static_cast<uint8_t>(message.param1);
            uint8_t step = static_cast<uint8_t>(message.param2);
            if (track < MAX_TRACKS && step < MAX_STEPS) {
                toggleStep(track, step);
                debugLog("CONTROL_MSG: Toggled step track=%d, step=%d", track, step);
                return true;
            }
            break;
        }
        
        case ControlMessage::Type::ENTER_PARAM_LOCK: {
            uint8_t track = static_cast<uint8_t>(message.param1);
            uint8_t step = static_cast<uint8_t>(message.param2);
            if (track < MAX_TRACKS && step < MAX_STEPS) {
                bool success = enterParameterLockMode(track, step);
                debugLog("CONTROL_MSG: Enter param lock track=%d, step=%d, success=%d", track, step, success);
                return success;
            }
            break;
        }
        
        case ControlMessage::Type::EXIT_PARAM_LOCK: {
            bool success = exitParameterLockMode();
            debugLog("CONTROL_MSG: Exit param lock, success=%d", success);
            return success;
        }
        
        case ControlMessage::Type::ADJUST_PARAMETER: {
            auto paramType = static_cast<ParameterLockPool::ParameterType>(message.param1);
            int8_t delta = static_cast<int8_t>(message.param2);
            bool success = adjustParameter(paramType, delta);
            debugLog("CONTROL_MSG: Adjust parameter type=%d, delta=%d, success=%d", static_cast<int>(paramType), delta, success);
            return success;
        }
        
        case ControlMessage::Type::START:
            start();
            debugLog("CONTROL_MSG: Start sequencer");
            return true;
            
        case ControlMessage::Type::STOP:
            stop();
            debugLog("CONTROL_MSG: Stop sequencer");
            return true;
            
        case ControlMessage::Type::RESET:
            reset();
            debugLog("CONTROL_MSG: Reset sequencer");
            return true;
            
        case ControlMessage::Type::SET_TEMPO: {
            uint16_t bpm = static_cast<uint16_t>(message.param1);
            setTempo(bpm);
            debugLog("CONTROL_MSG: Set tempo to %d BPM", bpm);
            return true;
        }
        
        default:
            debugLog("CONTROL_MSG: Unknown message type %d", static_cast<int>(message.type));
            return false;
    }
    
    debugLog("CONTROL_MSG: Invalid parameters for message type %d", static_cast<int>(message.type));
    return false;
}

void StepSequencer::debugLog(const char* format, ...) {
    if (debugOutput_) {
        va_list args;
        va_start(args, format);
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        debugOutput_->log(std::string(buffer));
    }
}