#ifndef CURSES_INPUT_LAYER_H
#define CURSES_INPUT_LAYER_H

#include "IInputLayer.h"
#include "InputEvent.h"
#include "InputSystemConfiguration.h"
#include "IClock.h"
#include "IDebugOutput.h"
#include <ncurses.h>
#include <queue>
#include <map>
#include <stdint.h>

/**
 * @file CursesInputLayer.h
 * @brief NCurses-based input layer implementation for simulation environment
 * 
 * Provides keyboard-based simulation of the NeoTrellis M4 button grid using
 * ncurses for cross-platform terminal input handling. Maps keyboard keys
 * to button positions and translates key events to standardized InputEvents.
 * 
 * Key Mapping (Bank-Aware):
 * Left Bank (cols 0-3):  <1234QWERASDFZXCV>
 * Right Bank (cols 4-7): <5678TYUIGHJKBNM,>
 * 
 * Row 0: 1234 | 5678  (numbers)
 * Row 1: qwer | tyui  (QWERTY row, both upper/lower case)  
 * Row 2: asdf | ghjk  (home row, both upper/lower case)
 * Row 3: zxcv | bnm,  (bottom row, both upper/lower case)
 * 
 * Input Behavior:
 * - Lowercase keys: Generate BUTTON_PRESS/BUTTON_RELEASE events (normal mode)
 * - Uppercase keys: Generate SHIFT_BUTTON_PRESS/SHIFT_BUTTON_RELEASE events (parameter lock)
 * - Bank information encoded in SHIFT events for left/right bank distinction
 * - ESC: System quit event
 * 
 * Phase 1 SHIFT Detection:
 * - Uppercase keys detected via capital letters (A-Z) and '<' (shift+comma)
 * - SHIFT events include bank information in context field
 * - Foundation for parameter lock system implementation
 * 
 * Note: This platform layer handles key-to-event translation and SHIFT detection.
 * Higher-level components (InputController, GestureDetector) handle gesture recognition.
 * 
 * For complete input documentation and examples, see docs/SIMULATION.md
 * 
 * This implementation supports the full InputEvent system with proper
 * timestamping, debouncing simulation, and event filtering.
 */
class CursesInputLayer : public IInputLayer {
public:
    /**
     * @brief Grid dimensions matching NeoTrellis M4
     */
    static constexpr uint8_t GRID_ROWS = 4;
    static constexpr uint8_t GRID_COLS = 8;
    
    /**
     * @brief Default constructor
     * 
     * Creates uninitialized input layer. Must call initialize()
     * before use with valid dependencies.
     */
    CursesInputLayer();
    
    
    /**
     * @brief Virtual destructor ensures proper cleanup
     */
    ~CursesInputLayer() override;
    
    /**
     * @brief Get current input state (stateless sensor report)
     * 
     * Returns current detections as InputState. This is a pure sensor
     * report - what keys are detected RIGHT NOW, with no memory of
     * previous states. InputStateEncoder handles all state transitions.
     * 
     * @return InputState representing current key detections
     */
    InputState getCurrentInputState() const override;
    
    // IInputLayer interface implementation
    bool initialize(const InputSystemConfiguration& config, 
                   const InputLayerDependencies& deps) override;
    void shutdown() override;
    bool poll() override;
    bool getNextEvent(InputEvent& event) override;
    bool hasEvents() const override;
    bool setConfiguration(const InputSystemConfiguration& config) override;
    InputSystemConfiguration getConfiguration() const override;
    uint8_t getCurrentButtonStates(bool* buttonStates, uint8_t maxButtons) const override;
    InputLayerStatus getStatus() const override;
    uint8_t flush() override;
    uint8_t clearEvents() override;

private:
    /**
     * @brief Key to button position mapping
     */
    struct KeyMapping {
        uint8_t row;    ///< Button row (0-3)
        uint8_t col;    ///< Button column (0-7)
    };
    
    // Configuration and dependencies
    InputSystemConfiguration config_;
    IClock* clock_ = nullptr;
    IDebugOutput* debug_ = nullptr;
    
    // State management
    bool initialized_ = false;
    
    // Key mapping and event processing
    std::map<int, KeyMapping> keyMap_;
    std::queue<InputEvent> eventQueue_;
    
    // Current key detection (stateless - only what's detected NOW)
    struct KeyDetection {
        uint32_t pressTimestamp = 0;      ///< When this key was first detected (for duration)
        uint8_t buttonId = 255;           ///< Associated button ID
    };
    
    std::map<int, KeyDetection> currentDetections_;   ///< What keys are detected RIGHT NOW
    
    // Statistics for status reporting
    mutable InputLayerStatus status_;
    
    /**
     * @brief Initialize keyboard to button mapping
     * 
     * Sets up the translation table from ncurses key codes
     * to button grid positions. Supports both upper and
     * lowercase letters for hold vs tap behavior.
     */
    void initializeKeyMapping();
    
    /**
     * @brief Get button position for keyboard key
     * 
     * @param key NCurses key code
     * @param row Output parameter for button row
     * @param col Output parameter for button column
     * @return true if key maps to a button
     */
    bool getKeyMapping(int key, uint8_t& row, uint8_t& col) const;
    
    /**
     * @brief Check if key represents uppercase letter
     * 
     * Used to determine SHIFT state for parameter lock mode.
     * Uppercase keys generate SHIFT_BUTTON_PRESS/SHIFT_BUTTON_RELEASE events.
     * 
     * @param key NCurses key code
     * @return true if key is uppercase letter
     */
    bool isUppercaseKey(int key) const;
    
    /**
     * @brief Get bank ID for a key
     * 
     * Determines which bank a key belongs to for parameter lock system.
     * Left bank: columns 0-3 (<1234QWERASDFZXCV>)
     * Right bank: columns 4-7 (<5678TYUIGHJKBNM,>)
     * 
     * @param key NCurses key code
     * @return Bank ID (0 = left bank, 1 = right bank)
     */
    uint8_t getBankForKey(int key) const;
    
    /**
     * @brief Process single keyboard key input
     * 
     * Implements Phase 1 SHIFT detection:
     * - Uppercase keys generate SHIFT_BUTTON_PRESS/SHIFT_BUTTON_RELEASE events
     * - Lowercase keys generate regular BUTTON_PRESS/BUTTON_RELEASE events
     * - Bank information included in SHIFT events for parameter lock system
     * - Proper hold duration calculation for both event types
     * 
     * @param key NCurses key code
     */
    void processKeyInput(int key);
    
    /**
     * @brief Update current detections based on ncurses input
     * 
     * Stateless operation: only updates what keys are detected right now.
     * No memory of previous states - that's handled by InputStateEncoder.
     * Generates appropriate SHIFT or regular button events based on key case.
     */
    void updateCurrentDetections();
    
    /**
     * @brief Create button press event
     * 
     * @param buttonId Linear button index (row * cols + col)
     * @param timestamp When the press occurred
     * @return Configured InputEvent for button press
     */
    InputEvent createButtonPressEvent(uint8_t buttonId, uint32_t timestamp) const;
    
    /**
     * @brief Create button release event
     * 
     * @param buttonId Linear button index (row * cols + col)  
     * @param timestamp When the release occurred
     * @param pressDuration How long button was held (milliseconds)
     * @return Configured InputEvent for button release
     */
    InputEvent createButtonReleaseEvent(uint8_t buttonId, uint32_t timestamp, uint32_t pressDuration) const;
    
    /**
     * @brief Create raw keyboard event (DEPRECATED)
     * 
     * Deprecated: Phase 1 implementation now generates proper SHIFT events.
     * Uppercase keys generate SHIFT_BUTTON_PRESS/SHIFT_BUTTON_RELEASE.
     * Lowercase keys generate regular BUTTON_PRESS/BUTTON_RELEASE.
     * 
     * @param buttonId Linear button index (row * cols + col)
     * @param timestamp When the key was pressed
     * @param keyCode Raw keyboard key code
     * @param uppercase Whether the key is uppercase
     * @return Configured InputEvent for raw keyboard input
     */
    InputEvent createRawKeyboardEvent(uint8_t buttonId, uint32_t timestamp, int keyCode, bool uppercase) const;
    
    /**
     * @brief Convert row/col to linear button index
     * 
     * @param row Button row (0-3)
     * @param col Button column (0-7)
     * @return Linear index (0-31)
     */
    uint8_t getButtonIndex(uint8_t row, uint8_t col) const;
    
    /**
     * @brief Validate configuration parameters
     * 
     * @param config Configuration to validate
     * @return true if configuration is acceptable
     */
    bool validateConfiguration(const InputSystemConfiguration& config) const;
    
    /**
     * @brief Update internal statistics
     * 
     * Called during poll() and event processing to maintain
     * accurate performance statistics for debugging.
     */
    void updateStatistics() const;
};

#endif // CURSES_INPUT_LAYER_H