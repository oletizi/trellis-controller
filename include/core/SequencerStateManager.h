#ifndef SEQUENCER_STATE_MANAGER_H
#define SEQUENCER_STATE_MANAGER_H

#include <stdint.h>

/**
 * State management for the sequencer with parameter lock mode support.
 * Integrates with existing sequencer architecture while adding new modes.
 */
class SequencerStateManager {
public:
    /**
     * Sequencer operation modes
     */
    enum class Mode : uint8_t {
        NORMAL = 0,            // Standard step sequencer operation
        PARAMETER_LOCK,        // Parameter lock editing mode
        PATTERN_SELECT,        // Pattern selection mode (existing)
        SHIFT_CONTROL,         // Shift key mode (existing)
        SETTINGS,              // Settings/configuration mode
        MODE_COUNT             // Total number of modes
    };
    
    /**
     * Parameter lock mode context
     */
    struct ParameterLockContext {
        bool active;                    // Mode is currently active
        uint8_t heldStep;              // Which step is being held (0-7)
        uint8_t heldTrack;             // Which track the held step is on (0-3)
        uint32_t holdStartTime;        // When the hold began (milliseconds)
        uint8_t controlGridStart;      // First button of control grid (0 or 24)
        Mode previousMode;             // Mode to return to when exiting
        uint32_t timeoutMs;            // Auto-exit timeout (default 10 seconds)
        
        // Default constructor
        ParameterLockContext()
            : active(false)
            , heldStep(0xFF)
            , heldTrack(0xFF)
            , holdStartTime(0)
            , controlGridStart(0)
            , previousMode(Mode::NORMAL)
            , timeoutMs(10000) {}
        
        // Check if context is valid
        bool isValid() const {
            return heldStep < 8 && 
                   heldTrack < 4 && 
                   (controlGridStart == 0 || controlGridStart == 4);
        }
        
        // Get the button index of the held step
        uint8_t getHeldButtonIndex() const {
            if (!isValid()) return 0xFF;
            return heldTrack * 8 + heldStep;
        }
        
        // Calculate control grid based on held step position
        void calculateControlGrid() {
            if (!isValid()) return;
            
            // If held step is in columns 0-3 (buttons 0-3, 8-11, 16-19, 24-27)
            // then control grid is columns 4-7 (buttons 4-7, 12-15, 20-23, 28-31)
            // If held step is in columns 4-7, control grid is columns 0-3
            controlGridStart = (heldStep < 4) ? 4 : 0;
        }
    };
    
    /**
     * Mode transition result
     */
    enum class TransitionResult : uint8_t {
        SUCCESS = 0,           // Mode transition successful
        INVALID_MODE,          // Requested mode is invalid
        TRANSITION_BLOCKED,    // Transition blocked by current state
        TIMEOUT,               // Mode timed out
        ERROR                  // General error
    };
    
public:
    /**
     * Constructor
     */
    SequencerStateManager();
    
    /**
     * Update state manager - call regularly from main loop
     * @param currentTime Current time in milliseconds
     */
    void update(uint32_t currentTime);
    
    /**
     * Get current mode
     * @return Current sequencer mode
     */
    Mode getCurrentMode() const { return currentMode_; }
    
    /**
     * Get previous mode
     * @return Previous sequencer mode
     */
    Mode getPreviousMode() const { return previousMode_; }
    
    /**
     * Check if mode transition is allowed
     * @param newMode Mode to transition to
     * @return true if transition is allowed
     */
    bool canTransitionTo(Mode newMode) const;
    
    /**
     * Enter parameter lock mode for specific step
     * @param track Track index (0-3)
     * @param step Step index (0-7)
     * @return Transition result
     */
    TransitionResult enterParameterLockMode(uint8_t track, uint8_t step);
    
    /**
     * Exit parameter lock mode
     * @return Transition result
     */
    TransitionResult exitParameterLockMode();
    
    /**
     * Force exit from current mode to normal mode
     * Used for error recovery
     */
    void forceExitToNormal();
    
    /**
     * Generic mode transition
     * @param newMode Mode to transition to
     * @return Transition result
     */
    TransitionResult transitionToMode(Mode newMode);
    
    /**
     * Get parameter lock context (only valid when in parameter lock mode)
     * @return Reference to parameter lock context
     */
    const ParameterLockContext& getParameterLockContext() const {
        return parameterLockContext_;
    }
    
    /**
     * Check if currently in parameter lock mode
     * @return true if in parameter lock mode
     */
    bool isInParameterLockMode() const {
        return currentMode_ == Mode::PARAMETER_LOCK && parameterLockContext_.active;
    }
    
    /**
     * Check if a button is in the current control grid
     * @param buttonIndex Button index (0-31)
     * @return true if button is in control grid
     */
    bool isInControlGrid(uint8_t buttonIndex) const;
    
    /**
     * Get control grid mapping for current context
     * @param controlButtons Array to fill with control button indices [out]
     * @param maxButtons Maximum number of buttons to return
     * @return Number of control buttons returned
     */
    uint8_t getControlGridButtons(uint8_t* controlButtons, uint8_t maxButtons) const;
    
    /**
     * Set parameter lock timeout
     * @param timeoutMs Timeout in milliseconds (0 = no timeout)
     */
    void setParameterLockTimeout(uint32_t timeoutMs) {
        parameterLockContext_.timeoutMs = timeoutMs;
    }
    
    /**
     * Check if mode has timed out
     * @param currentTime Current time in milliseconds
     * @return true if current mode should timeout
     */
    bool hasTimedOut(uint32_t currentTime) const;
    
    /**
     * Get mode name as string (for debugging)
     * @param mode Mode to get name for
     * @return Mode name string
     */
    static const char* getModeName(Mode mode);
    
    /**
     * Validate state consistency (for debugging)
     * @return true if state is consistent
     */
    bool validateState() const;

private:
    Mode currentMode_;                          // Current operating mode
    Mode previousMode_;                         // Previous mode (for returning)
    uint32_t modeStartTime_;                   // When current mode started
    ParameterLockContext parameterLockContext_; // Parameter lock mode context
    
    // Mode transition matrix - defines allowed transitions
    static const bool transitionMatrix_[static_cast<uint8_t>(Mode::MODE_COUNT)]
                                       [static_cast<uint8_t>(Mode::MODE_COUNT)];
    
    /**
     * Internal mode transition logic
     * @param newMode Mode to transition to
     * @param forced Whether this is a forced transition (bypasses checks)
     * @return Transition result
     */
    TransitionResult internalTransition(Mode newMode, bool forced = false);
    
    /**
     * Enter mode - called when transitioning into a mode
     * @param mode Mode being entered
     */
    void onEnterMode(Mode mode);
    
    /**
     * Exit mode - called when transitioning out of a mode
     * @param mode Mode being exited
     */
    void onExitMode(Mode mode);
    
    /**
     * Validate parameter lock context
     * @return true if context is valid
     */
    bool validateParameterLockContext() const;
};

#endif // SEQUENCER_STATE_MANAGER_H