#ifndef ADAPTIVE_BUTTON_TRACKER_H
#define ADAPTIVE_BUTTON_TRACKER_H

#include <stdint.h>

/**
 * Adaptive button hold detection system.
 * Learns user behavior and adjusts hold timing thresholds for better UX.
 * Integrates with existing button scanning infrastructure.
 */
class AdaptiveButtonTracker {
public:
    /**
     * Hold timing profile - can be customized per user
     */
    struct HoldProfile {
        uint32_t threshold;           // Current hold threshold (ms)
        uint32_t minThreshold;        // Minimum allowed threshold (ms)
        uint32_t maxThreshold;        // Maximum allowed threshold (ms)
        float adaptationRate;         // Learning rate (0.0-1.0)
        bool learningEnabled;         // Whether to adapt thresholds
        
        // Default constructor with conservative settings
        HoldProfile()
            : threshold(500)          // 500ms default
            , minThreshold(300)       // Minimum 300ms (fast users)
            , maxThreshold(700)       // Maximum 700ms (careful users)
            , adaptationRate(0.1f)    // Slow learning rate
            , learningEnabled(true) {}
        
        // Validate profile parameters
        bool isValid() const {
            return minThreshold <= threshold && 
                   threshold <= maxThreshold &&
                   adaptationRate >= 0.0f && 
                   adaptationRate <= 1.0f;
        }
    };
    
    /**
     * Button state tracking
     */
    struct ButtonState {
        bool pressed;                 // Currently pressed
        bool wasPressed;              // Was pressed this update
        bool wasReleased;             // Was released this update
        uint32_t pressTime;           // When button was pressed (ms)
        uint32_t releaseTime;         // When button was released (ms) 
        bool isHeld;                  // Button is being held
        bool holdProcessed;           // Hold event has been processed
        uint32_t holdDuration;        // How long button was held (ms)
        
        // Default constructor
        ButtonState()
            : pressed(false), wasPressed(false), wasReleased(false)
            , pressTime(0), releaseTime(0), isHeld(false)
            , holdProcessed(false), holdDuration(0) {}
        
        // Reset state
        void reset() {
            wasPressed = false;
            wasReleased = false;
            holdProcessed = false;
        }
    };
    
    /**
     * Learning statistics
     */
    struct LearningStats {
        uint32_t totalActivations;       // Total successful hold activations
        uint32_t falseActivations;       // False positive activations
        uint32_t missedActivations;      // User intended hold but too short
        float successRate;               // Success rate (0.0-1.0)
        uint32_t avgHoldDuration;        // Average hold duration (ms)
        uint32_t samples;                // Number of samples collected
        bool hasData;                    // Whether we have enough data
        
        LearningStats()
            : totalActivations(0), falseActivations(0), missedActivations(0)
            , successRate(0.0f), avgHoldDuration(0), samples(0), hasData(false) {}
    };

public:
    static constexpr uint8_t MAX_BUTTONS = 32;  // NeoTrellis M4 has 32 buttons
    
    /**
     * Constructor
     */
    AdaptiveButtonTracker();
    
    /**
     * Update button states from hardware scan
     * @param buttonMask Current button state (1 bit per button)
     * @param currentTime Current time in milliseconds
     */
    void update(uint32_t buttonMask, uint32_t currentTime);
    
    /**
     * Check if button was pressed this update cycle
     * @param button Button index (0-31)
     * @return true if button was pressed
     */
    bool wasPressed(uint8_t button);
    
    /**
     * Check if button was released this update cycle
     * @param button Button index (0-31)  
     * @return true if button was released
     */
    bool wasReleased(uint8_t button);
    
    /**
     * Check if button is currently being held
     * @param button Button index (0-31)
     * @return true if button is held beyond threshold
     */
    bool isHeld(uint8_t button);
    
    /**
     * Check if button is currently pressed (immediate state)
     * @param button Button index (0-31)
     * @return true if button is pressed
     */
    bool isPressed(uint8_t button);
    
    /**
     * Get the first held button (for single-hold detection)
     * @return Button index or 0xFF if none held
     */
    uint8_t getHeldButton();
    
    /**
     * Get how long a button has been held
     * @param button Button index (0-31)
     * @param currentTime Current time in milliseconds
     * @return Hold duration in milliseconds (0 if not held)
     */
    uint32_t getHoldDuration(uint8_t button, uint32_t currentTime);
    
    /**
     * Record successful hold activation for learning
     * @param button Button that was successfully activated
     * @param holdDuration How long it was held
     */
    void recordSuccessfulActivation(uint8_t button, uint32_t holdDuration);
    
    /**
     * Record false activation (user didn't intend to hold)
     * @param button Button that had false activation
     * @param holdDuration How long it was held
     */
    void recordFalseActivation(uint8_t button, uint32_t holdDuration);
    
    /**
     * Record missed activation (user intended hold but was too short)
     * @param button Button that was missed
     * @param holdDuration How long it was held
     */
    void recordMissedActivation(uint8_t button, uint32_t holdDuration);
    
    /**
     * Update hold threshold based on learning data
     * Called automatically but can be called manually
     */
    void updateThreshold();
    
    /**
     * Set hold profile
     * @param profile New profile settings
     */
    void setProfile(const HoldProfile& profile);
    
    /**
     * Get current hold profile
     * @return Current profile settings
     */
    const HoldProfile& getProfile() const { return profile_; }
    
    /**
     * Get learning statistics
     * @return Learning statistics structure
     */
    const LearningStats& getLearningStats() const { return stats_; }
    
    /**
     * Reset learning statistics
     */
    void resetLearning();
    
    /**
     * Enable/disable learning
     * @param enabled Whether to adapt thresholds automatically
     */
    void setLearningEnabled(bool enabled) {
        profile_.learningEnabled = enabled;
    }
    
    /**
     * Check if button state is valid
     * @param button Button index (0-31)
     * @return true if button index is valid
     */
    bool isValidButton(uint8_t button) const {
        return button < MAX_BUTTONS;
    }
    
    /**
     * Get button state for debugging
     * @param button Button index (0-31)
     * @return Button state structure (undefined if invalid button)
     */
    const ButtonState& getButtonState(uint8_t button) const;
    
    /**
     * Force button state update (for testing)
     * @param button Button index (0-31)
     * @param pressed New pressed state
     * @param currentTime Current time
     */
    void forceButtonState(uint8_t button, bool pressed, uint32_t currentTime);
    
    /**
     * Mark button hold as processed (to prevent multiple hold events)
     * @param button Button index (0-31)
     */
    void markHoldProcessed(uint8_t button);

private:
    ButtonState states_[MAX_BUTTONS];      // Button state tracking
    HoldProfile profile_;                  // Hold timing profile
    LearningStats stats_;                  // Learning statistics
    uint32_t lastUpdateTime_;              // Last update time
    
    // Learning data collection
    uint32_t holdDurations_[16];           // Recent hold durations (circular buffer)
    uint8_t holdDurationIndex_;            // Current index in buffer
    uint8_t holdSampleCount_;              // Number of samples in buffer
    
    /**
     * Update individual button state
     * @param button Button index
     * @param pressed Current pressed state
     * @param currentTime Current time
     */
    void updateButtonState(uint8_t button, bool pressed, uint32_t currentTime);
    
    /**
     * Check if button should be considered held
     * @param button Button index
     * @param currentTime Current time
     * @return true if button meets hold criteria
     */
    bool shouldBeHeld(uint8_t button, uint32_t currentTime);
    
    /**
     * Add hold duration sample for learning
     * @param duration Hold duration in milliseconds
     */
    void addHoldSample(uint32_t duration);
    
    /**
     * Calculate optimal threshold from learning data
     * @return Suggested threshold in milliseconds
     */
    uint32_t calculateOptimalThreshold() const;
    
    /**
     * Update learning statistics
     */
    void updateStats();
    
    /**
     * Validate internal state (for debugging)
     * @return true if state is consistent
     */
    bool validateState() const;
};

#endif // ADAPTIVE_BUTTON_TRACKER_H