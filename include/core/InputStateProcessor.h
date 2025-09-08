#ifndef INPUT_STATE_PROCESSOR_H
#define INPUT_STATE_PROCESSOR_H

#include <cstdint>
#include <vector>
#include "ControlMessage.h"
#include "IClock.h"
#include "IDebugOutput.h"

/**
 * @brief Unified input state representation using bitwise encoding
 * 
 * This solves the distributed state management problem by encoding
 * all input state into a single 64-bit value, enabling deterministic
 * state transitions and eliminating race conditions.
 */
struct InputState {
    union {
        uint64_t raw;
        struct {
            // Bits 0-31: Current button states (1=pressed, 0=released)
            uint32_t buttonStates;
            
            // Bits 32-39: Modifier flags
            // [7]=paramLockActive, [6]=shift, [5]=uppercase, [4-0]=reserved
            uint8_t modifiers;
            
            // Bits 40-47: Context information
            // [7-2]=lockButtonId (0-31), [1-0]=reserved
            uint8_t context;
            
            // Bits 48-55: Timing bucket (0-255, each unit = ~20ms)
            uint8_t timingInfo;
            
            // Bits 56-63: Reserved for future use
            uint8_t reserved;
        };
    };
    
    // Modifier flag masks
    static constexpr uint8_t PARAM_LOCK_ACTIVE = 0x80;  // Bit 7
    static constexpr uint8_t SHIFT_MODIFIER = 0x40;      // Bit 6
    static constexpr uint8_t UPPERCASE_MODIFIER = 0x20;  // Bit 5
    
    // Constructor for clean initialization
    InputState(uint32_t buttons = 0, bool paramLock = false, 
               uint8_t lockButton = 0, uint8_t timing = 0) 
        : buttonStates(buttons), 
          modifiers(paramLock ? PARAM_LOCK_ACTIVE : 0), 
          context(lockButton << 2), 
          timingInfo(timing), 
          reserved(0) {}
    
    // Helper methods
    bool isParameterLockActive() const { return modifiers & PARAM_LOCK_ACTIVE; }
    bool hasShiftModifier() const { return modifiers & SHIFT_MODIFIER; }
    bool hasUppercaseModifier() const { return modifiers & UPPERCASE_MODIFIER; }
    uint8_t getLockButtonId() const { return (context >> 2) & 0x3F; }
    
    void setParameterLockActive(bool active) {
        if (active) modifiers |= PARAM_LOCK_ACTIVE;
        else modifiers &= ~PARAM_LOCK_ACTIVE;
    }
    
    void setLockButtonId(uint8_t buttonId) {
        context = (context & 0x03) | ((buttonId & 0x3F) << 2);
    }
    
    bool isButtonPressed(uint8_t buttonId) const {
        if (buttonId >= 32) return false;
        return (buttonStates & (1U << buttonId)) != 0;
    }
    
    void setButtonState(uint8_t buttonId, bool pressed) {
        if (buttonId >= 32) return;
        if (pressed) buttonStates |= (1U << buttonId);
        else buttonStates &= ~(1U << buttonId);
    }
};

/**
 * @brief Translates input state to control messages
 * 
 * Single source of truth for all input state transitions.
 * Pure functional approach: (currentState, previousState) → ControlMessages
 */
class InputStateProcessor {
public:
    struct Dependencies {
        IClock* clock = nullptr;
        IDebugOutput* debugOutput = nullptr;
    };
    
    explicit InputStateProcessor(Dependencies deps);
    ~InputStateProcessor() = default;
    
    /**
     * @brief Translate state transition to control messages
     * 
     * Pure function that examines state transition and generates
     * appropriate control messages. All state transition logic
     * is centralized here for complete visibility.
     * 
     * @param currentState Current input state
     * @param previousState Previous input state
     * @param timestamp Current timestamp
     * @return Vector of control messages to process
     */
    std::vector<ControlMessage::Message> translateState(
        const InputState& currentState, 
        const InputState& previousState,
        uint32_t timestamp);
    
    /**
     * @brief Configure timing thresholds
     */
    void setHoldThreshold(uint32_t ms) { holdThresholdMs_ = ms; }
    uint32_t getHoldThreshold() const { return holdThresholdMs_; }
    
private:
    Dependencies dependencies_;
    uint32_t holdThresholdMs_ = 500;  // Default 500ms for parameter lock
    
    // State transition detection helpers
    bool isParameterLockExit(const InputState& current, const InputState& previous) const;
    bool isParameterLockEntry(const InputState& current, const InputState& previous) const;
    bool isStepToggle(const InputState& current, const InputState& previous) const;
    bool isParameterAdjustment(const InputState& current, const InputState& previous) const;
    
    // Utility functions
    uint32_t findChangedButtons(const InputState& current, const InputState& previous) const;
    uint8_t findFirstChangedButton(uint32_t changedMask) const;
    void getTrackStep(uint8_t buttonId, uint8_t& track, uint8_t& step) const;
    bool isButtonPress(uint8_t buttonId, const InputState& current, const InputState& previous) const;
    bool isButtonRelease(uint8_t buttonId, const InputState& current, const InputState& previous) const;
    
    // Debug helpers
    void debugLog(const std::string& message) const;
};

#endif // INPUT_STATE_PROCESSOR_H