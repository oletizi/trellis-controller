#include "NonRealtimeSequencer.h"
#include "JsonState.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <stdexcept>

/**
 * Simple VirtualClock implementation for virtual time management
 */
class VirtualClock : public IClock {
private:
    uint32_t virtualTime_;

public:
    VirtualClock() : virtualTime_(0) {}
    
    uint32_t getCurrentTime() const override {
        return virtualTime_;
    }
    
    void delay(uint32_t milliseconds) override {
        virtualTime_ += milliseconds;
    }
    
    void reset() override {
        virtualTime_ = 0;
    }
    
    void setTime(uint32_t time) {
        virtualTime_ = time;
    }
    
    void advance(uint32_t milliseconds) {
        virtualTime_ += milliseconds;
    }
};

NonRealtimeSequencer::NonRealtimeSequencer()
    : clock_(std::make_unique<VirtualClock>())
    , nullMidi_(std::make_unique<NullMidiOutput>())
    , nullDisplay_(std::make_unique<NullDisplay>())
    , nullInput_(std::make_unique<NullInput>())
    , logStream_(nullptr)
    , verbose_(false)
    , virtualTime_(0)
    , stateDirectory_("./states")
{
    // Initialize sequencer with null dependencies
    StepSequencer::Dependencies deps{
        .clock = clock_.get(),
        .midiOutput = nullMidi_.get(),
        .midiInput = nullptr,
        .display = nullDisplay_.get(),
        .input = nullInput_.get()
    };
    
    sequencer_ = std::make_unique<StepSequencer>(deps);
}

NonRealtimeSequencer::~NonRealtimeSequencer() = default;

void NonRealtimeSequencer::init(uint16_t bpm, uint8_t steps) {
    log("Initializing NonRealtimeSequencer with BPM=" + std::to_string(bpm) + ", steps=" + std::to_string(steps));
    
    sequencer_->init(bpm, steps);
    clock_->reset();
    virtualTime_ = 0;
    
    ensureStateDirectory();
}

NonRealtimeSequencer::ExecutionResult NonRealtimeSequencer::processMessage(const ControlMessage::Message& message) {
    ExecutionResult result;
    
    // Validate message first
    std::string validationError;
    if (!validateMessage(message, validationError)) {
        result.success = false;
        result.errorMessage = "Invalid message: " + validationError;
        return result;
    }
    
    // Capture state before execution (convert from JsonState to SequencerState format)
    result.stateBefore = convertJsonStateToSequencerState(JsonState::captureState(*sequencer_));
    
    log("Processing: " + message.toString());
    
    try {
        // Advance virtual time to message timestamp
        if (message.timestamp > virtualTime_) {
            static_cast<VirtualClock*>(clock_.get())->setTime(message.timestamp);
            virtualTime_ = message.timestamp;
        }
        
        // Process message based on type
        switch (message.type) {
            case ControlMessage::Type::KEY_PRESS:
            case ControlMessage::Type::KEY_RELEASE:
                result = processKeyMessage(message);
                break;
                
            case ControlMessage::Type::CLOCK_TICK:
                result = processClockTick(message);
                break;
                
            case ControlMessage::Type::TIME_ADVANCE:
                result = processTimeAdvance(message);
                break;
                
            case ControlMessage::Type::START:
            case ControlMessage::Type::STOP:
            case ControlMessage::Type::RESET:
                result = processSequencerControl(message);
                break;
                
            case ControlMessage::Type::SAVE_STATE:
            case ControlMessage::Type::LOAD_STATE:
            case ControlMessage::Type::VERIFY_STATE:
            case ControlMessage::Type::QUERY_STATE:
                result = processStateMessage(message);
                break;
                
            case ControlMessage::Type::SET_TEMPO:
                sequencer_->setTempo(static_cast<uint16_t>(message.param1));
                result.output = "Set tempo to " + std::to_string(message.param1) + " BPM";
                break;
                
            default:
                result.success = false;
                result.errorMessage = "Unknown message type: " + std::to_string(static_cast<int>(message.type));
                break;
        }
        
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = "Exception during processing: " + std::string(e.what());
    }
    
    // Capture state after execution (convert from JsonState to SequencerState format)
    result.stateAfter = convertJsonStateToSequencerState(JsonState::captureState(*sequencer_));
    
    if (verbose_ && !result.output.empty()) {
        log("Result: " + result.output);
    }
    
    return result;
}

NonRealtimeSequencer::BatchResult NonRealtimeSequencer::processBatch(const std::vector<ControlMessage::Message>& messages) {
    BatchResult batchResult;
    
    log("Processing batch of " + std::to_string(messages.size()) + " messages");
    
    for (const auto& message : messages) {
        ExecutionResult result = processMessage(message);
        batchResult.addResult(result);
        
        // Stop on first error if not successful
        if (!result.success) {
            log("Batch processing stopped due to error: " + result.errorMessage);
            break;
        }
    }
    
    // Generate summary
    std::stringstream summary;
    summary << "Processed " << batchResult.successfulMessages << "/" << batchResult.totalMessages << " messages";
    if (!batchResult.allSucceeded) {
        summary << " (ERRORS ENCOUNTERED)";
    }
    batchResult.summary = summary.str();
    
    log("Batch complete: " + batchResult.summary);
    return batchResult;
}

NonRealtimeSequencer::BatchResult NonRealtimeSequencer::executeFromFile(const std::string& filename) {
    BatchResult result;
    
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            ExecutionResult error(false, "Could not open file: " + filename);
            result.addResult(error);
            return result;
        }
        
        std::vector<ControlMessage::Message> messages;
        std::string line;
        int lineNumber = 0;
        (void)lineNumber; // Suppress unused variable warning
        
        while (std::getline(file, line)) {
            lineNumber++;
            
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') {
                continue;
            }
            
            // TODO: Parse line into ControlMessage::Message
            // For now, create a simple parser for basic message types
            // This would need to be implemented based on the expected file format
            
            log("Warning: executeFromFile not fully implemented - parsing logic needed");
            break;
        }
        
        if (!messages.empty()) {
            result = processBatch(messages);
        }
        
    } catch (const std::exception& e) {
        ExecutionResult error(false, "Exception reading file: " + std::string(e.what()));
        result.addResult(error);
    }
    
    return result;
}

SequencerState::Snapshot NonRealtimeSequencer::getCurrentState() const {
    // Convert JsonState::Snapshot to SequencerState::Snapshot
    auto jsonSnapshot = JsonState::captureState(*sequencer_);
    
    // Create compatible SequencerState::Snapshot
    SequencerState::Snapshot snapshot;
    
    // Copy basic sequencer state
    snapshot.bpm = jsonSnapshot.sequencer.bpm;
    snapshot.stepCount = jsonSnapshot.sequencer.stepCount;
    snapshot.currentStep = jsonSnapshot.sequencer.currentStep;
    snapshot.playing = jsonSnapshot.sequencer.playing;
    snapshot.currentTime = jsonSnapshot.sequencer.currentTime;
    snapshot.tickCounter = jsonSnapshot.sequencer.tickCounter;
    
    // Copy pattern data
    for (int track = 0; track < 4; ++track) {
        for (int step = 0; step < 8; ++step) {
            snapshot.pattern[track][step].active = jsonSnapshot.pattern[track][step].active;
            snapshot.pattern[track][step].hasLock = jsonSnapshot.pattern[track][step].hasLock;
            snapshot.pattern[track][step].lockIndex = jsonSnapshot.pattern[track][step].lockIndex;
        }
    }
    
    // Copy parameter locks
    for (int i = 0; i < 64; ++i) {
        const auto& jsonLock = jsonSnapshot.parameterLocks[i];
        auto& stateLock = snapshot.parameterLocks[i];
        
        stateLock.inUse = jsonLock.inUse;
        stateLock.stepIndex = jsonLock.stepIndex;
        stateLock.trackIndex = jsonLock.trackIndex;
        stateLock.activeLocks = jsonLock.activeLocks;
        stateLock.noteOffset = jsonLock.noteOffset;
        stateLock.velocity = jsonLock.velocity;
        stateLock.length = jsonLock.length;
    }
    
    // Copy button states
    for (int i = 0; i < 32; ++i) {
        const auto& jsonButton = jsonSnapshot.buttons[i];
        auto& stateButton = snapshot.buttons[i];
        
        stateButton.pressed = jsonButton.pressed;
        stateButton.wasPressed = jsonButton.wasPressed;
        stateButton.wasReleased = jsonButton.wasReleased;
        stateButton.pressTime = jsonButton.pressTime;
        stateButton.releaseTime = jsonButton.releaseTime;
        stateButton.isHeld = jsonButton.isHeld;
        stateButton.holdProcessed = jsonButton.holdProcessed;
        stateButton.holdDuration = jsonButton.holdDuration;
    }
    
    // Copy parameter lock mode state
    snapshot.inParameterLockMode = jsonSnapshot.parameterLockMode.active;
    snapshot.heldTrack = jsonSnapshot.parameterLockMode.heldTrack;
    snapshot.heldStep = jsonSnapshot.parameterLockMode.heldStep;
    
    // Copy track settings
    for (int i = 0; i < 4; ++i) {
        const auto& jsonTrack = jsonSnapshot.tracks[i];
        snapshot.trackVolumes[i] = jsonTrack.volume;
        snapshot.trackMutes[i] = jsonTrack.muted;
        snapshot.trackNotes[i] = jsonTrack.note;
        snapshot.trackChannels[i] = jsonTrack.channel;
    }
    
    return snapshot;
}

bool NonRealtimeSequencer::setState(const SequencerState::Snapshot& snapshot) {
    // Convert SequencerState::Snapshot to JsonState::Snapshot
    JsonState::Snapshot jsonSnapshot;
    
    // Copy basic sequencer state
    jsonSnapshot.sequencer.bpm = snapshot.bpm;
    jsonSnapshot.sequencer.stepCount = snapshot.stepCount;
    jsonSnapshot.sequencer.currentStep = snapshot.currentStep;
    jsonSnapshot.sequencer.playing = snapshot.playing;
    jsonSnapshot.sequencer.currentTime = snapshot.currentTime;
    jsonSnapshot.sequencer.tickCounter = snapshot.tickCounter;
    
    // Copy pattern data
    for (int track = 0; track < 4; ++track) {
        for (int step = 0; step < 8; ++step) {
            jsonSnapshot.pattern[track][step].active = snapshot.pattern[track][step].active;
            jsonSnapshot.pattern[track][step].hasLock = snapshot.pattern[track][step].hasLock;
            jsonSnapshot.pattern[track][step].lockIndex = snapshot.pattern[track][step].lockIndex;
        }
    }
    
    // Copy parameter locks
    for (int i = 0; i < 64; ++i) {
        const auto& stateLock = snapshot.parameterLocks[i];
        auto& jsonLock = jsonSnapshot.parameterLocks[i];
        
        jsonLock.inUse = stateLock.inUse;
        jsonLock.stepIndex = stateLock.stepIndex;
        jsonLock.trackIndex = stateLock.trackIndex;
        jsonLock.activeLocks = stateLock.activeLocks;
        jsonLock.noteOffset = stateLock.noteOffset;
        jsonLock.velocity = stateLock.velocity;
        jsonLock.length = stateLock.length;
    }
    
    // Copy button states
    for (int i = 0; i < 32; ++i) {
        const auto& stateButton = snapshot.buttons[i];
        auto& jsonButton = jsonSnapshot.buttons[i];
        
        jsonButton.pressed = stateButton.pressed;
        jsonButton.wasPressed = stateButton.wasPressed;
        jsonButton.wasReleased = stateButton.wasReleased;
        jsonButton.pressTime = stateButton.pressTime;
        jsonButton.releaseTime = stateButton.releaseTime;
        jsonButton.isHeld = stateButton.isHeld;
        jsonButton.holdProcessed = stateButton.holdProcessed;
        jsonButton.holdDuration = stateButton.holdDuration;
    }
    
    // Copy parameter lock mode state
    jsonSnapshot.parameterLockMode.active = snapshot.inParameterLockMode;
    jsonSnapshot.parameterLockMode.heldTrack = snapshot.heldTrack;
    jsonSnapshot.parameterLockMode.heldStep = snapshot.heldStep;
    
    // Copy track settings
    for (int i = 0; i < 4; ++i) {
        auto& jsonTrack = jsonSnapshot.tracks[i];
        jsonTrack.volume = snapshot.trackVolumes[i];
        jsonTrack.muted = snapshot.trackMutes[i];
        jsonTrack.note = snapshot.trackNotes[i];
        jsonTrack.channel = snapshot.trackChannels[i];
    }
    
    // Restore state using JsonState system
    return JsonState::restoreState(*sequencer_, jsonSnapshot);
}

bool NonRealtimeSequencer::saveState(const std::string& filename) const {
    try {
        ensureStateDirectory();
        
        std::string fullPath = stateDirectory_ + "/" + filename;
        auto snapshot = JsonState::captureState(*sequencer_);
        
        bool success = snapshot.saveToFile(fullPath);
        if (success) {
            log("State saved to: " + fullPath);
        } else {
            log("Failed to save state to: " + fullPath);
        }
        
        return success;
    } catch (const std::exception& e) {
        log("Exception saving state: " + std::string(e.what()));
        return false;
    }
}

bool NonRealtimeSequencer::loadState(const std::string& filename) {
    try {
        std::string fullPath = stateDirectory_ + "/" + filename;
        
        JsonState::Snapshot snapshot;
        bool success = snapshot.loadFromFile(fullPath);
        
        if (success) {
            success = JsonState::restoreState(*sequencer_, snapshot);
            if (success) {
                log("State loaded from: " + fullPath);
            } else {
                log("Failed to restore state from: " + fullPath);
            }
        } else {
            log("Failed to load state file: " + fullPath);
        }
        
        return success;
    } catch (const std::exception& e) {
        log("Exception loading state: " + std::string(e.what()));
        return false;
    }
}

void NonRealtimeSequencer::reset() {
    log("Resetting sequencer");
    
    sequencer_->reset();
    clock_->reset();
    virtualTime_ = 0;
}

void NonRealtimeSequencer::log(const std::string& message) const {
    if (logStream_ && verbose_) {
        *logStream_ << "[NonRealtimeSequencer] " << message << std::endl;
    }
}

NonRealtimeSequencer::ExecutionResult NonRealtimeSequencer::processKeyMessage(const ControlMessage::Message& message) {
    ExecutionResult result;
    
    uint8_t button = static_cast<uint8_t>(message.param1);
    bool pressed = (message.type == ControlMessage::Type::KEY_PRESS);
    
    // Validate button index (assuming 4x8 grid = 32 buttons)
    if (button >= 32) {
        result.success = false;
        result.errorMessage = "Button index out of range: " + std::to_string(button);
        return result;
    }
    
    try {
        sequencer_->handleButton(button, pressed, virtualTime_);
        result.output = (pressed ? "Pressed" : "Released") + std::string(" button ") + std::to_string(button);
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = "Error handling button: " + std::string(e.what());
    }
    
    return result;
}

NonRealtimeSequencer::ExecutionResult NonRealtimeSequencer::processClockTick(const ControlMessage::Message& message) {
    ExecutionResult result;
    
    uint32_t ticks = message.param1;
    if (ticks == 0) {
        ticks = 1; // Default to 1 tick
    }
    
    try {
        for (uint32_t i = 0; i < ticks; ++i) {
            sequencer_->tick();
        }
        result.output = "Advanced " + std::to_string(ticks) + " clock tick" + (ticks > 1 ? "s" : "");
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = "Error during clock tick: " + std::string(e.what());
    }
    
    return result;
}

NonRealtimeSequencer::ExecutionResult NonRealtimeSequencer::processTimeAdvance(const ControlMessage::Message& message) {
    ExecutionResult result;
    
    uint32_t milliseconds = message.param1;
    
    try {
        static_cast<VirtualClock*>(clock_.get())->advance(milliseconds);
        virtualTime_ += milliseconds;
        result.output = "Advanced time by " + std::to_string(milliseconds) + "ms (now at " + std::to_string(virtualTime_) + "ms)";
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = "Error advancing time: " + std::string(e.what());
    }
    
    return result;
}

NonRealtimeSequencer::ExecutionResult NonRealtimeSequencer::processSequencerControl(const ControlMessage::Message& message) {
    ExecutionResult result;
    
    try {
        switch (message.type) {
            case ControlMessage::Type::START:
                sequencer_->start();
                result.output = "Started sequencer";
                break;
                
            case ControlMessage::Type::STOP:
                sequencer_->stop();
                result.output = "Stopped sequencer";
                break;
                
            case ControlMessage::Type::RESET:
                sequencer_->reset();
                result.output = "Reset sequencer";
                break;
                
            default:
                result.success = false;
                result.errorMessage = "Unknown sequencer control type";
                break;
        }
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = "Error in sequencer control: " + std::string(e.what());
    }
    
    return result;
}

NonRealtimeSequencer::ExecutionResult NonRealtimeSequencer::processStateMessage(const ControlMessage::Message& message) {
    ExecutionResult result;
    
    try {
        switch (message.type) {
            case ControlMessage::Type::SAVE_STATE:
                if (message.stringParam.empty()) {
                    result.success = false;
                    result.errorMessage = "No filename provided for SAVE_STATE";
                } else {
                    result.success = saveState(message.stringParam);
                    result.output = result.success ? 
                        ("State saved to " + message.stringParam) :
                        ("Failed to save state to " + message.stringParam);
                }
                break;
                
            case ControlMessage::Type::LOAD_STATE:
                if (message.stringParam.empty()) {
                    result.success = false;
                    result.errorMessage = "No filename provided for LOAD_STATE";
                } else {
                    result.success = loadState(message.stringParam);
                    result.output = result.success ?
                        ("State loaded from " + message.stringParam) :
                        ("Failed to load state from " + message.stringParam);
                }
                break;
                
            case ControlMessage::Type::VERIFY_STATE:
                {
                    auto currentSnapshot = JsonState::captureState(*sequencer_);
                    JsonState::Snapshot expectedSnapshot;
                    
                    if (!expectedSnapshot.fromJson(message.stringParam)) {
                        result.success = false;
                        result.errorMessage = "Invalid JSON in expected state";
                    } else {
                        bool matches = currentSnapshot.equals(expectedSnapshot);
                        result.success = matches;
                        result.output = matches ? 
                            "State verification PASSED" :
                            ("State verification FAILED: " + currentSnapshot.getDiff(expectedSnapshot));
                    }
                }
                break;
                
            case ControlMessage::Type::QUERY_STATE:
                {
                    auto snapshot = JsonState::captureState(*sequencer_);
                    result.output = snapshot.getSummary();
                }
                break;
                
            default:
                result.success = false;
                result.errorMessage = "Unknown state message type";
                break;
        }
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = "Error in state message: " + std::string(e.what());
    }
    
    return result;
}

bool NonRealtimeSequencer::validateMessage(const ControlMessage::Message& message, std::string& error) {
    // Basic validation
    switch (message.type) {
        case ControlMessage::Type::KEY_PRESS:
        case ControlMessage::Type::KEY_RELEASE:
            if (message.param1 >= 32) {
                error = "Button index " + std::to_string(message.param1) + " out of range (0-31)";
                return false;
            }
            break;
            
        case ControlMessage::Type::CLOCK_TICK:
            // param1 = tick count, allow 0 (defaults to 1)
            break;
            
        case ControlMessage::Type::TIME_ADVANCE:
            // param1 = milliseconds, any value valid
            break;
            
        case ControlMessage::Type::SET_TEMPO:
            if (message.param1 < 60 || message.param1 > 200) {
                error = "BPM " + std::to_string(message.param1) + " out of range (60-200)";
                return false;
            }
            break;
            
        case ControlMessage::Type::SAVE_STATE:
        case ControlMessage::Type::LOAD_STATE:
            if (message.stringParam.empty()) {
                error = "Filename required but not provided";
                return false;
            }
            break;
            
        case ControlMessage::Type::VERIFY_STATE:
            if (message.stringParam.empty()) {
                error = "Expected state JSON required but not provided";
                return false;
            }
            break;
            
        case ControlMessage::Type::START:
        case ControlMessage::Type::STOP:
        case ControlMessage::Type::RESET:
        case ControlMessage::Type::QUERY_STATE:
            // No additional validation needed
            break;
            
        default:
            error = "Unknown message type: " + std::to_string(static_cast<int>(message.type));
            return false;
    }
    
    return true;
}

void NonRealtimeSequencer::ensureStateDirectory() const {
    try {
        std::filesystem::create_directories(stateDirectory_);
    } catch (const std::exception& e) {
        // Log error but don't fail - let file operations handle missing directory
        if (logStream_) {
            *logStream_ << "[NonRealtimeSequencer] Warning: Could not create state directory: " 
                       << e.what() << std::endl;
        }
    }
}

// Helper function to convert JsonState::Snapshot to SequencerState::Snapshot
SequencerState::Snapshot NonRealtimeSequencer::convertJsonStateToSequencerState(const JsonState::Snapshot& jsonSnapshot) const {
    SequencerState::Snapshot snapshot;
    
    // Copy basic sequencer state
    snapshot.bpm = jsonSnapshot.sequencer.bpm;
    snapshot.stepCount = jsonSnapshot.sequencer.stepCount;
    snapshot.currentStep = jsonSnapshot.sequencer.currentStep;
    snapshot.playing = jsonSnapshot.sequencer.playing;
    snapshot.currentTime = jsonSnapshot.sequencer.currentTime;
    snapshot.tickCounter = jsonSnapshot.sequencer.tickCounter;
    
    // Copy pattern data
    for (int track = 0; track < 4; ++track) {
        for (int step = 0; step < 8; ++step) {
            snapshot.pattern[track][step].active = jsonSnapshot.pattern[track][step].active;
            snapshot.pattern[track][step].hasLock = jsonSnapshot.pattern[track][step].hasLock;
            snapshot.pattern[track][step].lockIndex = jsonSnapshot.pattern[track][step].lockIndex;
        }
    }
    
    // Copy parameter locks
    for (int i = 0; i < 64; ++i) {
        const auto& jsonLock = jsonSnapshot.parameterLocks[i];
        auto& stateLock = snapshot.parameterLocks[i];
        
        stateLock.inUse = jsonLock.inUse;
        stateLock.stepIndex = jsonLock.stepIndex;
        stateLock.trackIndex = jsonLock.trackIndex;
        stateLock.activeLocks = jsonLock.activeLocks;
        stateLock.noteOffset = jsonLock.noteOffset;
        stateLock.velocity = jsonLock.velocity;
        stateLock.length = jsonLock.length;
    }
    
    // Copy button states
    for (int i = 0; i < 32; ++i) {
        const auto& jsonButton = jsonSnapshot.buttons[i];
        auto& stateButton = snapshot.buttons[i];
        
        stateButton.pressed = jsonButton.pressed;
        stateButton.wasPressed = jsonButton.wasPressed;
        stateButton.wasReleased = jsonButton.wasReleased;
        stateButton.pressTime = jsonButton.pressTime;
        stateButton.releaseTime = jsonButton.releaseTime;
        stateButton.isHeld = jsonButton.isHeld;
        stateButton.holdProcessed = jsonButton.holdProcessed;
        stateButton.holdDuration = jsonButton.holdDuration;
    }
    
    // Copy parameter lock mode state
    snapshot.inParameterLockMode = jsonSnapshot.parameterLockMode.active;
    snapshot.heldTrack = jsonSnapshot.parameterLockMode.heldTrack;
    snapshot.heldStep = jsonSnapshot.parameterLockMode.heldStep;
    
    // Copy track settings
    for (int i = 0; i < 4; ++i) {
        const auto& jsonTrack = jsonSnapshot.tracks[i];
        snapshot.trackVolumes[i] = jsonTrack.volume;
        snapshot.trackMutes[i] = jsonTrack.muted;
        snapshot.trackNotes[i] = jsonTrack.note;
        snapshot.trackChannels[i] = jsonTrack.channel;
    }
    
    return snapshot;
}