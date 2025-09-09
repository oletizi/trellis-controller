#include "CursesInputLayer.h"
#include <cstring>
#include <stdexcept>
#include <algorithm>

CursesInputLayer::CursesInputLayer() {
    status_.reset();
    // Initialize states with default values
    currentState_ = InputState{};
    previousState_ = InputState{};
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
    
    // Note: InputStateEncoder must be provided by higher-level components
    // since CursesInputLayer is a platform layer and doesn't create business logic
    encoder_ = nullptr;  // Will be set by InputController or similar
    
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
        
        // Initialize input states
        currentState_ = InputState{};
        previousState_ = InputState{};
        
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
    
    // Clear key mappings
    keyMap_.clear();
    
    // Reset states
    currentState_ = InputState{};
    previousState_ = InputState{};
    encoder_ = nullptr;
    
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
    
    // Now we can provide authoritative button state from currentState_
    uint8_t totalButtons = std::min(static_cast<uint8_t>(GRID_ROWS * GRID_COLS), maxButtons);
    
    // Extract button states from our authoritative InputState
    for (uint8_t i = 0; i < totalButtons; ++i) {
        buttonStates[i] = currentState_.isButtonPressed(i);
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
    
    // Row 0: Numbers 1-8 (Track 0)
    keyMap_['1'] = {0, 0}; keyMap_['2'] = {0, 1}; keyMap_['3'] = {0, 2}; keyMap_['4'] = {0, 3};
    keyMap_['5'] = {0, 4}; keyMap_['6'] = {0, 5}; keyMap_['7'] = {0, 6}; keyMap_['8'] = {0, 7};
    
    // Row 1: QWERTY row - both upper and lowercase (Track 1)
    keyMap_['q'] = {1, 0}; keyMap_['w'] = {1, 1}; keyMap_['e'] = {1, 2}; keyMap_['r'] = {1, 3};
    keyMap_['t'] = {1, 4}; keyMap_['y'] = {1, 5}; keyMap_['u'] = {1, 6}; keyMap_['i'] = {1, 7};
    keyMap_['Q'] = {1, 0}; keyMap_['W'] = {1, 1}; keyMap_['E'] = {1, 2}; keyMap_['R'] = {1, 3};
    keyMap_['T'] = {1, 4}; keyMap_['Y'] = {1, 5}; keyMap_['U'] = {1, 6}; keyMap_['I'] = {1, 7};
    
    // Row 2: ASDF row - both upper and lowercase (Track 2)  
    keyMap_['a'] = {2, 0}; keyMap_['s'] = {2, 1}; keyMap_['d'] = {2, 2}; keyMap_['f'] = {2, 3};
    keyMap_['g'] = {2, 4}; keyMap_['h'] = {2, 5}; keyMap_['j'] = {2, 6}; keyMap_['k'] = {2, 7};
    keyMap_['A'] = {2, 0}; keyMap_['S'] = {2, 1}; keyMap_['D'] = {2, 2}; keyMap_['F'] = {2, 3};
    keyMap_['G'] = {2, 4}; keyMap_['H'] = {2, 5}; keyMap_['J'] = {2, 6}; keyMap_['K'] = {2, 7};
    
    // Row 3: ZXCV row - both upper and lowercase (Track 3)
    keyMap_['z'] = {3, 0}; keyMap_['x'] = {3, 1}; keyMap_['c'] = {3, 2}; keyMap_['v'] = {3, 3};
    keyMap_['b'] = {3, 4}; keyMap_['n'] = {3, 5}; keyMap_['m'] = {3, 6}; keyMap_[','] = {3, 7};
    keyMap_['Z'] = {3, 0}; keyMap_['X'] = {3, 1}; keyMap_['C'] = {3, 2}; keyMap_['V'] = {3, 3};
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
    
    // Create raw keyboard event - platform layer just translates, doesn't interpret
    // The 'value' field carries the raw key code, 'context' carries uppercase flag
    // This removes state interpretation from the platform layer
    InputEvent event = createRawKeyboardEvent(buttonId, currentTime, key, isUppercase);
    
    if (debug_) {
        debug_->log("[CursesInputLayer] RAW KEY " + std::string(1, key) + 
                   " (" + (isUppercase ? "uppercase" : "lowercase") + 
                   ") -> RAW EVENT for button " + std::to_string(buttonId));
    }
    
    // **NEW: Update authoritative state using InputStateEncoder**
    if (encoder_) {
        // Store previous state for transition detection
        previousState_ = currentState_;
        
        // Update current state through encoder
        currentState_ = encoder_->processInputEvent(event, currentState_);
        
        if (debug_) {
            debug_->log("[CursesInputLayer] State updated - Button " + std::to_string(buttonId) + 
                       " now " + (currentState_.isButtonPressed(buttonId) ? "PRESSED" : "RELEASED"));
        }
    } else if (debug_) {
        debug_->log("[CursesInputLayer] No encoder available - state update skipped");
    }
    
    // **Maintain backward compatibility**: Still generate events for legacy interface
    // Check for queue overflow
    if (eventQueue_.size() >= config_.performance.eventQueueSize) {
        status_.eventsDropped++;
        if (debug_) debug_->log("Event queue overflow - dropping event");
        return;
    }
    
    eventQueue_.push(event);
}

InputEvent CursesInputLayer::createButtonPressEvent(uint8_t buttonId, uint32_t timestamp) const {
    return InputEvent(InputEvent::Type::BUTTON_PRESS, buttonId, timestamp, 1, 0);
}

InputEvent CursesInputLayer::createButtonReleaseEvent(uint8_t buttonId, uint32_t timestamp, uint32_t pressDuration) const {
    return InputEvent(InputEvent::Type::BUTTON_RELEASE, buttonId, timestamp, static_cast<int32_t>(pressDuration), 0);
}

InputEvent CursesInputLayer::createRawKeyboardEvent(uint8_t buttonId, uint32_t timestamp, int keyCode, bool uppercase) const {
    // Use SYSTEM_EVENT type with specific deviceId to indicate raw keyboard input
    // deviceId = buttonId, value = keyCode, context = uppercase flag
    // This allows higher layers to interpret the semantic meaning
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
    return currentState_;
}

void CursesInputLayer::setInputStateEncoder(InputStateEncoder* encoder) {
    encoder_ = encoder;
    
    if (debug_) {
        if (encoder) {
            debug_->log("[CursesInputLayer] InputStateEncoder set - state management enabled");
        } else {
            debug_->log("[CursesInputLayer] InputStateEncoder cleared - state management disabled");
        }
    }
}

void CursesInputLayer::updateStatistics() const {
    // Update queue utilization percentage
    uint16_t queueSize = static_cast<uint16_t>(eventQueue_.size());
    uint16_t maxSize = config_.performance.eventQueueSize;
    status_.queueUtilization = static_cast<uint8_t>((queueSize * 100) / maxSize);
    
    // Hardware error is always false for simulation
    status_.hardwareError = false;
}