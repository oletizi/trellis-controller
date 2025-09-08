#ifndef IINPUT_LAYER_H
#define IINPUT_LAYER_H

#include "InputEvent.h"
#include "InputSystemConfiguration.h"
#include <stdint.h>

/**
 * @file IInputLayer.h  
 * @brief Core Input Layer Abstraction interface for cross-platform input handling
 * 
 * This file defines the fundamental abstraction layer that isolates all platform-specific
 * input handling from the core business logic. All input implementations (NeoTrellis hardware,
 * simulation, testing mocks) must implement this interface.
 * 
 * Design Principles:
 * - Complete platform abstraction - no hardware dependencies in interface
 * - Polling-based event retrieval for deterministic behavior
 * - Zero dynamic allocation in interface - all memory managed by implementations
 * - Real-time friendly - all operations have bounded execution time
 * - Configuration-driven behavior for flexibility across platforms
 * - Testability via dependency injection and mocking
 * 
 * This interface replaces the legacy IInput interface with a more comprehensive
 * and platform-agnostic design that supports multiple input types and advanced
 * gesture detection capabilities.
 */

/**
 * @brief Dependencies required by input layer implementations
 * 
 * Contains all external dependencies that input layers need for operation.
 * Using dependency injection allows for testing with mocks and enables
 * different platform implementations to use appropriate platform-specific
 * timing and debugging facilities.
 */
struct InputLayerDependencies {
    /**
     * @brief Clock interface for timing operations
     * 
     * Required for debouncing, hold detection, and timestamp generation.
     * Must not be null for input layer to function correctly.
     */
    class IClock* clock = nullptr;
    
    /**
     * @brief Debug output interface (optional)
     * 
     * Used for logging input events, debugging gesture detection,
     * and diagnosing input system issues. Can be null in production builds.
     */
    class IDebugOutput* debugOutput = nullptr;
    
    /**
     * @brief Validate dependencies are sufficient for operation
     * 
     * @return true if all required dependencies are present
     */
    bool isValid() const {
        return clock != nullptr;  // Clock is required, debug is optional
    }
};

/**
 * @brief Platform-agnostic input layer abstraction
 * 
 * This interface defines the contract between platform-specific input implementations
 * and the core input processing system. It abstracts away all hardware and platform
 * details, providing a uniform way to handle input events across different platforms.
 * 
 * Key Responsibilities:
 * - Translate platform-specific input events to standardized InputEvent format
 * - Handle hardware-specific timing requirements (debouncing, polling)
 * - Provide buffered, non-blocking access to input events
 * - Apply event filtering based on configuration
 * - Maintain input device state for gesture detection
 * 
 * Platform Implementations:
 * - NeoTrellisInputLayer: I2C hardware communication with Seesaw protocol
 * - CursesInputLayer: Keyboard simulation using ncurses
 * - MockInputLayer: Programmable testing input with predefined sequences
 * 
 * The interface is designed to be real-time safe - all operations have bounded
 * execution time and no dynamic memory allocation occurs during normal operation.
 */
class IInputLayer {
public:
    /**
     * @brief Virtual destructor for proper cleanup
     * 
     * Ensures that platform-specific cleanup code is called when
     * input layers are destroyed through the interface pointer.
     */
    virtual ~IInputLayer() = default;
    
    /**
     * @brief Initialize the input layer with given configuration
     * 
     * This method must be called before any other operations. It configures
     * the input layer behavior, initializes hardware (if applicable), and
     * prepares the event processing system.
     * 
     * @param config Complete input system configuration
     * @param deps Dependencies required for operation
     * @return true if initialization succeeded, false on error
     * 
     * @throws std::invalid_argument if configuration is invalid
     * @throws std::runtime_error if hardware initialization fails
     * 
     * Platform Implementation Notes:
     * - NeoTrellis: Initializes I2C communication, configures Seesaw chip
     * - Simulation: Sets up keyboard mappings, initializes ncurses
     * - MockInput: Validates test sequence data, prepares event queue
     */
    virtual bool initialize(const InputSystemConfiguration& config, 
                           const InputLayerDependencies& deps) = 0;
    
    /**
     * @brief Shutdown the input layer and release resources
     * 
     * Performs clean shutdown of input processing, releases hardware resources,
     * and ensures the input layer can be safely destroyed. Should be called
     * before destruction or when input is no longer needed.
     * 
     * Platform Implementation Notes:
     * - NeoTrellis: Releases I2C resources, puts hardware in low-power mode
     * - Simulation: Restores terminal state, cleans up ncurses
     * - MockInput: Clears event queues, resets state
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief Poll input devices for new events
     * 
     * This method should be called regularly (typically every 10-20ms) to
     * check for new input events. It performs hardware polling, debouncing,
     * and event generation according to the current configuration.
     * 
     * The method is designed to be fast and non-blocking. It processes
     * any pending hardware events and adds them to the internal event queue
     * but does not block waiting for events to occur.
     * 
     * Platform Implementation Notes:
     * - NeoTrellis: Reads button states via I2C, performs debouncing
     * - Simulation: Checks for keyboard input from ncurses
     * - MockInput: Advances through predefined event sequence
     * 
     * @return true if polling was successful, false on hardware error
     */
    virtual bool poll() = 0;
    
    /**
     * @brief Retrieve the next available input event
     * 
     * Retrieves the oldest unprocessed input event from the input layer's
     * event queue. Events are returned in chronological order based on
     * their timestamps.
     * 
     * This method is non-blocking - it returns immediately whether or not
     * an event is available. Use hasEvents() to check for availability
     * before calling if needed.
     * 
     * @param event Output parameter to receive the next event
     * @return true if an event was retrieved, false if queue is empty
     */
    virtual bool getNextEvent(InputEvent& event) = 0;
    
    /**
     * @brief Check if events are available for retrieval
     * 
     * Provides a fast way to check if getNextEvent() will return an event
     * without actually retrieving it. Useful for event processing loops
     * and avoiding unnecessary function calls.
     * 
     * @return true if at least one event is available in the queue
     */
    virtual bool hasEvents() const = 0;
    
    /**
     * @brief Update input layer configuration at runtime
     * 
     * Allows dynamic reconfiguration of input behavior without requiring
     * shutdown and reinitialization. Useful for adjusting timing parameters,
     * enabling/disabling event types, or changing performance settings.
     * 
     * Not all configuration changes may be supported at runtime - some
     * changes (like grid layout) may require reinitialization.
     * 
     * @param config New configuration to apply
     * @return true if configuration was applied successfully
     * 
     * Implementation Notes:
     * - Should validate configuration before applying changes
     * - May reject incompatible changes and return false
     * - Should preserve current state when possible during reconfiguration
     */
    virtual bool setConfiguration(const InputSystemConfiguration& config) = 0;
    
    /**
     * @brief Get current input layer configuration
     * 
     * Returns the currently active configuration. Useful for debugging,
     * state serialization, and verifying that configuration changes
     * were applied successfully.
     * 
     * @return Current input system configuration
     */
    virtual InputSystemConfiguration getConfiguration() const = 0;
    
    /**
     * @brief Get current input device state
     * 
     * Returns a snapshot of the current state of all input devices.
     * This includes which buttons are currently pressed, encoder positions,
     * and any other device-specific state information.
     * 
     * Useful for state serialization, debugging, and implementing features
     * that need to know the current input state (like chord detection).
     * 
     * @param buttonStates Array to receive button states (true = pressed)
     * @param maxButtons Size of the buttonStates array
     * @return Number of buttons for which state was returned
     * 
     * Note: Array must be at least as large as the number of buttons
     * configured in the input layout, or some button states will not
     * be returned.
     */
    virtual uint8_t getCurrentButtonStates(bool* buttonStates, uint8_t maxButtons) const = 0;
    
    /**
     * @brief Get input layer status and statistics
     * 
     * Returns operational statistics useful for monitoring performance,
     * debugging issues, and optimizing configuration parameters.
     * 
     * @return Status structure with current statistics
     */
    struct InputLayerStatus {
        uint32_t eventsProcessed = 0;    ///< Total events processed since init
        uint32_t eventsDropped = 0;      ///< Events dropped due to full queue
        uint32_t pollCount = 0;          ///< Number of poll() calls made
        uint32_t lastPollTime = 0;       ///< Timestamp of last poll() call
        uint32_t averagePollInterval = 0; ///< Average time between polls (ms)
        uint8_t queueUtilization = 0;    ///< Current queue usage percentage
        bool hardwareError = false;      ///< Hardware communication error flag
        
        /**
         * @brief Reset all statistics counters
         * 
         * Useful for performance measurement and testing.
         */
        void reset() {
            eventsProcessed = 0;
            eventsDropped = 0;
            pollCount = 0;
            lastPollTime = 0;
            averagePollInterval = 0;
            queueUtilization = 0;
            hardwareError = false;
        }
    };
    
    virtual InputLayerStatus getStatus() const = 0;
    
    /**
     * @brief Force immediate event queue flush
     * 
     * Processes any pending hardware events immediately and adds them
     * to the event queue, bypassing normal polling intervals. Useful
     * for responsive handling of critical events or during shutdown.
     * 
     * This operation may take longer than normal poll() calls as it
     * ensures all pending events are processed.
     * 
     * @return Number of events added to queue during flush
     */
    virtual uint8_t flush() = 0;
    
    /**
     * @brief Clear all queued events
     * 
     * Removes all pending events from the input queue. Useful for
     * resetting input state, handling error conditions, or transitioning
     * between different input modes.
     * 
     * @return Number of events that were cleared from the queue
     */
    virtual uint8_t clearEvents() = 0;
};

/**
 * @brief Factory interface for creating platform-specific input layers
 * 
 * Provides a standardized way to create input layer implementations
 * without depending on specific platform classes. This enables the
 * core system to work with different input implementations through
 * the common interface.
 * 
 * Platform-specific factories should implement this interface to
 * provide their input layer implementations.
 */
class IInputLayerFactory {
public:
    virtual ~IInputLayerFactory() = default;
    
    /**
     * @brief Create input layer instance for this platform
     * 
     * @param config Initial configuration for the input layer
     * @param deps Dependencies required for operation
     * @return Unique pointer to created input layer, or nullptr on failure
     */
    virtual std::unique_ptr<IInputLayer> create(
        const InputSystemConfiguration& config,
        const InputLayerDependencies& deps) = 0;
    
    /**
     * @brief Get human-readable name for this input layer type
     * 
     * @return Platform-specific input layer name (e.g., "NeoTrellis", "Simulation")
     */
    virtual const char* getName() const = 0;
    
    /**
     * @brief Check if this factory can create input layers in current environment
     * 
     * Allows factories to check for required hardware, libraries, or other
     * dependencies before attempting to create an input layer.
     * 
     * @return true if input layer can be created successfully
     */
    virtual bool isAvailable() const = 0;
};

#endif // IINPUT_LAYER_H