#ifndef INPUT_SYSTEM_CONFIGURATION_H
#define INPUT_SYSTEM_CONFIGURATION_H

#include <stdint.h>
#include "InputEvent.h"

/**
 * @file InputSystemConfiguration.h
 * @brief Configuration structures for the Input Layer Abstraction system
 * 
 * This file defines configuration structures that control the behavior of
 * input layer implementations. The configuration is platform-agnostic but
 * allows platform-specific optimizations and customizations.
 * 
 * Design Principles:
 * - Compile-time constants where possible for embedded optimization
 * - Runtime configuration for testing and simulation flexibility
 * - Sensible defaults that work across all platforms
 * - Memory-conscious design for embedded constraints
 */

/**
 * @brief Core timing configuration for input processing
 * 
 * Controls debouncing, gesture detection, and polling behavior.
 * Values are in milliseconds for platform independence.
 */
struct InputTimingConfiguration {
    /**
     * @brief Button debounce time in milliseconds
     * 
     * Minimum time between button state changes to prevent bouncing.
     * Higher values are more conservative but may feel less responsive.
     * - Typical embedded: 20-50ms
     * - Simulation: 5-10ms (no physical bouncing)
     * - Testing: 0ms (deterministic)
     */
    uint32_t buttonDebounceMs = 20;
    
    /**
     * @brief Hold threshold time in milliseconds
     * 
     * Time a button must be held to register as a "hold" gesture
     * rather than a tap. Critical for parameter lock functionality.
     * - Typical: 500ms (half second)
     * - Fast users: 300-400ms
     * - Slow/accessibility: 800-1000ms
     */
    uint32_t holdThresholdMs = 500;
    
    /**
     * @brief Double-tap detection window in milliseconds
     * 
     * Maximum time between two taps to register as double-tap.
     * Shorter values require faster tapping but reduce accidental triggers.
     * - Typical: 300ms
     * - Fast interface: 200-250ms
     * - Accessibility: 400-500ms
     */
    uint32_t doubleTapThresholdMs = 300;
    
    /**
     * @brief Encoder debounce time in milliseconds
     * 
     * Minimum time between encoder position changes.
     * Mechanical encoders may need 5-10ms, optical encoders can be faster.
     */
    uint32_t encoderDebounceMs = 5;
    
    /**
     * @brief Polling interval in milliseconds
     * 
     * How often the input layer should check for new events.
     * Smaller values provide better responsiveness but use more CPU.
     * - Embedded: 10-20ms (50-100Hz)
     * - Simulation: 16ms (60FPS)
     * - Testing: 1ms (maximum precision)
     */
    uint32_t pollingIntervalMs = 10;
    
    /**
     * @brief Chord detection window in milliseconds
     * 
     * Maximum time difference between button presses to be considered
     * part of the same chord. Used for multi-button gesture detection.
     */
    uint32_t chordDetectionMs = 50;
    
    /**
     * @brief Get configuration optimized for embedded systems
     * 
     * @return Configuration with conservative timing suitable for hardware
     */
    static InputTimingConfiguration forEmbedded() {
        InputTimingConfiguration config;
        config.buttonDebounceMs = 25;
        config.holdThresholdMs = 500;
        config.doubleTapThresholdMs = 300;
        config.encoderDebounceMs = 10;
        config.pollingIntervalMs = 20;
        config.chordDetectionMs = 100;
        return config;
    }
    
    /**
     * @brief Get configuration optimized for simulation
     * 
     * @return Configuration with responsive timing for desktop simulation
     */
    static InputTimingConfiguration forSimulation() {
        InputTimingConfiguration config;
        config.buttonDebounceMs = 5;
        config.holdThresholdMs = 400;
        config.doubleTapThresholdMs = 250;
        config.encoderDebounceMs = 2;
        config.pollingIntervalMs = 16;  // 60 FPS
        config.chordDetectionMs = 50;
        return config;
    }
    
    /**
     * @brief Get configuration optimized for testing
     * 
     * @return Configuration with minimal delays for deterministic testing
     */
    static InputTimingConfiguration forTesting() {
        InputTimingConfiguration config;
        config.buttonDebounceMs = 0;     // No debouncing in tests
        config.holdThresholdMs = 100;    // Fast holds for quick tests
        config.doubleTapThresholdMs = 50; // Fast double-taps
        config.encoderDebounceMs = 0;    // No encoder debouncing
        config.pollingIntervalMs = 1;    // Maximum precision
        config.chordDetectionMs = 10;    // Tight chord timing
        return config;
    }
};

/**
 * @brief Hardware layout and mapping configuration
 * 
 * Describes the physical layout of input devices and how they map
 * to logical functions. Platform-specific implementations use this
 * to translate hardware events to standardized InputEvents.
 */
struct InputLayoutConfiguration {
    /**
     * @brief Grid dimensions for button matrix devices
     * 
     * For NeoTrellis: 4 rows x 8 columns = 32 buttons
     * For simulation: Can be configured for different layouts
     */
    uint8_t gridRows = 4;
    uint8_t gridCols = 8;
    
    /**
     * @brief Total number of buttons available
     * 
     * Should equal gridRows * gridCols for matrix layouts.
     * Can be different for non-matrix button arrangements.
     */
    uint8_t totalButtons = 32;
    
    /**
     * @brief Number of encoders available
     * 
     * Current hardware has 0 encoders, but architecture supports
     * future expansion or alternative hardware configurations.
     */
    uint8_t encoderCount = 0;
    
    /**
     * @brief MIDI input channels to monitor
     * 
     * Bitmask of MIDI channels (0-15) to process.
     * Bit 0 = Channel 1, Bit 15 = Channel 16.
     * 0xFFFF = all channels, 0x0001 = channel 1 only.
     */
    uint16_t midiChannelMask = 0x0001;  // Channel 1 only by default
    
    /**
     * @brief Convert button row/col to linear index
     * 
     * @param row Button row (0-based)
     * @param col Button column (0-based) 
     * @return Linear button index, or 255 if invalid
     */
    uint8_t buttonIndex(uint8_t row, uint8_t col) const {
        if (row >= gridRows || col >= gridCols) {
            return 255;  // Invalid index
        }
        return row * gridCols + col;
    }
    
    /**
     * @brief Convert linear button index to row/col
     * 
     * @param index Linear button index
     * @param row Output parameter for row
     * @param col Output parameter for column
     * @return true if conversion successful
     */
    bool indexToRowCol(uint8_t index, uint8_t& row, uint8_t& col) const {
        if (index >= totalButtons) {
            return false;
        }
        row = index / gridCols;
        col = index % gridCols;
        return true;
    }
    
    /**
     * @brief Check if button index is valid
     * 
     * @param index Button index to validate
     * @return true if index is within valid range
     */
    bool isValidButtonIndex(uint8_t index) const {
        return index < totalButtons;
    }
    
    /**
     * @brief Get standard NeoTrellis M4 layout
     * 
     * @return Configuration for 4x8 NeoTrellis M4 button grid
     */
    static InputLayoutConfiguration forNeoTrellis() {
        InputLayoutConfiguration config;
        config.gridRows = 4;
        config.gridCols = 8; 
        config.totalButtons = 32;
        config.encoderCount = 0;
        config.midiChannelMask = 0x0001;  // Channel 1
        return config;
    }
    
    /**
     * @brief Get flexible layout for simulation/testing
     * 
     * @param rows Number of button rows
     * @param cols Number of button columns
     * @return Configuration for custom button layout
     */
    static InputLayoutConfiguration forCustomGrid(uint8_t rows, uint8_t cols) {
        InputLayoutConfiguration config;
        config.gridRows = rows;
        config.gridCols = cols;
        config.totalButtons = rows * cols;
        config.encoderCount = 0;
        config.midiChannelMask = 0xFFFF;  // All channels for flexibility
        return config;
    }
};

/**
 * @brief Performance and resource configuration
 * 
 * Controls memory allocation, queue sizes, and performance trade-offs.
 * Critical for embedded systems with limited resources.
 */
struct InputPerformanceConfiguration {
    /**
     * @brief Event queue size
     * 
     * Maximum number of input events that can be queued before
     * older events are dropped. Must be a power of 2 for efficiency.
     * - Embedded: 16-32 events (minimal memory)
     * - Simulation: 64-128 events (smooth interaction)
     * - Testing: 256+ events (capture long sequences)
     */
    uint16_t eventQueueSize = 32;
    
    /**
     * @brief Message queue size
     * 
     * Maximum number of control messages that can be queued in
     * the InputController. Should be larger than eventQueueSize
     * since one input event may generate multiple messages.
     */
    uint16_t messageQueueSize = 64;
    
    /**
     * @brief Maximum simultaneous button presses to track
     * 
     * Limits memory usage for chord detection and multi-touch
     * gesture recognition. 10 fingers is theoretical maximum.
     */
    uint8_t maxSimultaneousButtons = 10;
    
    /**
     * @brief Enable high-precision timestamping
     * 
     * When true, uses microsecond precision for timestamps.
     * Requires more CPU but provides better accuracy for
     * timing-sensitive applications.
     */
    bool highPrecisionTiming = false;
    
    /**
     * @brief Enable event coalescing
     * 
     * When true, combines similar sequential events to reduce
     * queue usage. For example, multiple encoder steps in the
     * same direction become one event with larger delta.
     */
    bool enableEventCoalescing = true;
    
    /**
     * @brief Priority for input processing
     * 
     * 0 = lowest priority (background processing)
     * 255 = highest priority (real-time)
     * Used by platform-specific implementations that support
     * priority-based scheduling.
     */
    uint8_t processingPriority = 128;
    
    /**
     * @brief Get configuration optimized for embedded systems
     * 
     * @return Configuration that minimizes memory and CPU usage
     */
    static InputPerformanceConfiguration forEmbedded() {
        InputPerformanceConfiguration config;
        config.eventQueueSize = 16;
        config.messageQueueSize = 32;
        config.maxSimultaneousButtons = 5;
        config.highPrecisionTiming = false;
        config.enableEventCoalescing = true;
        config.processingPriority = 200;  // High priority for responsiveness
        return config;
    }
    
    /**
     * @brief Get configuration optimized for simulation
     * 
     * @return Configuration that prioritizes smooth user interaction
     */
    static InputPerformanceConfiguration forSimulation() {
        InputPerformanceConfiguration config;
        config.eventQueueSize = 64;
        config.messageQueueSize = 128;
        config.maxSimultaneousButtons = 10;
        config.highPrecisionTiming = true;
        config.enableEventCoalescing = false;  // Preserve all events for debugging
        config.processingPriority = 128;  // Normal priority
        return config;
    }
    
    /**
     * @brief Get configuration optimized for testing
     * 
     * @return Configuration that captures all events for analysis
     */
    static InputPerformanceConfiguration forTesting() {
        InputPerformanceConfiguration config;
        config.eventQueueSize = 128;
        config.messageQueueSize = 256;
        config.maxSimultaneousButtons = 32;  // Allow testing of many simultaneous presses
        config.highPrecisionTiming = true;
        config.enableEventCoalescing = false;  // Preserve all events for verification
        config.processingPriority = 64;   // Lower priority, testing not time-critical
        return config;
    }
};

/**
 * @brief Complete input system configuration
 * 
 * Combines all configuration aspects into a single structure
 * that can be passed to input layer implementations.
 */
struct InputSystemConfiguration {
    InputTimingConfiguration timing;
    InputLayoutConfiguration layout;
    InputPerformanceConfiguration performance;
    InputEventFilter eventFilter;
    
    /**
     * @brief Default constructor with sensible defaults
     * 
     * Creates configuration suitable for most use cases.
     * Individual components can be modified as needed.
     */
    InputSystemConfiguration() = default;
    
    /**
     * @brief Create configuration optimized for NeoTrellis M4 hardware
     * 
     * @return Complete configuration for embedded NeoTrellis deployment
     */
    static InputSystemConfiguration forNeoTrellis() {
        InputSystemConfiguration config;
        config.timing = InputTimingConfiguration::forEmbedded();
        config.layout = InputLayoutConfiguration::forNeoTrellis();
        config.performance = InputPerformanceConfiguration::forEmbedded();
        config.eventFilter.enableButtons = true;
        config.eventFilter.enableEncoders = false;  // No encoders on NeoTrellis
        config.eventFilter.enableMidi = true;
        config.eventFilter.enableSystemEvents = false;
        return config;
    }
    
    /**
     * @brief Create configuration optimized for simulation
     * 
     * @return Complete configuration for desktop simulation
     */
    static InputSystemConfiguration forSimulation() {
        InputSystemConfiguration config;
        config.timing = InputTimingConfiguration::forSimulation();
        config.layout = InputLayoutConfiguration::forNeoTrellis();  // Same layout as hardware
        config.performance = InputPerformanceConfiguration::forSimulation();
        config.eventFilter.enableButtons = true;
        config.eventFilter.enableEncoders = true;   // Can simulate encoders
        config.eventFilter.enableMidi = true;
        config.eventFilter.enableSystemEvents = true;  // Can simulate system events
        return config;
    }
    
    /**
     * @brief Create configuration optimized for testing
     * 
     * @param gridRows Number of button rows for test layout
     * @param gridCols Number of button columns for test layout
     * @return Complete configuration for unit testing
     */
    static InputSystemConfiguration forTesting(uint8_t gridRows = 4, uint8_t gridCols = 8) {
        InputSystemConfiguration config;
        config.timing = InputTimingConfiguration::forTesting();
        config.layout = InputLayoutConfiguration::forCustomGrid(gridRows, gridCols);
        config.performance = InputPerformanceConfiguration::forTesting();
        config.eventFilter.enableButtons = true;
        config.eventFilter.enableEncoders = true;
        config.eventFilter.enableMidi = true;
        config.eventFilter.enableSystemEvents = true;
        return config;
    }
    
    /**
     * @brief Validate configuration for consistency
     * 
     * Checks that all configuration values are reasonable and
     * consistent with each other. Used for debugging configuration
     * issues during development.
     * 
     * @return true if configuration appears valid
     */
    bool isValid() const {
        // Basic sanity checks
        if (layout.totalButtons == 0 || layout.totalButtons > 64) {
            return false;  // Invalid button count
        }
        
        if (performance.eventQueueSize == 0 || performance.messageQueueSize == 0) {
            return false;  // Must have some queue space
        }
        
        if (timing.holdThresholdMs < timing.buttonDebounceMs) {
            return false;  // Hold must be longer than debounce
        }
        
        if (layout.gridRows * layout.gridCols != layout.totalButtons) {
            return false;  // Grid dimensions don't match total
        }
        
        return true;
    }
};

#endif // INPUT_SYSTEM_CONFIGURATION_H