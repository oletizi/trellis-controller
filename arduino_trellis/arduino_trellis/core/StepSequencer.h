#ifndef STEP_SEQUENCER_H
#define STEP_SEQUENCER_H

#include <stdint.h>
#include "IClock.h"
#include "IMidiOutput.h"
#include "IMidiInput.h"
#include "IDisplay.h"
#include "IInput.h"
#include "ParameterLockPool.h"
#include "ParameterLockTypes.h"
#include "ParameterEngine.h"
#include "SequencerStateManager.h"
#include "AdaptiveButtonTracker.h"
#include "ControlGrid.h"

class StepSequencer {
public:
    static constexpr uint8_t MAX_TRACKS = 4;
    static constexpr uint8_t MAX_STEPS = 8;
    
    struct Dependencies {
        IClock* clock = nullptr;
        IMidiOutput* midiOutput = nullptr;
        IMidiInput* midiInput = nullptr;
        IDisplay* display = nullptr;
        IInput* input = nullptr;
    };
    
    StepSequencer();
    explicit StepSequencer(Dependencies deps);
    ~StepSequencer();
    
    void init(uint16_t bpm, uint8_t steps);
    void tick();
    void start();
    void stop();
    void reset();
    
    void toggleStep(uint8_t track, uint8_t step);
    bool isStepActive(uint8_t track, uint8_t step) const;
    
    void setTempo(uint16_t bpm);
    uint16_t getTempo() const { return bpm_; }
    
    uint8_t getCurrentStep() const { return currentStep_; }
    bool isPlaying() const { return playing_; }
    
    void setTrackVolume(uint8_t track, uint8_t volume);
    void setTrackMute(uint8_t track, bool mute);
    uint8_t getTrackVolume(uint8_t track) const;
    bool isTrackMuted(uint8_t track) const;
    
    void setTrackMidiNote(uint8_t track, uint8_t note);
    void setTrackMidiChannel(uint8_t track, uint8_t channel);
    uint8_t getTrackMidiNote(uint8_t track) const;
    uint8_t getTrackMidiChannel(uint8_t track) const;
    
    void setMidiSync(bool enabled);
    bool isMidiSync() const { return midiSyncEnabled_; }
    
    struct TrackTrigger {
        uint8_t track;
        uint8_t velocity;
        bool triggered;
    };
    
    TrackTrigger getTriggeredTracks();
    
    // Parameter Lock Interface
    void handleButton(uint8_t button, bool pressed, uint32_t currentTime);
    bool enterParameterLockMode(uint8_t track, uint8_t step);
    bool exitParameterLockMode();
    bool isInParameterLockMode() const;
    bool adjustParameter(ParameterLockPool::ParameterType paramType, int8_t delta);
    void clearStepLocks(uint8_t track, uint8_t step);
    void clearAllLocks();
    void updateDisplay();
    void setHandPreference(ControlGrid::HandPreference preference);
    
    // For testing
    uint32_t getTickCounter() const { return tickCounter_; }
    uint32_t getTicksPerStep() const { return ticksPerStep_; }
    
private:
    // Pattern data with parameter lock support
    PatternData patternData_;
    TrackDefaults trackDefaults_[MAX_TRACKS];
    
    // Legacy compatibility arrays
    uint8_t trackVolumes_[MAX_TRACKS];
    bool trackMutes_[MAX_TRACKS];
    uint8_t trackMidiNotes_[MAX_TRACKS];
    uint8_t trackMidiChannels_[MAX_TRACKS];
    
    uint16_t bpm_;
    uint8_t stepCount_;
    uint8_t currentStep_;
    bool playing_;
    bool midiSyncEnabled_;
    
    uint32_t ticksPerStep_;
    uint32_t tickCounter_;
    uint32_t lastStepTime_;
    
    // Core interfaces
    IClock* clock_;
    IMidiOutput* midiOutput_;
    IMidiInput* midiInput_;
    IDisplay* display_;
    IInput* input_;
    bool ownsClock_;
    
    // Parameter lock components
    ParameterLockPool lockPool_;
    ParameterEngine paramEngine_;
    SequencerStateManager stateManager_;
    AdaptiveButtonTracker buttonTracker_;
    ControlGrid controlGrid_;
    
    // State tracking
    uint32_t lastUpdateTime_;
    
    void calculateTicksPerStep();
    void advanceStep();
    void sendMidiTriggers();
    void handleMidiInput();
    
    // Parameter lock methods
    void initializePatternData();
    void updateButtonStates(uint32_t currentTime);
    void checkForHoldEvents();
    void handleNormalModeButton(uint8_t button, bool pressed);
    void handleParameterLockInput(uint8_t button, bool pressed);
    bool buttonToTrackStep(uint8_t button, uint8_t& track, uint8_t& step) const;
    void updateVisualFeedback();
    void sendMidiWithParameters(uint8_t track, const CalculatedParameters& params);
};

#endif