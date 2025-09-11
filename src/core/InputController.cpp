#include "InputController.h"
#include "CursesInputLayer.h"  // For dynamic_cast to set encoder
#include "ShiftBasedGestureDetector.h"  // For SHIFT-based parameter locks
#include <stdexcept>
#include <algorithm>
#include <string>

InputController::InputController(Dependencies deps, const InputSystemConfiguration& config)
    : dependencies_(std::move(deps)), config_(config), currentInputState_(0, false, 0, 0) {
    if (!dependencies_.isValid()) {
        std::string error = "InputController: Invalid dependencies - missing inputLayer, clock";
        if (!dependencies_.inputStateProcessor && !dependencies_.gestureDetector) {
            error += ", and either inputStateProcessor OR gestureDetector";
        }
        throw std::invalid_argument(error);
    }
    
    // Log which system is being used
    if (dependencies_.inputStateProcessor) {
        debugLog("Using modern state-based InputStateProcessor system");
        // Check if we also have ShiftBasedGestureDetector for parameter locks
        if (dependencies_.gestureDetector) {
            auto* shiftDetector = dynamic_cast<ShiftBasedGestureDetector*>(dependencies_.gestureDetector.get());
            if (shiftDetector) {
                debugLog("SHIFT-based parameter lock system integrated");
            }
        }
    } else if (dependencies_.gestureDetector) {
        debugLog("Using legacy event-based GestureDetector system");
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
        
        // Note: InputStateEncoder is now managed internally by input layers
        // State-based processing uses inputLayer->getCurrentInputState() directly
        
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
        // Poll input layer for new state/events
        if (!dependencies_.inputLayer->poll()) {
            status_.inputLayerError = true;
            success = false;
            debugLog("Input layer poll failed");
        } else {
            status_.inputLayerError = false;
        }
        
        uint16_t messagesFromInput = 0;
        uint16_t messagesFromTiming = 0;
        
        // Primary path: Use modern state-based processing with optional SHIFT gesture detection
        if (dependencies_.inputStateProcessor) {
            messagesFromInput = processStateBasedInput();
            // Process SHIFT-based gestures if available (for parameter locks)
            if (dependencies_.gestureDetector) {
                messagesFromInput += processShiftGestures();
            }
        } else {
            // Fallback: Use legacy event-based processing
            messagesFromInput = processInputEventsLegacy();
        }
        
        // Update timing state (works with both modern and legacy systems)
        messagesFromTiming = updateTimingState();
        
        status_.messagesGenerated += messagesFromInput + messagesFromTiming;
        
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
    
    // Primary path: Use modern state-based system
    if (dependencies_.inputStateProcessor) {
        // Get current state directly from input layer
        InputState currentState = dependencies_.inputLayer->getCurrentInputState();
        
        // Extract button states from InputState
        uint8_t buttonCount = std::min(static_cast<uint8_t>(32), maxButtons);
        
        for (uint8_t i = 0; i < buttonCount; ++i) {
            buttonStates[i] = currentState.isButtonPressed(i);
        }
        
        return buttonCount;
    }
    
    // Fallback: Use legacy gesture detector system
    if (dependencies_.gestureDetector) {
        return dependencies_.gestureDetector->getCurrentButtonStates(buttonStates, maxButtons);
    }
    
    return 0;
}

bool InputController::isInParameterLockMode() const {
    if (!initialized_) {
        return false;
    }
    
    // Primary path: Use modern state-based system
    if (dependencies_.inputStateProcessor) {
        // Get current state directly from input layer
        InputState currentState = dependencies_.inputLayer->getCurrentInputState();
        return currentState.isParameterLockActive();
    }
    
    // Fallback: Use legacy gesture detector system
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
        
        // Update modern input state processor configuration (if present)
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

uint16_t InputController::processStateBasedInput() {
    if (!dependencies_.inputLayer || !dependencies_.inputStateProcessor) {
        debugLog("Missing dependencies for state-based input processing");
        return 0;
    }
    
    uint16_t messagesGenerated = 0;
    
    try {
        // Get current authoritative state from input layer
        InputState newState = dependencies_.inputLayer->getCurrentInputState();
        
        // Process state transition if state has changed
        if (newState.raw != currentInputState_.raw) {
            uint32_t currentTime = dependencies_.clock->getCurrentTime();
            
            // Use InputStateProcessor to generate control messages from state transition
            auto messages = dependencies_.inputStateProcessor->translateState(
                newState, currentInputState_, currentTime);
            
            // Queue resulting control messages
            for (const auto& message : messages) {
                if (enqueueMessage(message)) {
                    messagesGenerated++;
                }
            }
            
            // Debug logging for state changes
            if (newState.raw != currentInputState_.raw) {
                debugLog("State transition: 0x" + std::to_string(currentInputState_.raw) + 
                        " -> 0x" + std::to_string(newState.raw));
            }
            
            // Update current state
            currentInputState_ = newState;
        }
        
    } catch (const std::exception& e) {
        debugLog("State-based input processing error: " + std::string(e.what()));
    }
    
    return messagesGenerated;
}

uint16_t InputController::processShiftGestures() {
    if (!dependencies_.inputLayer || !dependencies_.gestureDetector) {
        return 0;
    }
    
    uint16_t messagesGenerated = 0;
    InputEvent inputEvent;
    std::vector<ControlMessage::Message> controlMessages;
    
    // Process SHIFT-specific events through the gesture detector
    // This allows ShiftBasedGestureDetector to handle SHIFT + button combinations
    // for parameter lock functionality while InputStateProcessor handles regular state
    while (dependencies_.inputLayer->getNextEvent(inputEvent)) {
        controlMessages.clear();
        
        // Only process SHIFT-related events through gesture detector
        if (inputEvent.type == InputEvent::Type::SHIFT_BUTTON_PRESS || 
            inputEvent.type == InputEvent::Type::SHIFT_BUTTON_RELEASE) {
            dependencies_.gestureDetector->processInputEvent(inputEvent, controlMessages);
            
            // Queue resulting control messages
            for (const auto& message : controlMessages) {
                if (enqueueMessage(message)) {
                    messagesGenerated++;
                }
            }
        } else {
            // For non-SHIFT events, check if we're in parameter lock mode
            // and let the gesture detector handle them if so
            if (dependencies_.gestureDetector->isInParameterLockMode() &&
                (inputEvent.type == InputEvent::Type::BUTTON_PRESS ||
                 inputEvent.type == InputEvent::Type::BUTTON_RELEASE)) {
                dependencies_.gestureDetector->processInputEvent(inputEvent, controlMessages);
                
                // Queue resulting control messages
                for (const auto& message : controlMessages) {
                    if (enqueueMessage(message)) {
                        messagesGenerated++;
                    }
                }
            }
            // Note: Regular button events are handled by InputStateProcessor in processStateBasedInput()
        }
    }
    
    return messagesGenerated;
}

uint16_t InputController::processInputEventsLegacy() {
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

uint16_t InputController::updateTimingState() {
    if (!dependencies_.clock) {
        return 0;
    }
    
    uint32_t currentTime = dependencies_.clock->getCurrentTime();
    uint16_t messagesGenerated = 0;
    std::vector<ControlMessage::Message> controlMessages;
    
    if (dependencies_.inputStateProcessor) {
        // Modern state system: Check for timing-based state changes
        InputState previousState = currentInputState_;
        InputState currentState = dependencies_.inputLayer->getCurrentInputState();
        
        // Process any timing-based state changes (like hold detection)
        if (currentState.raw != currentInputState_.raw) {
            auto messages = dependencies_.inputStateProcessor->translateState(currentState, currentInputState_, currentTime);
            
            // Add messages to control messages vector
            controlMessages.insert(controlMessages.end(), messages.begin(), messages.end());
            
            // Update current state
            currentInputState_ = currentState;
            
            // Debug logging for timing-based state changes
            debugLog("Timing state transition: 0x" + std::to_string(previousState.raw) + 
                    " -> 0x" + std::to_string(currentState.raw));
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