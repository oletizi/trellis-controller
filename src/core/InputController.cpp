#include "InputController.h"
#include <stdexcept>
#include <algorithm>

InputController::InputController(Dependencies deps, const InputSystemConfiguration& config)
    : dependencies_(std::move(deps)), config_(config) {
    if (!dependencies_.isValid()) {
        throw std::invalid_argument("InputController: Invalid dependencies provided");
    }
    
    status_.reset();
}

InputController::~InputController() {
    shutdown();
}

bool InputController::initialize() {
    if (initialized_) {
        debugLog("Already initialized");
        return true;
    }
    
    try {
        // Create input layer dependencies
        InputLayerDependencies inputDeps;
        inputDeps.clock = dependencies_.clock;
        inputDeps.debugOutput = dependencies_.debugOutput;
        
        // Initialize input layer
        if (!dependencies_.inputLayer->initialize(config_, inputDeps)) {
            debugLog("Failed to initialize input layer");
            return false;
        }
        
        // Reset gesture detector
        dependencies_.gestureDetector->reset();
        
        // Clear message queue
        clearMessages();
        status_.reset();
        
        initialized_ = true;
        debugLog("InputController initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        debugLog("Initialization failed: " + std::string(e.what()));
        return false;
    }
}

void InputController::shutdown() {
    if (!initialized_) return;
    
    // Shutdown input layer
    if (dependencies_.inputLayer) {
        dependencies_.inputLayer->shutdown();
    }
    
    // Reset gesture detector
    if (dependencies_.gestureDetector) {
        dependencies_.gestureDetector->reset();
    }
    
    // Clear queues
    clearMessages();
    
    initialized_ = false;
    debugLog("InputController shutdown complete");
}

bool InputController::poll() {
    if (!initialized_) {
        debugLog("Poll called on uninitialized InputController");
        return false;
    }
    
    uint32_t pollStartTime = dependencies_.clock->getCurrentTime();
    status_.pollCount++;
    status_.lastPollTime = pollStartTime;
    
    bool success = true;
    
    try {
        // Poll input layer for new events
        if (!dependencies_.inputLayer->poll()) {
            status_.inputLayerError = true;
            success = false;
            debugLog("Input layer poll failed");
        } else {
            status_.inputLayerError = false;
        }
        
        // Process any new input events
        uint16_t messagesFromEvents = processInputEvents();
        
        // Update gesture timing (for hold detection, timeouts)
        uint16_t messagesFromTiming = updateGestureTiming();
        
        status_.messagesGenerated += messagesFromEvents + messagesFromTiming;
        
        updateStatistics();
        
    } catch (const std::exception& e) {
        debugLog("Poll error: " + std::string(e.what()));
        success = false;
    }
    
    return success;
}

bool InputController::getNextMessage(ControlMessage::Message& message) {
    if (messageQueue_.empty()) {
        return false;
    }
    
    message = messageQueue_.front();
    messageQueue_.pop();
    
    updateStatistics();
    return true;
}

bool InputController::hasMessages() const {
    return !messageQueue_.empty();
}

uint8_t InputController::getCurrentButtonStates(bool* buttonStates, uint8_t maxButtons) const {
    if (!initialized_ || !dependencies_.gestureDetector) {
        return 0;
    }
    
    return dependencies_.gestureDetector->getCurrentButtonStates(buttonStates, maxButtons);
}

bool InputController::isInParameterLockMode() const {
    if (!initialized_ || !dependencies_.gestureDetector) {
        return false;
    }
    
    return dependencies_.gestureDetector->isInParameterLockMode();
}

InputController::Status InputController::getStatus() const {
    updateStatistics();
    return status_;
}

bool InputController::setConfiguration(const InputSystemConfiguration& config) {
    try {
        // Update input layer configuration
        if (dependencies_.inputLayer && !dependencies_.inputLayer->setConfiguration(config)) {
            debugLog("Failed to update input layer configuration");
            return false;
        }
        
        // Update gesture detector configuration
        if (dependencies_.gestureDetector) {
            dependencies_.gestureDetector->setConfiguration(config);
        }
        
        config_ = config;
        debugLog("Configuration updated successfully");
        return true;
        
    } catch (const std::exception& e) {
        debugLog("Configuration update failed: " + std::string(e.what()));
        return false;
    }
}

InputSystemConfiguration InputController::getConfiguration() const {
    return config_;
}

uint16_t InputController::clearMessages() {
    uint16_t clearedCount = static_cast<uint16_t>(messageQueue_.size());
    
    while (!messageQueue_.empty()) {
        messageQueue_.pop();
    }
    
    if (clearedCount > 0) {
        debugLog("Cleared " + std::to_string(clearedCount) + " messages");
    }
    
    return clearedCount;
}

void InputController::reset() {
    if (!initialized_) return;
    
    // Reset gesture detector state
    if (dependencies_.gestureDetector) {
        dependencies_.gestureDetector->reset();
    }
    
    // Clear input layer events
    if (dependencies_.inputLayer) {
        dependencies_.inputLayer->clearEvents();
    }
    
    // Clear message queue
    clearMessages();
    
    // Reset statistics
    status_.reset();
    
    debugLog("InputController reset complete");
}

uint16_t InputController::processInputEvents() {
    if (!dependencies_.inputLayer || !dependencies_.gestureDetector) {
        return 0;
    }
    
    uint16_t messagesGenerated = 0;
    InputEvent inputEvent;
    std::vector<ControlMessage::Message> controlMessages;
    
    // Process all available input events
    while (dependencies_.inputLayer->getNextEvent(inputEvent)) {
        status_.eventsProcessed++;
        controlMessages.clear();
        
        // Process event through gesture detection
        uint8_t eventMessages = dependencies_.gestureDetector->processInputEvent(inputEvent, controlMessages);
        
        // Queue resulting control messages
        for (const auto& message : controlMessages) {
            if (enqueueMessage(message)) {
                messagesGenerated++;
            }
        }
    }
    
    return messagesGenerated;
}

uint16_t InputController::updateGestureTiming() {
    if (!dependencies_.gestureDetector || !dependencies_.clock) {
        return 0;
    }
    
    uint32_t currentTime = dependencies_.clock->getCurrentTime();
    std::vector<ControlMessage::Message> controlMessages;
    
    // Update gesture detector timing
    uint8_t timingMessages = dependencies_.gestureDetector->updateTiming(currentTime, controlMessages);
    
    uint16_t messagesGenerated = 0;
    
    // Queue timing-generated control messages
    for (const auto& message : controlMessages) {
        if (enqueueMessage(message)) {
            messagesGenerated++;
        }
    }
    
    return messagesGenerated;
}

bool InputController::enqueueMessage(const ControlMessage::Message& message) {
    // Check for queue overflow
    if (messageQueue_.size() >= config_.performance.messageQueueSize) {
        debugLog("Message queue overflow - dropping message");
        return false;
    }
    
    messageQueue_.push(message);
    return true;
}

void InputController::updateStatistics() const {
    // Update queue depths
    status_.messageQueueDepth = static_cast<uint16_t>(messageQueue_.size());
    
    if (dependencies_.inputLayer) {
        auto inputStatus = dependencies_.inputLayer->getStatus();
        status_.eventQueueDepth = static_cast<uint16_t>(inputStatus.queueUtilization * config_.performance.eventQueueSize / 100);
    }
}

void InputController::debugLog(const std::string& message) const {
    if (dependencies_.debugOutput) {
        dependencies_.debugOutput->print("InputController: " + message);
    }
}