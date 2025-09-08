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
 * Key Mapping:
 * Row 0: 1 2 3 4 5 6 7 8  (numbers)
 * Row 1: q w e r t y u i  (QWERTY row, both upper/lower case)  
 * Row 2: a s d f g h j k  (home row, both upper/lower case)
 * Row 3: z x c v b n m ,  (bottom row, both upper/lower case)
 * 
 * Input Behavior:
 * - Lowercase/numbers: Quick tap (press + immediate release)
 * - Uppercase letters: Hold until corresponding lowercase releases
 * - ESC: System quit event
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
    bool buttonStates_[GRID_ROWS][GRID_COLS];
    
    // Key mapping and event processing
    std::map<int, KeyMapping> keyMap_;
    std::queue<InputEvent> eventQueue_;
    
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
     * Used to determine hold vs tap behavior for simulation.
     * Uppercase = hold, lowercase/numbers = tap.
     * 
     * @param key NCurses key code
     * @return true if key is uppercase letter
     */
    bool isUppercaseKey(int key) const;
    
    /**
     * @brief Process single keyboard key input
     * 
     * Handles the key mapping, state tracking, and event generation
     * for a single key press. Manages hold vs tap behavior and
     * creates appropriate InputEvents.
     * 
     * @param key NCurses key code
     */
    void processKeyInput(int key);
    
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