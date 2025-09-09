#ifndef INPUT_STATE_ENCODER_H
#define INPUT_STATE_ENCODER_H

#include <cstdint>
#include <string>
#include "InputStateProcessor.h"
#include "InputEvent.h"
#include "IClock.h"
#include "IDebugOutput.h"

/**
 * @brief Bridge component between InputEvent streams and InputState transitions
 * 
 * This component is the critical bridge that converts platform-agnostic InputEvent
 * streams into bitwise-encoded InputState representations. It enables the integration
 * of the bitwise state management system into the existing input pipeline.
 * 
 * Architecture Integration:
 * Before: IInputLayer → InputEvent → IGestureDetector → ControlMessage
 * After:  IInputLayer → InputEvent → InputStateEncoder → InputState → InputStateProcessor → ControlMessage
 * 
 * Key Responsibilities:
 * - Convert InputEvent timing to normalized timing buckets (0-255, ~20ms units)
 * - Handle parameter lock entry detection based on hold thresholds
 * - Maintain bitwise button state encoding
 * - Bridge timing-based gestures to state transitions
 * 
 * Design Principles:
 * - Real-time safe (no dynamic allocation)
 * - Thread-safe operations
 * - Deterministic state transitions
 * - Zero-overhead abstractions
 */
class InputStateEncoder {
public:
    /**
     * @brief Dependency injection configuration
     * 
     * Following the project's RAII and dependency injection patterns,
     * all external dependencies are explicitly provided via constructor.
     */
    struct Dependencies {
        IClock* clock = nullptr;
        IDebugOutput* debugOutput = nullptr;
    };
    
    /**
     * @brief Construct InputStateEncoder with dependencies
     * 
     * @param deps Dependencies for clock and debug output
     */
    explicit InputStateEncoder(Dependencies deps);
    
    /**
     * @brief Convert InputEvent stream to InputState transitions
     * 
     * Core conversion function that translates platform-agnostic InputEvents
     * into bitwise-encoded InputState transitions. Handles all event types
     * and maintains proper state consistency.
     * 
     * State Transition Logic:
     * - BUTTON_PRESS: Set button bit, reset timing bucket
     * - BUTTON_RELEASE: Clear button bit, encode press duration, detect parameter lock
     * - Other events: Pass through with minimal state changes
     * 
     * @param event The input event to process
     * @param previousState Current state to transition from
     * @return New InputState representing the transition
     */
    InputState processInputEvent(const InputEvent& event, const InputState& previousState);
    
    /**
     * @brief Handle timing-based state updates for ongoing button presses
     * 
     * Updates state for timing-dependent features like hold detection.
     * For now, this is a pass-through implementation that can be enhanced
     * for real-time hold detection in future iterations.
     * 
     * @param currentTime Current system time in milliseconds
     * @param currentState Current input state
     * @return Updated InputState with timing adjustments
     */
    InputState updateTiming(uint32_t currentTime, const InputState& currentState);
    
    /**
     * @brief Configure hold threshold for parameter lock detection
     * 
     * @param ms Hold time in milliseconds required to trigger parameter lock
     */
    void setHoldThreshold(uint32_t ms) { holdThresholdMs_ = ms; }
    
    /**
     * @brief Get current hold threshold
     * 
     * @return Hold threshold in milliseconds
     */
    uint32_t getHoldThreshold() const { return holdThresholdMs_; }
    
private:
    Dependencies dependencies_;
    uint32_t holdThresholdMs_ = 500;  ///< Default 500ms hold threshold for parameter lock
    
    /**
     * @brief Convert press duration to normalized timing bucket
     * 
     * Converts millisecond duration to 8-bit timing bucket (0-255).
     * Each unit represents approximately 20ms, providing ~5.1 second
     * maximum timing resolution with 20ms precision.
     * 
     * Formula: bucket = min(255, durationMs / 20)
     * 
     * @param durationMs Duration in milliseconds
     * @return Timing bucket value (0-255)
     */
    uint8_t calculateTimingBucket(uint32_t durationMs) const;
    
    /**
     * @brief Determine if press duration qualifies for parameter lock entry
     * 
     * Compares press duration against configured hold threshold to determine
     * if parameter lock mode should be activated.
     * 
     * @param pressDuration Duration of button press in milliseconds
     * @return true if parameter lock should be activated
     */
    bool shouldEnterParameterLock(uint32_t pressDuration) const;
    
    /**
     * @brief Debug logging helper
     * 
     * Conditional debug output that only logs if debug output dependency
     * is provided. Follows the project's error handling philosophy.
     * 
     * @param message Debug message to log
     */
    void debugLog(const std::string& message) const;
};

#endif // INPUT_STATE_ENCODER_H