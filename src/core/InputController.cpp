#include "InputController.h"
#include <stdexcept>
#include <algorithm>
#include <string>

InputController::InputController(Dependencies deps, const InputSystemConfiguration& config)
    : dependencies_(std::move(deps)), config_(config), currentInputState_(0, false, 0, 0) {
    if (!dependencies_.isValid()) {
        std::string error = "InputController: Invalid dependencies - missing inputLayer or clock";
        if (!dependencies_.gestureDetector && (!dependencies_.inputStateEncoder || !dependencies_.inputStateProcessor)) {
            error += ", and either gestureDetector OR (inputStateEncoder AND inputStateProcessor)";
        }
        throw std::invalid_argument(error);
    }
    
    // Log which system is being used
    if (dependencies_.gestureDetector) {
        debugLog("Using legacy GestureDetector system");
    }
    if (dependencies_.inputStateEncoder && dependencies_.inputStateProcessor) {
        debugLog("Using new InputStateEncoder+InputStateProcessor system");
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
        
        // Reset gesture detector (if present for backward compatibility)
        if (dependencies_.gestureDetector) {
            dependencies_.gestureDetector->reset();
        }
        
        // Initialize input state to clean state
        currentInputState_ = InputState(0, false, 0, 0);
        
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
    
    // Reset gesture detector (if present for backward compatibility)
    if (dependencies_.gestureDetector) {
        dependencies_.gestureDetector->reset();
    }
    
    // Reset input state to clean state
    currentInputState_ = InputState(0, false, 0, 0);
    
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
    if (!initialized_ || !buttonStates) {
        return 0;
    }
    
    // Try new bitwise state system first
    if (dependencies_.inputStateEncoder && dependencies_.inputStateProcessor) {
        // Extract button states from current InputState
        uint8_t buttonCount = std::min(static_cast<uint8_t>(32), maxButtons);
        
        for (uint8_t i = 0; i < buttonCount; ++i) {
            buttonStates[i] = currentInputState_.isButtonPressed(i);
        }
        
        return buttonCount;
    }
    
    // Fall back to legacy gesture detector system
    if (dependencies_.gestureDetector) {
        return dependencies_.gestureDetector->getCurrentButtonStates(buttonStates, maxButtons);
    }
    
    return 0;
}

bool InputController::isInParameterLockMode() const {
    if (!initialized_) {
        return false;
    }
    
    // Try new bitwise state system first
    if (dependencies_.inputStateEncoder && dependencies_.inputStateProcessor) {
        return currentInputState_.isParameterLockActive();
    }
    
    // Fall back to legacy gesture detector system
    if (dependencies_.gestureDetector) {
        return dependencies_.gestureDetector->isInParameterLockMode();
    }
    
    return false;
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
        
        // Update gesture detector configuration (if present for backward compatibility)
        if (dependencies_.gestureDetector) {
            dependencies_.gestureDetector->setConfiguration(config);
        }
        
        // Update new input state components configuration (if present)
        if (dependencies_.inputStateEncoder) {
            dependencies_.inputStateEncoder->setHoldThreshold(config.timing.holdThresholdMs);
        }
        
        if (dependencies_.inputStateProcessor) {
            dependencies_.inputStateProcessor->setHoldThreshold(config.timing.holdThresholdMs);
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
    
    // Reset gesture detector state (if present for backward compatibility)
    if (dependencies_.gestureDetector) {
        dependencies_.gestureDetector->reset();
    }
    
    // Reset input state to clean state
    currentInputState_ = InputState(0, false, 0, 0);
    
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
    if (!dependencies_.inputLayer) {
        debugLog("Missing inputLayer dependency for event processing");
        return 0;
    }
    
    uint16_t messagesGenerated = 0;
    InputEvent inputEvent;
    std::vector<ControlMessage::Message> controlMessages;
    
    // Process all available input events
    while (dependencies_.inputLayer->getNextEvent(inputEvent)) {
        status_.eventsProcessed++;
        controlMessages.clear();
        
        // Handle system events directly (not processed by either system)
        if (inputEvent.type == InputEvent::Type::SYSTEM_EVENT) {
            // Convert system events to control messages
            if (inputEvent.deviceId == 255 && inputEvent.value == 1) {
                // Quit event from simulation
                ControlMessage::Message quitMsg;
                quitMsg.type = ControlMessage::Type::SYSTEM_EVENT;
                quitMsg.timestamp = inputEvent.timestamp;
                quitMsg.param1 = 255;  // Quit signal
                quitMsg.param2 = 255;  // Quit signal
                quitMsg.stringParam = "";
                controlMessages.push_back(quitMsg);
                debugLog("System quit event processed");
            }
        } else if (dependencies_.inputStateEncoder && dependencies_.inputStateProcessor) {
            // Process through new bitwise state system
            InputState previousState = currentInputState_;
            
            // Use InputStateEncoder to convert InputEvent to InputState
            InputState newState = dependencies_.inputStateEncoder->processInputEvent(inputEvent, currentInputState_);
            
            // Use InputStateProcessor to generate ControlMessages from state transition
            auto messages = dependencies_.inputStateProcessor->translateState(newState, currentInputState_, inputEvent.timestamp);
            
            // Add messages to the control messages vector
            controlMessages.insert(controlMessages.end(), messages.begin(), messages.end());
            
            // Update current state
            currentInputState_ = newState;
            
            // Debug logging for state changes
            if (newState.raw != previousState.raw) {
                debugLog("State transition: 0x" + std::to_string(previousState.raw) + " -> 0x" + std::to_string(newState.raw));
            }
        } else if (dependencies_.gestureDetector) {
            // Process through legacy gesture detection system
            dependencies_.gestureDetector->processInputEvent(inputEvent, controlMessages);
        } else {
            debugLog("No valid processing system available");
        }
        
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
    if (!dependencies_.clock) {
        return 0;
    }
    
    uint32_t currentTime = dependencies_.clock->getCurrentTime();
    uint16_t messagesGenerated = 0;
    std::vector<ControlMessage::Message> controlMessages;
    
    if (dependencies_.inputStateEncoder && dependencies_.inputStateProcessor) {
        // Use new bitwise state system for timing updates
        InputState previousState = currentInputState_;
        
        // Use InputStateEncoder for timing-based state updates
        InputState newState = dependencies_.inputStateEncoder->updateTiming(currentTime, currentInputState_);
        
        if (newState.raw != currentInputState_.raw) {
            // Process any timing-based state changes
            auto messages = dependencies_.inputStateProcessor->translateState(newState, currentInputState_, currentTime);
            
            // Add messages to control messages vector
            controlMessages.insert(controlMessages.end(), messages.begin(), messages.end());
            
            // Update current state
            currentInputState_ = newState;
            
            // Debug logging for timing-based state changes
            debugLog("Timing state transition: 0x" + std::to_string(previousState.raw) + " -> 0x" + std::to_string(newState.raw));
        }
    } else if (dependencies_.gestureDetector) {
        // Use legacy gesture detector for timing updates
        dependencies_.gestureDetector->updateTiming(currentTime, controlMessages);
    }
    
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
        dependencies_.debugOutput->log("InputController: " + message);
    }
}