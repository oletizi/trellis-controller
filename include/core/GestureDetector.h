#ifndef GESTURE_DETECTOR_H
#define GESTURE_DETECTOR_H

#include "IGestureDetector.h"
#include "InputSystemConfiguration.h"
#include "IClock.h"
#include "IDebugOutput.h"
#include <array>
#include <vector>

/**
 * @file GestureDetector.h
 * @brief Concrete implementation of gesture detection for trellis sequencer
 * 
 * Implements sophisticated gesture recognition specifically designed for
 * the step sequencer interface. Handles the complex state management
 * required for parameter locks, step toggling, and parameter adjustment.
 * 
 * Key Features:
 * - Hold-based parameter lock entry (500ms+ hold)
 * - Tap-based step toggling (quick press/release)
 * - Parameter adjustment in parameter lock mode
 * - Automatic parameter lock exit on inactivity
 * - Configurable timing thresholds
 * - 4x8 grid button mapping with track/step semantics
 * 
 * Button Layout Semantics:
 * - Buttons 0-31 represent steps in 4 tracks (8 steps per track)
 * - Row 0-3 = Track 0-3, Columns 0-7 = Step 0-7
 * - Hold any step button to enter parameter lock for that step
 * - In parameter lock mode, other buttons adjust parameters
 */
class GestureDetector : public IGestureDetector {
public:
    /**
     * @brief Constructor with configuration and dependencies
     * 
     * @param config Input system configuration with timing parameters
     * @param clock Clock interface for timing operations
     * @param debug Optional debug output interface
     */
    GestureDetector(const InputSystemConfiguration& config,
                    IClock* clock,
                    IDebugOutput* debug = nullptr);
    
    /**
     * @brief Virtual destructor
     */
    ~GestureDetector() override = default;
    
    // IGestureDetector interface
    uint8_t processInputEvent(const InputEvent& inputEvent, 
                             std::vector<ControlMessage::Message>& controlMessages) override;
    
    uint8_t updateTiming(uint32_t currentTime,
                        std::vector<ControlMessage::Message>& controlMessages) override;
    
    void reset() override;
    
    uint8_t getCurrentButtonStates(bool* buttonStates, uint8_t maxButtons) const override;
    
    bool isInParameterLockMode() const override;
    
    /**
     * @brief Update configuration at runtime
     * 
     * @param config New configuration to apply
     */
    void setConfiguration(const InputSystemConfiguration& config);
    
    /**
     * @brief Get current configuration
     * 
     * @return Current input system configuration
     */
    InputSystemConfiguration getConfiguration() const;

private:
    /**
     * @brief Button press tracking information
     */
    struct ButtonState {
        bool pressed = false;           ///< Current press state
        uint32_t pressStartTime = 0;   ///< When press began (for hold detection)
        uint32_t lastEventTime = 0;    ///< Last event timestamp for this button
        bool holdDetected = false;     ///< Whether hold threshold was reached
        bool holdMessageSent = false;  ///< Whether hold message was already generated
    };
    
    /**
     * @brief Parameter lock state information
     */
    struct ParameterLockState {
        bool active = false;           ///< Whether parameter lock is currently active
        uint8_t lockedTrack = 0;      ///< Track of the locked step
        uint8_t lockedStep = 0;       ///< Step index within track
        uint32_t lockStartTime = 0;   ///< When parameter lock was entered
        uint32_t lastActivityTime = 0; ///< Last time a parameter was adjusted
    };
    
    // Configuration and dependencies
    InputSystemConfiguration config_;
    IClock* clock_;
    IDebugOutput* debug_;
    
    // State tracking
    std::array<ButtonState, 32> buttonStates_;  // 4x8 button grid
    ParameterLockState paramLockState_;
    
    /**
     * @brief Process button press event
     * 
     * @param buttonId Button index (0-31)
     * @param timestamp When the press occurred
     * @param controlMessages Output vector for generated messages
     * @return Number of messages generated
     */
    uint8_t processButtonPress(uint8_t buttonId, uint32_t timestamp,
                              std::vector<ControlMessage::Message>& controlMessages);
    
    /**
     * @brief Process button release event
     * 
     * @param buttonId Button index (0-31)
     * @param timestamp When the release occurred
     * @param pressDuration How long the button was held
     * @param controlMessages Output vector for generated messages
     * @return Number of messages generated
     */
    uint8_t processButtonRelease(uint8_t buttonId, uint32_t timestamp, uint32_t pressDuration,
                                std::vector<ControlMessage::Message>& controlMessages);
    
    /**
     * @brief Check for hold detection on all pressed buttons
     * 
     * Called during timing updates to detect when buttons cross
     * the hold threshold and generate parameter lock entry.
     * 
     * @param currentTime Current timestamp
     * @param controlMessages Output vector for hold detection messages
     * @return Number of hold detection messages generated
     */
    uint8_t checkForHoldDetection(uint32_t currentTime,
                                 std::vector<ControlMessage::Message>& controlMessages);
    
    /**
     * @brief Check for parameter lock timeout
     * 
     * Automatically exits parameter lock mode if no activity occurs
     * within the configured timeout period.
     * 
     * @param currentTime Current timestamp
     * @param controlMessages Output vector for timeout messages
     * @return Number of timeout messages generated
     */
    uint8_t checkForParameterLockTimeout(uint32_t currentTime,
                                        std::vector<ControlMessage::Message>& controlMessages);
    
    /**
     * @brief Generate step toggle message
     * 
     * @param buttonId Button index (0-31)
     * @param timestamp Event timestamp
     * @return ControlMessage for step toggle
     */
    ControlMessage::Message createStepToggleMessage(uint8_t buttonId, uint32_t timestamp);
    
    /**
     * @brief Generate parameter lock entry message
     * 
     * @param buttonId Button index (0-31) of the step to lock
     * @param timestamp Event timestamp
     * @return ControlMessage for parameter lock entry
     */
    ControlMessage::Message createParameterLockEntryMessage(uint8_t buttonId, uint32_t timestamp);
    
    /**
     * @brief Generate parameter lock exit message
     * 
     * @param timestamp Event timestamp
     * @return ControlMessage for parameter lock exit
     */
    ControlMessage::Message createParameterLockExitMessage(uint32_t timestamp);
    
    /**
     * @brief Generate parameter adjustment message
     * 
     * @param buttonId Button index used for parameter adjustment
     * @param timestamp Event timestamp
     * @return ControlMessage for parameter adjustment
     */
    ControlMessage::Message createParameterAdjustMessage(uint8_t buttonId, uint32_t timestamp);
    
    /**
     * @brief Convert button index to track and step
     * 
     * @param buttonId Linear button index (0-31)
     * @param track Output parameter for track number (0-3)
     * @param step Output parameter for step number (0-7)
     */
    void buttonIndexToTrackStep(uint8_t buttonId, uint8_t& track, uint8_t& step) const;
    
    /**
     * @brief Convert track and step to button index
     * 
     * @param track Track number (0-3)
     * @param step Step number (0-7)
     * @return Linear button index (0-31)
     */
    uint8_t trackStepToButtonIndex(uint8_t track, uint8_t step) const;
    
    /**
     * @brief Map button to parameter type for adjustment
     * 
     * In parameter lock mode, different buttons adjust different parameters.
     * This mapping defines which button controls which parameter.
     * 
     * @param buttonId Button used for parameter adjustment
     * @return Parameter type identifier
     */
    uint8_t mapButtonToParameterType(uint8_t buttonId) const;
    
    /**
     * @brief Calculate parameter adjustment delta
     * 
     * @param buttonId Button used for adjustment
     * @return Signed adjustment value
     */
    int8_t calculateParameterDelta(uint8_t buttonId) const;
    
    /**
     * @brief Log debug message if debug output is available
     * 
     * @param message Message to log
     */
    void debugLog(const std::string& message) const;
};

#endif // GESTURE_DETECTOR_H