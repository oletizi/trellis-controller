#ifndef ENHANCED_STEP_SEQUENCER_H
#define ENHANCED_STEP_SEQUENCER_H

#include "StepSequencer.h"
#include "ParameterLockPool.h"
#include "ParameterLockTypes.h"
#include "ParameterEngine.h"
#include "SequencerStateManager.h"
#include "AdaptiveButtonTracker.h"
#include "ControlGrid.h"
#include "IDisplay.h"
#include "IInput.h"

/**
 * Enhanced step sequencer with parameter lock support.
 * Extends the base StepSequencer with Elektron-style parameter locks.
 */
class EnhancedStepSequencer : public StepSequencer {
public:
    /**
     * Extended dependencies including parameter lock components
     */
    struct ExtendedDependencies : public StepSequencer::Dependencies {
        IDisplay* display = nullptr;
        IInput* input = nullptr;
    };
    
    /**
     * Parameter lock statistics for monitoring
     */
    struct ParameterLockStats {
        size_t totalLocks;            // Total parameter locks in use
        size_t locksPerTrack[4];      // Locks per track
        float poolUtilization;        // Lock pool utilization
        uint32_t lockEditTime;        // Time spent in lock edit mode (ms)
        uint32_t successfulEdits;     // Number of successful parameter edits
        bool memoryPressure;          // Memory pressure detected
        
        ParameterLockStats() 
            : totalLocks(0), poolUtilization(0.0f), lockEditTime(0)
            , successfulEdits(0), memoryPressure(false) {
            for (int i = 0; i < 4; ++i) locksPerTrack[i] = 0;
        }
    };

public:
    /**
     * Constructor with extended dependencies
     * @param deps Extended dependencies including display and input
     */
    explicit EnhancedStepSequencer(const ExtendedDependencies& deps);
    
    /**
     * Destructor
     */
    virtual ~EnhancedStepSequencer();
    
    /**
     * Update sequencer state (call from main loop)
     * Handles button input, parameter lock mode, and sequencing
     */
    void update() override;
    
    /**
     * Trigger a step with parameter locks applied
     * @param track Track index (0-3)
     * @param step Step index (0-7)
     */
    void triggerStep(uint8_t track, uint8_t step) override;
    
    /**
     * Handle button press
     * @param button Button index (0-31)
     * @param pressed Whether button is pressed
     * @param currentTime Current time in milliseconds
     */
    void handleButton(uint8_t button, bool pressed, uint32_t currentTime);
    
    /**
     * Enter parameter lock mode for a step
     * @param track Track index (0-3)
     * @param step Step index (0-7)
     * @return true if successfully entered parameter lock mode
     */
    bool enterParameterLockMode(uint8_t track, uint8_t step);
    
    /**
     * Exit parameter lock mode
     * @return true if successfully exited
     */
    bool exitParameterLockMode();
    
    /**
     * Check if in parameter lock mode
     * @return true if in parameter lock mode
     */
    bool isInParameterLockMode() const;
    
    /**
     * Adjust parameter for current held step
     * @param paramType Parameter type to adjust
     * @param delta Adjustment amount (-1 or +1)
     * @return true if parameter was adjusted
     */
    bool adjustParameter(ParameterLockPool::ParameterType paramType, int8_t delta);
    
    /**
     * Clear all parameter locks for a step
     * @param track Track index (0-3)
     * @param step Step index (0-7)
     */
    void clearStepLocks(uint8_t track, uint8_t step);
    
    /**
     * Clear all parameter locks for a track
     * @param track Track index (0-3)
     */
    void clearTrackLocks(uint8_t track);
    
    /**
     * Clear all parameter locks
     */
    void clearAllLocks();
    
    /**
     * Copy parameter locks from one step to another
     * @param srcTrack Source track
     * @param srcStep Source step
     * @param dstTrack Destination track
     * @param dstStep Destination step
     * @return true if copy was successful
     */
    bool copyStepLocks(uint8_t srcTrack, uint8_t srcStep, 
                      uint8_t dstTrack, uint8_t dstStep);
    
    /**
     * Get parameter lock statistics
     * @return Parameter lock statistics
     */
    ParameterLockStats getParameterLockStats() const;
    
    /**
     * Get memory usage statistics
     * @return Memory statistics
     */
    MemoryStats getMemoryStats() const;
    
    /**
     * Get performance statistics
     * @return Performance statistics
     */
    PerformanceStats getPerformanceStats() const;
    
    /**
     * Update display with current state
     */
    void updateDisplay();
    
    /**
     * Set hand preference for control grid
     * @param preference Hand preference
     */
    void setHandPreference(ControlGrid::HandPreference preference);
    
    /**
     * Enable/disable adaptive button timing
     * @param enabled Whether to enable learning
     */
    void setAdaptiveLearning(bool enabled);
    
    /**
     * Set button hold threshold
     * @param thresholdMs Hold threshold in milliseconds
     */
    void setHoldThreshold(uint32_t thresholdMs);
    
    /**
     * Get current sequencer mode
     * @return Current mode
     */
    SequencerStateManager::Mode getCurrentMode() const;
    
    /**
     * Save pattern with parameter locks
     * @param patternIndex Pattern slot index
     * @return true if save was successful
     */
    bool savePattern(uint8_t patternIndex);
    
    /**
     * Load pattern with parameter locks
     * @param patternIndex Pattern slot index
     * @return true if load was successful
     */
    bool loadPattern(uint8_t patternIndex);

protected:
    /**
     * Prepare parameters for next step
     * @param nextStep Next step index
     */
    void prepareNextStep(uint8_t nextStep) override;
    
    /**
     * Handle parameter lock mode input
     * @param button Button that was pressed
     * @param pressed Whether button is pressed
     */
    void handleParameterLockInput(uint8_t button, bool pressed);
    
    /**
     * Update performance monitoring
     */
    void updatePerformanceMonitoring();
    
    /**
     * Apply parameter locks to MIDI output
     * @param track Track index
     * @param params Calculated parameters
     */
    void sendMidiWithParameters(uint8_t track, const CalculatedParameters& params);

private:
    // Parameter lock components
    ParameterLockPool lockPool_;
    ParameterEngine paramEngine_;
    SequencerStateManager stateManager_;
    AdaptiveButtonTracker buttonTracker_;
    ControlGrid controlGrid_;
    
    // Pattern data with parameter lock support
    PatternData patternData_;
    TrackDefaults trackDefaults_[MAX_TRACKS];
    
    // Display and input interfaces
    IDisplay* display_;
    IInput* input_;
    
    // State tracking
    uint32_t lastUpdateTime_;
    uint32_t parameterLockStartTime_;
    uint8_t copiedStep_;
    uint8_t copiedTrack_;
    bool hasCopiedData_;
    
    // Performance monitoring
    PerformanceStats perfStats_;
    uint32_t lastPerfUpdate_;
    
    /**
     * Initialize pattern data
     */
    void initializePatternData();
    
    /**
     * Update button states from input
     * @param currentTime Current time in milliseconds
     */
    void updateButtonStates(uint32_t currentTime);
    
    /**
     * Handle normal mode button press
     * @param button Button index
     * @param pressed Whether pressed
     */
    void handleNormalModeButton(uint8_t button, bool pressed);
    
    /**
     * Get button to track/step mapping
     * @param button Button index
     * @param track Output track index
     * @param step Output step index
     * @return true if button maps to a valid track/step
     */
    bool buttonToTrackStep(uint8_t button, uint8_t& track, uint8_t& step) const;
    
    /**
     * Update visual feedback
     */
    void updateVisualFeedback();
    
    /**
     * Show parameter lock mode visuals
     */
    void showParameterLockVisuals();
    
    /**
     * Show normal sequencer visuals
     */
    void showNormalVisuals();
    
    /**
     * Validate system state
     * @return true if state is valid
     */
    bool validateState() const;
};

#endif // ENHANCED_STEP_SEQUENCER_H