#include "CursesInputLayer.h"
#include <cstring>
#include <stdexcept>
#include <algorithm>

CursesInputLayer::CursesInputLayer() {
    status_.reset();
}

CursesInputLayer::~CursesInputLayer() {
    shutdown();
}

bool CursesInputLayer::initialize(const InputSystemConfiguration& config, 
                                 const InputLayerDependencies& deps) {
    if (initialized_) {
        if (debug_) debug_->log("CursesInputLayer already initialized");
        return true;
    }
    
    // Validate dependencies
    if (!deps.isValid()) {
        throw std::invalid_argument("CursesInputLayer: Invalid dependencies - clock is required");
    }
    
    // Validate configuration  
    if (!validateConfiguration(config)) {
        throw std::invalid_argument("CursesInputLayer: Invalid configuration parameters");
    }
    
    // Store dependencies and configuration
    clock_ = deps.clock;
    debug_ = deps.debugOutput;
    config_ = config;
    
    // CursesInputLayer is a stateless sensor - no business logic
    
    // Verify ncurses is initialized
    if (stdscr == nullptr) {
        throw std::runtime_error("CursesInputLayer: ncurses not initialized. CursesDisplay must be initialized first.");
    }
    
    try {
        // Configure ncurses for non-blocking input
        nodelay(stdscr, TRUE);
        keypad(stdscr, TRUE); 
        noecho();
        
        // Initialize key mappings
        initializeKeyMapping();
        
        // Reset state
        status_.reset();
        clearEvents();
        
        // Initialize current detections
        currentDetections_.clear();
        
        initialized_ = true;
        
        if (debug_) {
            debug_->log("CursesInputLayer initialized successfully");
        }
        
        return true;
        
    } catch (const std::exception& e) {
        if (debug_) {
            debug_->log("CursesInputLayer initialization failed: " + std::string(e.what()));
        }
        throw std::runtime_error("CursesInputLayer initialization failed: " + std::string(e.what()));
    }
}

void CursesInputLayer::shutdown() {
    if (!initialized_) return;
    
    // Clear event queue
    clearEvents();
    
    // Clear key mappings and current detections
    keyMap_.clear();
    currentDetections_.clear();
    
    initialized_ = false;
    
    if (debug_) {
        debug_->log("CursesInputLayer shutdown complete");
    }
}

bool CursesInputLayer::poll() {
    if (!initialized_) return false;
    
    uint32_t pollStartTime = clock_->getCurrentTime();
    status_.pollCount++;
    
    int key;
    
    // Process all available keys in non-blocking mode
    while ((key = getch()) != ERR) {
        processKeyInput(key);
    }
    
    // Update current detections (stateless)
    updateCurrentDetections();
    
    // Update polling statistics
    status_.lastPollTime = pollStartTime;
    if (status_.pollCount > 1) {
        uint32_t interval = pollStartTime - status_.lastPollTime;
        status_.averagePollInterval = (status_.averagePollInterval * 3 + interval) / 4; // Simple moving average
    }
    
    updateStatistics();
    
    // Return true to indicate successful polling (regardless of whether events were generated)
    return true;
}

bool CursesInputLayer::getNextEvent(InputEvent& event) {
    if (eventQueue_.empty()) {
        return false;
    }
    
    event = eventQueue_.front();
    eventQueue_.pop();
    status_.eventsProcessed++;
    
    updateStatistics();
    
    return true;
}

bool CursesInputLayer::hasEvents() const {
    return !eventQueue_.empty();
}

bool CursesInputLayer::setConfiguration(const InputSystemConfiguration& config) {
    if (!validateConfiguration(config)) {
        if (debug_) debug_->log("CursesInputLayer: Configuration validation failed");
        return false;
    }
    
    config_ = config;
    
    if (debug_) debug_->log("CursesInputLayer configuration updated");
    return true;
}

InputSystemConfiguration CursesInputLayer::getConfiguration() const {
    return config_;
}

uint8_t CursesInputLayer::getCurrentButtonStates(bool* buttonStates, uint8_t maxButtons) const {
    if (!buttonStates || maxButtons == 0) return 0;
    
    // Report what buttons are CURRENTLY detected (stateless)
    uint8_t totalButtons = std::min(static_cast<uint8_t>(GRID_ROWS * GRID_COLS), maxButtons);
    
    // Initialize all buttons as not pressed
    for (uint8_t i = 0; i < totalButtons; ++i) {
        buttonStates[i] = false;
    }
    
    // Set buttons that are currently detected
    for (const auto& [keyCode, detection] : currentDetections_) {
        if (detection.buttonId < totalButtons) {
            buttonStates[detection.buttonId] = true;
        }
    }
    
    return totalButtons;
}

IInputLayer::InputLayerStatus CursesInputLayer::getStatus() const {
    updateStatistics();
    return status_;
}

uint8_t CursesInputLayer::flush() {
    if (!initialized_) return 0;
    
    // Force immediate processing of any pending input
    return poll() ? static_cast<uint8_t>(eventQueue_.size()) : 0;
}

uint8_t CursesInputLayer::clearEvents() {
    uint8_t clearedCount = static_cast<uint8_t>(eventQueue_.size());
    
    while (!eventQueue_.empty()) {
        eventQueue_.pop();
    }
    
    if (debug_ && clearedCount > 0) {
        debug_->log("Cleared " + std::to_string(clearedCount) + " events from queue");
    }
    
    return clearedCount;
}

void CursesInputLayer::initializeKeyMapping() {
    keyMap_.clear();
    
    // Left Bank Keys (columns 0-3): <1234QWERASDFZXCV>
    // Row 0: Numbers 1-4 (left bank)
    keyMap_['1'] = {0, 0}; keyMap_['2'] = {0, 1}; keyMap_['3'] = {0, 2}; keyMap_['4'] = {0, 3};
    
    // Row 1: QWER (left bank, both upper and lowercase)
    keyMap_['q'] = {1, 0}; keyMap_['w'] = {1, 1}; keyMap_['e'] = {1, 2}; keyMap_['r'] = {1, 3};
    keyMap_['Q'] = {1, 0}; keyMap_['W'] = {1, 1}; keyMap_['E'] = {1, 2}; keyMap_['R'] = {1, 3};
    
    // Row 2: ASDF (left bank, both upper and lowercase)
    keyMap_['a'] = {2, 0}; keyMap_['s'] = {2, 1}; keyMap_['d'] = {2, 2}; keyMap_['f'] = {2, 3};
    keyMap_['A'] = {2, 0}; keyMap_['S'] = {2, 1}; keyMap_['D'] = {2, 2}; keyMap_['F'] = {2, 3};
    
    // Row 3: ZXCV (left bank, both upper and lowercase)
    keyMap_['z'] = {3, 0}; keyMap_['x'] = {3, 1}; keyMap_['c'] = {3, 2}; keyMap_['v'] = {3, 3};
    keyMap_['Z'] = {3, 0}; keyMap_['X'] = {3, 1}; keyMap_['C'] = {3, 2}; keyMap_['V'] = {3, 3};
    
    // Right Bank Keys (columns 4-7): <5678TYUIGHJKBNM,>
    // Row 0: Numbers 5-8 (right bank)
    keyMap_['5'] = {0, 4}; keyMap_['6'] = {0, 5}; keyMap_['7'] = {0, 6}; keyMap_['8'] = {0, 7};
    
    // Row 1: TYUI (right bank, both upper and lowercase)
    keyMap_['t'] = {1, 4}; keyMap_['y'] = {1, 5}; keyMap_['u'] = {1, 6}; keyMap_['i'] = {1, 7};
    keyMap_['T'] = {1, 4}; keyMap_['Y'] = {1, 5}; keyMap_['U'] = {1, 6}; keyMap_['I'] = {1, 7};
    
    // Row 2: GHJK (right bank, both upper and lowercase)
    keyMap_['g'] = {2, 4}; keyMap_['h'] = {2, 5}; keyMap_['j'] = {2, 6}; keyMap_['k'] = {2, 7};
    keyMap_['G'] = {2, 4}; keyMap_['H'] = {2, 5}; keyMap_['J'] = {2, 6}; keyMap_['K'] = {2, 7};
    
    // Row 3: BNM, (right bank, both upper and lowercase)
    keyMap_['b'] = {3, 4}; keyMap_['n'] = {3, 5}; keyMap_['m'] = {3, 6}; keyMap_[','] = {3, 7};
    keyMap_['B'] = {3, 4}; keyMap_['N'] = {3, 5}; keyMap_['M'] = {3, 6}; keyMap_['<'] = {3, 7};
}

bool CursesInputLayer::getKeyMapping(int key, uint8_t& row, uint8_t& col) const {
    auto it = keyMap_.find(key);
    if (it != keyMap_.end()) {
        row = it->second.row;
        col = it->second.col;
        return true;
    }
    return false;
}

bool CursesInputLayer::isUppercaseKey(int key) const {
    return (key >= 'A' && key <= 'Z') || key == '<'; // '<' is shift+comma
}

uint8_t CursesInputLayer::getBankForKey(int key) const {
    // Left bank (columns 0-3): 1234QWERASLVDFZXCVj
    if (key == '1' || key == '2' || key == '3' || key == '4' ||
        key == 'q' || key == 'w' || key == 'e' || key == 'r' ||
        key == 'Q' || key == 'W' || key == 'E' || key == 'R' ||
        key == 'a' || key == 's' || key == 'd' || key == 'f' ||
        key == 'A' || key == 'S' || key == 'D' || key == 'F' ||
        key == 'z' || key == 'x' || key == 'c' || key == 'v' ||
        key == 'Z' || key == 'X' || key == 'C' || key == 'V') {
        return 0; // Left bank
    }
    
    // Right bank (columns 4-7): 5678TYUIGHJKBNM,
    if (key == '5' || key == '6' || key == '7' || key == '8' ||
        key == 't' || key == 'y' || key == 'u' || key == 'i' ||
        key == 'T' || key == 'Y' || key == 'U' || key == 'I' ||
        key == 'g' || key == 'h' || key == 'j' || key == 'k' ||
        key == 'G' || key == 'H' || key == 'J' || key == 'K' ||
        key == 'b' || key == 'n' || key == 'm' || key == ',' ||
        key == 'B' || key == 'N' || key == 'M' || key == '<') {
        return 1; // Right bank
    }
    
    return 0; // Default to left bank for unmapped keys
}

void CursesInputLayer::processKeyInput(int key) {
    // Handle special system keys
    if (key == 27) { // ESC key - quit simulation
        InputEvent quitEvent(InputEvent::Type::SYSTEM_EVENT, 255, clock_->getCurrentTime(), 1, 0);
        eventQueue_.push(quitEvent);
        return;
    }
    
    // Check if key maps to a button
    uint8_t row, col;
    if (!getKeyMapping(key, row, col)) {
        // Ignore unmapped keys
        return;
    }
    
    uint32_t currentTime = clock_->getCurrentTime();
    uint8_t buttonId = getButtonIndex(row, col);
    bool isUppercase = isUppercaseKey(key);
    
    // **STATELESS OPERATION**: Just update current detections
    // No state memory - InputStateEncoder handles press/release logic
    
    auto& detection = currentDetections_[key];
    if (detection.pressTimestamp == 0) {
        // First time detecting this key
        detection.pressTimestamp = currentTime;
        detection.buttonId = buttonId;
        
        if (debug_) {
            debug_->log("[CursesInputLayer] KEY DETECTED '" + std::string(1, key) + 
                       "' -> button " + std::to_string(buttonId) + 
                       (isUppercase ? " (uppercase)" : " (lowercase)"));
        }
    }
    
    // For uppercase keys, generate SHIFT + button press/release events
    // This enables parameter lock mode via SHIFT detection
    if (isUppercase) {
        uint8_t bankId = getBankForKey(key);
        
        // Generate SHIFT button press event
        InputEvent pressEvent = InputEvent::shiftButtonPress(buttonId, currentTime, bankId);
        if (eventQueue_.size() < config_.performance.eventQueueSize) {
            eventQueue_.push(pressEvent);
        } else {
            status_.eventsDropped++;
        }
        
        // Generate immediate SHIFT button release with parameter lock duration
        uint32_t holdDuration = 600; // 600ms for parameter lock
        InputEvent releaseEvent = InputEvent::shiftButtonRelease(buttonId, currentTime, holdDuration, bankId);
        if (eventQueue_.size() < config_.performance.eventQueueSize) {
            eventQueue_.push(releaseEvent);
        } else {
            status_.eventsDropped++;
        }
        
        // Remove from current detections since it's completed
        currentDetections_.erase(key);
        
        if (debug_) {
            debug_->log("[CursesInputLayer] SHIFT+KEY " + std::string(1, key) + 
                       " -> SHIFT_BUTTON_PRESS+RELEASE (bank=" + std::to_string(bankId) + 
                       ", duration=" + std::to_string(holdDuration) + "ms)");
        }
    }
    // For lowercase keys, generate regular button press event on first detection
    else {
        // Only generate press event if this is the first detection of this key
        if (detection.pressTimestamp == currentTime) {
            InputEvent pressEvent = InputEvent::buttonPress(buttonId, currentTime);
            if (eventQueue_.size() < config_.performance.eventQueueSize) {
                eventQueue_.push(pressEvent);
                
                if (debug_) {
                    uint8_t bankId = getBankForKey(key);
                    debug_->log("[CursesInputLayer] LOWERCASE KEY " + std::string(1, key) + 
                               " -> BUTTON_PRESS (bank=" + std::to_string(bankId) + 
                               ", sustained hold)");
                }
            } else {
                status_.eventsDropped++;
            }
        }
    }
}

InputEvent CursesInputLayer::createButtonPressEvent(uint8_t buttonId, uint32_t timestamp) const {
    return InputEvent::buttonPress(buttonId, timestamp);
}

InputEvent CursesInputLayer::createButtonReleaseEvent(uint8_t buttonId, uint32_t timestamp, uint32_t pressDuration) const {
    return InputEvent::buttonRelease(buttonId, timestamp, pressDuration);
}

void CursesInputLayer::updateCurrentDetections() {
    // **STATELESS TIMEOUT DETECTION**: Check which keys haven't been seen recently
    // This is the ONLY state we maintain - what keys ncurses is detecting RIGHT NOW
    
    uint32_t currentTime = clock_->getCurrentTime();
    constexpr uint32_t KEY_RELEASE_TIMEOUT_MS = 200; // If no repeat in 200ms, no longer detected
    
    // Find keys that are no longer detected
    std::vector<int> keysNoLongerDetected;
    
    for (auto& [keyCode, detection] : currentDetections_) {
        uint32_t timeSinceLastSeen = currentTime - detection.pressTimestamp;
        
        // If we haven't seen this key recently, it's no longer detected
        if (timeSinceLastSeen >= KEY_RELEASE_TIMEOUT_MS) {
            keysNoLongerDetected.push_back(keyCode);
        }
    }
    
    // Generate release events for keys no longer detected
    for (int keyCode : keysNoLongerDetected) {
        auto& detection = currentDetections_[keyCode];
        uint32_t holdDuration = currentTime - detection.pressTimestamp;
        
        // Generate regular button release event (InputStateEncoder will handle the state logic)
        InputEvent releaseEvent = InputEvent::buttonRelease(detection.buttonId, currentTime, holdDuration);
        
        if (eventQueue_.size() < config_.performance.eventQueueSize) {
            eventQueue_.push(releaseEvent);
            
            if (debug_) {
                debug_->log("[CursesInputLayer] KEY NO LONGER DETECTED " + std::string(1, keyCode) + 
                           " (button " + std::to_string(detection.buttonId) + 
                           ") after " + std::to_string(holdDuration) + "ms");
            }
        } else {
            status_.eventsDropped++;
        }
        
        // Remove from current detections
        currentDetections_.erase(keyCode);
    }
}

InputEvent CursesInputLayer::createRawKeyboardEvent(uint8_t buttonId, uint32_t timestamp, int keyCode, bool uppercase) const {
    // **DEPRECATED**: This method is no longer used since we generate proper BUTTON_PRESS/BUTTON_RELEASE events
    // Kept for potential future raw keyboard event needs
    return InputEvent(InputEvent::Type::SYSTEM_EVENT, buttonId, timestamp, keyCode, uppercase ? 1 : 0);
}

uint8_t CursesInputLayer::getButtonIndex(uint8_t row, uint8_t col) const {
    return row * GRID_COLS + col;
}

bool CursesInputLayer::validateConfiguration(const InputSystemConfiguration& config) const {
    // Verify layout matches our grid
    if (config.layout.gridRows != GRID_ROWS || config.layout.gridCols != GRID_COLS) {
        if (debug_) debug_->log("Invalid grid dimensions for CursesInputLayer");
        return false;
    }
    
    // Verify event queue is reasonable
    if (config.performance.eventQueueSize == 0 || config.performance.eventQueueSize > 1024) {
        if (debug_) debug_->log("Invalid event queue size");
        return false;
    }
    
    // Basic timing validation
    if (config.timing.pollingIntervalMs == 0 || config.timing.pollingIntervalMs > 1000) {
        if (debug_) debug_->log("Invalid polling interval");
        return false;
    }
    
    return true;
}

InputState CursesInputLayer::getCurrentInputState() const {
    // **STATELESS SENSOR REPORT**: Return what keys are detected RIGHT NOW
    // No memory of previous states - just current detections
    
    InputState currentDetectionState{};
    
    // Report all currently detected keys as pressed buttons
    for (const auto& [keyCode, detection] : currentDetections_) {
        if (detection.buttonId < 32) { // Valid button range
            currentDetectionState.setButtonState(detection.buttonId, true);
        }
    }
    
    return currentDetectionState;
}

void CursesInputLayer::updateStatistics() const {
    // Update queue utilization percentage
    uint16_t queueSize = static_cast<uint16_t>(eventQueue_.size());
    uint16_t maxSize = config_.performance.eventQueueSize;
    status_.queueUtilization = static_cast<uint8_t>((queueSize * 100) / maxSize);
    
    // Hardware error is always false for simulation
    status_.hardwareError = false;
}