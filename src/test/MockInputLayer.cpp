#include "MockInputLayer.h"
#include <cstring>
#include <algorithm>
#include <stdexcept>

MockInputLayer::MockInputLayer() 
    : hardwareFailure_(false) {
    // Initialize all button states to unpressed
    memset(buttonStates_, false, sizeof(buttonStates_));
    status_.reset();
}

MockInputLayer::~MockInputLayer() {
    shutdown();
}

bool MockInputLayer::initialize(const InputSystemConfiguration& config, 
                               const InputLayerDependencies& deps) {
    if (initialized_) {
        if (debug_) debug_->print("MockInputLayer already initialized");
        return true;
    }
    
    // Validate dependencies
    if (!deps.isValid()) {
        throw std::invalid_argument("MockInputLayer: Invalid dependencies - clock is required");
    }
    
    // Validate configuration
    if (!validateConfiguration(config)) {
        throw std::invalid_argument("MockInputLayer: Invalid configuration parameters");
    }
    
    // Store dependencies and configuration
    clock_ = deps.clock;
    debug_ = deps.debugOutput;
    config_ = config;
    
    // Reset state
    status_.reset();
    clearEvents();
    memset(buttonStates_, false, sizeof(buttonStates_));
    hardwareFailure_ = false;
    
    initialized_ = true;
    
    if (debug_) {
        debug_->print("MockInputLayer initialized with " + 
                     std::to_string(programmedEvents_.size()) + " programmed events");
    }
    
    return true;
}

void MockInputLayer::shutdown() {
    if (!initialized_) return;
    
    // Clear all state
    clearEvents();
    clearProgrammedEvents();
    generatedEvents_.clear();
    memset(buttonStates_, false, sizeof(buttonStates_));
    
    initialized_ = false;
    
    if (debug_) {
        debug_->print("MockInputLayer shutdown complete");
    }
}

bool MockInputLayer::poll() {
    if (!initialized_) return false;
    
    // Simulate hardware failure if requested
    if (hardwareFailure_) {
        status_.hardwareError = true;
        return false;
    }
    
    uint32_t pollStartTime = clock_->getCurrentTime();
    status_.pollCount++;
    status_.lastPollTime = pollStartTime;
    
    // Process any programmed events that should trigger now
    uint32_t eventsGenerated = processProgrammedEvents();
    
    // Update polling interval statistics
    if (status_.pollCount > 1) {
        static uint32_t lastPollTime = pollStartTime;
        uint32_t interval = pollStartTime - lastPollTime;
        status_.averagePollInterval = (status_.averagePollInterval * 3 + interval) / 4;
        lastPollTime = pollStartTime;
    }
    
    updateStatistics();
    
    return eventsGenerated > 0;
}

bool MockInputLayer::getNextEvent(InputEvent& event) {
    if (eventQueue_.empty()) {
        return false;
    }
    
    event = eventQueue_.front();
    eventQueue_.pop();
    status_.eventsProcessed++;
    
    // Track generated events for test verification
    generatedEvents_.push_back(event);
    
    updateStatistics();
    
    return true;
}

bool MockInputLayer::hasEvents() const {
    return !eventQueue_.empty();
}

bool MockInputLayer::setConfiguration(const InputSystemConfiguration& config) {
    if (!validateConfiguration(config)) {
        if (debug_) debug_->print("MockInputLayer: Configuration validation failed");
        return false;
    }
    
    config_ = config;
    
    if (debug_) debug_->print("MockInputLayer configuration updated");
    return true;
}

InputSystemConfiguration MockInputLayer::getConfiguration() const {
    return config_;
}

uint8_t MockInputLayer::getCurrentButtonStates(bool* buttonStates, uint8_t maxButtons) const {
    if (!buttonStates || maxButtons == 0) return 0;
    
    uint8_t copyCount = std::min(static_cast<uint8_t>(32), maxButtons);
    
    for (uint8_t i = 0; i < copyCount; ++i) {
        buttonStates[i] = buttonStates_[i];
    }
    
    return copyCount;
}

IInputLayer::InputLayerStatus MockInputLayer::getStatus() const {
    updateStatistics();
    return status_;
}

uint8_t MockInputLayer::flush() {
    if (!initialized_) return 0;
    
    // Force processing of all pending programmed events
    return processProgrammedEvents();
}

uint8_t MockInputLayer::clearEvents() {
    uint8_t clearedCount = static_cast<uint8_t>(eventQueue_.size());
    
    while (!eventQueue_.empty()) {
        eventQueue_.pop();
    }
    
    if (debug_ && clearedCount > 0) {
        debug_->print("MockInputLayer cleared " + std::to_string(clearedCount) + " events");
    }
    
    return clearedCount;
}

void MockInputLayer::addProgrammedEvent(const InputEvent& event, uint32_t triggerTime) {
    programmedEvents_.emplace_back(event, triggerTime);
    
    // Sort by trigger time to maintain chronological order
    std::sort(programmedEvents_.begin(), programmedEvents_.end(),
              [](const ProgrammedEvent& a, const ProgrammedEvent& b) {
                  return a.triggerTime < b.triggerTime;
              });
    
    if (debug_) {
        debug_->print("Added programmed event for time " + std::to_string(triggerTime));
    }
}

void MockInputLayer::addButtonPress(uint8_t buttonId, uint32_t pressTime) {
    if (buttonId >= 32) {
        if (debug_) debug_->print("Invalid button ID: " + std::to_string(buttonId));
        return;
    }
    
    InputEvent pressEvent = InputEvent::buttonPress(buttonId, pressTime);
    addProgrammedEvent(pressEvent, pressTime);
}

void MockInputLayer::addButtonRelease(uint8_t buttonId, uint32_t releaseTime, uint32_t pressDuration) {
    if (buttonId >= 32) {
        if (debug_) debug_->print("Invalid button ID: " + std::to_string(buttonId));
        return;
    }
    
    InputEvent releaseEvent = InputEvent::buttonRelease(buttonId, releaseTime, pressDuration);
    addProgrammedEvent(releaseEvent, releaseTime);
}

void MockInputLayer::addButtonTap(uint8_t buttonId, uint32_t tapTime, uint32_t tapDuration) {
    addButtonPress(buttonId, tapTime);
    addButtonRelease(buttonId, tapTime + tapDuration, tapDuration);
}

void MockInputLayer::addButtonHold(uint8_t buttonId, uint32_t pressTime, uint32_t holdDuration) {
    addButtonPress(buttonId, pressTime);
    addButtonRelease(buttonId, pressTime + holdDuration, holdDuration);
}

void MockInputLayer::clearProgrammedEvents() {
    programmedEvents_.clear();
    
    if (debug_) {
        debug_->print("Cleared all programmed events");
    }
}

void MockInputLayer::setHardwareFailure(bool shouldFail) {
    hardwareFailure_ = shouldFail;
    status_.hardwareError = shouldFail;
    
    if (debug_) {
        debug_->print("Hardware failure simulation: " + std::string(shouldFail ? "enabled" : "disabled"));
    }
}

uint32_t MockInputLayer::getRemainingProgrammedEvents() const {
    uint32_t count = 0;
    for (const auto& event : programmedEvents_) {
        if (!event.triggered) {
            count++;
        }
    }
    return count;
}

bool MockInputLayer::allEventsTriggered() const {
    return getRemainingProgrammedEvents() == 0;
}

std::vector<InputEvent> MockInputLayer::getGeneratedEvents() const {
    return generatedEvents_;
}

void MockInputLayer::setButtonState(uint8_t buttonId, bool pressed) {
    if (buttonId < 32) {
        buttonStates_[buttonId] = pressed;
        
        if (debug_) {
            debug_->print("Button " + std::to_string(buttonId) + " state set to " + 
                         (pressed ? "pressed" : "released"));
        }
    }
}

void MockInputLayer::setAllButtonStates(const bool* buttonStates) {
    if (buttonStates) {
        memcpy(buttonStates_, buttonStates, sizeof(buttonStates_));
        
        if (debug_) {
            debug_->print("All button states updated");
        }
    }
}

uint32_t MockInputLayer::processProgrammedEvents() {
    if (!clock_) return 0;
    
    uint32_t currentTime = clock_->getCurrentTime();
    uint32_t eventsGenerated = 0;
    
    for (auto& programmedEvent : programmedEvents_) {
        // Skip already triggered events
        if (programmedEvent.triggered) {
            continue;
        }
        
        // Check if it's time to trigger this event
        if (currentTime >= programmedEvent.triggerTime) {
            // Generate the event
            if (eventQueue_.size() >= config_.performance.eventQueueSize) {
                status_.eventsDropped++;
                if (debug_) {
                    debug_->print("Event queue overflow - dropping programmed event");
                }
                continue;
            }
            
            eventQueue_.push(programmedEvent.event);
            updateButtonStateFromEvent(programmedEvent.event);
            programmedEvent.triggered = true;
            eventsGenerated++;
            
            if (debug_) {
                debug_->print("Triggered programmed event at time " + std::to_string(currentTime));
            }
        }
    }
    
    return eventsGenerated;
}

void MockInputLayer::updateButtonStateFromEvent(const InputEvent& event) {
    // Update internal button state tracking based on generated events
    if (event.type == InputEvent::Type::BUTTON_PRESS && event.deviceId < 32) {
        buttonStates_[event.deviceId] = (event.value != 0);
    } else if (event.type == InputEvent::Type::BUTTON_RELEASE && event.deviceId < 32) {
        buttonStates_[event.deviceId] = false;
    }
}

bool MockInputLayer::validateConfiguration(const InputSystemConfiguration& config) const {
    // Verify button count matches expected grid
    if (config.layout.totalButtons != 32) {
        if (debug_) debug_->print("Invalid button count for MockInputLayer");
        return false;
    }
    
    // Verify event queue size is reasonable
    if (config.performance.eventQueueSize == 0 || config.performance.eventQueueSize > 1024) {
        if (debug_) debug_->print("Invalid event queue size");
        return false;
    }
    
    return true;
}

void MockInputLayer::updateStatistics() const {
    // Update queue utilization
    uint16_t queueSize = static_cast<uint16_t>(eventQueue_.size());
    uint16_t maxSize = config_.performance.eventQueueSize;
    if (maxSize > 0) {
        status_.queueUtilization = static_cast<uint8_t>((queueSize * 100) / maxSize);
    }
}