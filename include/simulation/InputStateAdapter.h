#ifndef INPUT_STATE_ADAPTER_H
#define INPUT_STATE_ADAPTER_H

#include "../core/InputStateProcessor.h"
#include "CursesInputLayer.h"
#include "IClock.h"

/**
 * @brief Adapter that converts CursesInputLayer events to InputState encoding
 * 
 * This adapter performs the critical role of semantic interpretation for
 * platform-specific input. It takes raw keyboard events from CursesInputLayer
 * (which only translates keys without interpretation) and applies the
 * simulation-specific semantics (uppercase = press, lowercase = release).
 * 
 * This maintains proper architectural separation:
 * - CursesInputLayer: Pure platform translation (stateless)
 * - InputStateAdapter: Semantic interpretation and state management
 * - InputStateProcessor: Gesture detection and control message generation
 * 
 * The adapter acts as a bridge:
 * CursesInputLayer → Raw Events → InputStateAdapter → InputState → ControlMessages
 */
class InputStateAdapter {
public:
    struct Dependencies {
        CursesInputLayer* cursesLayer = nullptr;
        IClock* clock = nullptr;
        IDebugOutput* debugOutput = nullptr;
    };
    
    explicit InputStateAdapter(Dependencies deps);
    ~InputStateAdapter() = default;
    
    /**
     * @brief Poll for state changes and generate control messages
     * 
     * This replaces the InputController pattern with direct state-based processing.
     * 
     * @return Vector of control messages generated from state transitions
     */
    std::vector<ControlMessage::Message> poll();
    
    /**
     * @brief Get current input state for debugging
     */
    InputState getCurrentState() const { return currentState_; }
    
    /**
     * @brief Configure hold threshold
     */
    void setHoldThreshold(uint32_t ms) { processor_.setHoldThreshold(ms); }
    
private:
    Dependencies dependencies_;
    InputStateProcessor processor_;
    InputState previousState_;
    InputState currentState_;
    
    /**
     * @brief Build InputState from CursesInputLayer events
     */
    InputState buildStateFromEvents();
    
    /**
     * @brief Process a single input event into the state
     * 
     * Handles both legacy BUTTON_PRESS/RELEASE events and new raw keyboard
     * events (SYSTEM_EVENT with keyboard data). Performs semantic interpretation
     * of uppercase/lowercase keys to determine press/release behavior.
     */
    void processInputEvent(const InputEvent& event, InputState& state);
    
    /**
     * @brief Process timing information for hold detection
     */
    uint8_t calculateTimingBucket(uint32_t durationMs) const;
    
    /**
     * @brief Determine if we should enter parameter lock mode
     */
    bool shouldEnterParameterLock(uint8_t buttonId, uint32_t durationMs) const;
    
    void debugLog(const std::string& message) const;
};

#endif // INPUT_STATE_ADAPTER_H