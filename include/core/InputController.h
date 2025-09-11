#ifndef INPUT_CONTROLLER_H
#define INPUT_CONTROLLER_H

#include "IInputLayer.h"
#include "IGestureDetector.h"
#include "ControlMessage.h"
#include "InputSystemConfiguration.h"
#include "IClock.h"
#include "IDebugOutput.h"
#include "InputStateEncoder.h"
#include "InputStateProcessor.h"
#include <memory>
#include <queue>
#include <vector>

/**
 * @file InputController.h
 * @brief Central coordinator for the input system architecture
 * 
 * The InputController is the main orchestrator that connects the platform-specific
 * input layer with gesture detection and provides a clean interface to the
 * sequencer business logic. It manages the complete input processing pipeline:
 * 
 * InputLayer → InputEvents → InputStateEncoder → InputState → InputStateProcessor → ControlMessages → Sequencer
 * 
 * Key Responsibilities:
 * - Coordinate polling from the input layer
 * - Process raw InputEvents through gesture detection
 * - Buffer and deliver ControlMessages to consumers
 * - Manage timing and state updates across components
 * - Provide unified configuration management
 * - Handle errors and resource management
 * 
 * This replaces the legacy IInput interface with a more sophisticated
 * architecture that supports multiple platforms and complex gesture recognition.
 */
class InputController {
public:
    /**
     * @brief Dependencies required for InputController operation
     */
    struct Dependencies {
        std::unique_ptr<IInputLayer> inputLayer;           ///< Platform-specific input implementation
        std::unique_ptr<IGestureDetector> gestureDetector; ///< Legacy gesture recognition (optional for backward compatibility)
        std::unique_ptr<InputStateProcessor> inputStateProcessor; ///< State-based message generation
        IClock* clock = nullptr;                           ///< Clock for timing operations
        IDebugOutput* debugOutput = nullptr;               ///< Optional debug output
        
        /**
         * @brief Check if all required dependencies are present
         * 
         * Prioritizes new state-based system (InputStateProcessor) over legacy system.
         * 
         * @return true if all required dependencies are available
         */
        bool isValid() const {
            if (!inputLayer || !clock) return false;
            // Either new system (processor) or legacy system (gestureDetector) is required
            bool hasNewSystem = inputStateProcessor != nullptr;
            bool hasLegacySystem = gestureDetector != nullptr;
            return hasNewSystem || hasLegacySystem;
        }
    };
    
    /**
     * @brief Operational status of the input controller
     */
    struct Status {
        uint32_t eventsProcessed = 0;     ///< Total input events processed
        uint32_t messagesGenerated = 0;   ///< Total control messages generated
        uint32_t pollCount = 0;           ///< Number of poll cycles completed
        uint32_t lastPollTime = 0;        ///< Timestamp of last poll
        uint16_t eventQueueDepth = 0;     ///< Current input event queue depth
        uint16_t messageQueueDepth = 0;   ///< Current control message queue depth
        bool inputLayerError = false;     ///< Input layer error flag
        bool gestureDetectorError = false; ///< Gesture detector error flag
        
        /**
         * @brief Reset all status counters
         */
        void reset() {
            eventsProcessed = 0;
            messagesGenerated = 0;
            pollCount = 0;
            lastPollTime = 0;
            eventQueueDepth = 0;
            messageQueueDepth = 0;
            inputLayerError = false;
            gestureDetectorError = false;
        }
    };
    
    /**
     * @brief Constructor with dependencies
     * 
     * @param deps All required dependencies for operation
     * @param config Input system configuration
     */
    InputController(Dependencies deps, const InputSystemConfiguration& config);
    
    /**
     * @brief Virtual destructor ensures proper cleanup
     */
    ~InputController();
    
    /**
     * @brief Initialize the input controller
     * 
     * Initializes all components and prepares for input processing.
     * Must be called before polling or message retrieval.
     * 
     * @return true if initialization succeeded
     */
    bool initialize();
    
    /**
     * @brief Shutdown the input controller
     * 
     * Cleanly shuts down all components and releases resources.
     */
    void shutdown();
    
    /**
     * @brief Poll input devices and process state changes
     * 
     * Performs one complete input processing cycle using modern state-based approach:
     * 1. Poll input layer for state updates
     * 2. Get current authoritative input state
     * 3. Process state transitions through InputStateProcessor
     * 4. Queue resulting control messages
     * 5. Falls back to legacy event processing if needed
     * 
     * This method should be called regularly (10-20ms intervals)
     * to maintain responsive input processing.
     * 
     * @return true if processing was successful
     */
    bool poll();
    
    /**
     * @brief Get next available control message
     * 
     * Retrieves the next control message from the processed queue.
     * Messages are returned in chronological order.
     * 
     * @param message Output parameter to receive the message
     * @return true if a message was retrieved
     */
    bool getNextMessage(ControlMessage::Message& message);
    
    /**
     * @brief Check if control messages are available
     * 
     * @return true if at least one message is queued
     */
    bool hasMessages() const;
    
    /**
     * @brief Get current button states
     * 
     * Returns the current state of all input buttons as tracked
     * by the gesture detection system.
     * 
     * @param buttonStates Array to receive button states
     * @param maxButtons Size of the buttonStates array  
     * @return Number of button states returned
     */
    uint8_t getCurrentButtonStates(bool* buttonStates, uint8_t maxButtons) const;
    
    /**
     * @brief Check if currently in parameter lock mode
     * 
     * @return true if gesture detector is in parameter lock mode
     */
    bool isInParameterLockMode() const;
    
    /**
     * @brief Get operational status
     * 
     * @return Current status with performance statistics
     */
    Status getStatus() const;
    
    /**
     * @brief Update input system configuration
     * 
     * Applies new configuration to all components. Some configuration
     * changes may require reinitialization.
     * 
     * @param config New configuration to apply
     * @return true if configuration was applied successfully
     */
    bool setConfiguration(const InputSystemConfiguration& config);
    
    /**
     * @brief Get current configuration
     * 
     * @return Current input system configuration
     */
    InputSystemConfiguration getConfiguration() const;
    
    /**
     * @brief Clear all queued messages
     * 
     * Removes all pending control messages from the queue.
     * Useful for mode changes or error recovery.
     * 
     * @return Number of messages cleared
     */
    uint16_t clearMessages();
    
    /**
     * @brief Reset all input system state
     * 
     * Resets gesture detection state, clears queues, and
     * reinitializes all components. Useful for error recovery
     * or mode transitions.
     */
    void reset();

private:
    // Dependencies
    Dependencies dependencies_;
    InputSystemConfiguration config_;
    
    // State management
    bool initialized_ = false;
    mutable Status status_;
    InputState currentInputState_;  ///< Current bitwise input state
    
    // Message queue for control messages
    std::queue<ControlMessage::Message> messageQueue_;
    
    /**
     * @brief Process state-based input using modern state system
     * 
     * Primary processing path: Gets current state from input layer
     * and processes state transitions through InputStateProcessor.
     * 
     * @return Number of control messages generated
     */
    uint16_t processStateBasedInput();
    
    /**
     * @brief Process SHIFT-based gestures for parameter locks
     * 
     * Handles SHIFT + button combinations through ShiftBasedGestureDetector
     * for parameter lock functionality. Works alongside state-based processing.
     * 
     * @return Number of control messages generated
     */
    uint16_t processShiftGestures();
    
    /**
     * @brief Legacy fallback: Process raw input events through gesture detection
     * 
     * Fallback processing path for backward compatibility.
     * Takes input events from the input layer and processes them
     * through legacy gesture detection to generate control messages.
     * 
     * @return Number of control messages generated
     */
    uint16_t processInputEventsLegacy();
    
    /**
     * @brief Update timing for both modern and legacy systems
     * 
     * Updates timing state and processes any time-based events
     * like hold detection or timeouts. Works with both modern
     * InputStateProcessor and legacy GestureDetector.
     * 
     * @return Number of control messages generated from timing updates
     */
    uint16_t updateTimingState();
    
    /**
     * @brief Add control message to output queue
     * 
     * @param message Control message to queue
     * @return true if message was queued successfully
     */
    bool enqueueMessage(const ControlMessage::Message& message);
    
    /**
     * @brief Update performance statistics
     */
    void updateStatistics() const;
    
    /**
     * @brief Log debug message if available
     * 
     * @param message Message to log
     */
    void debugLog(const std::string& message) const;
};

#endif // INPUT_CONTROLLER_H