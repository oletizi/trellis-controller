#ifndef STEP_SEQUENCER_H
#define STEP_SEQUENCER_H

#include <stdint.h>

class StepSequencer {
public:
    static constexpr uint8_t MAX_TRACKS = 4;
    static constexpr uint8_t MAX_STEPS = 16;
    
    StepSequencer();
    
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
    
    struct TrackTrigger {
        uint8_t track;
        uint8_t velocity;
        bool triggered;
    };
    
    TrackTrigger getTriggeredTracks();
    
private:
    bool pattern_[MAX_TRACKS][MAX_STEPS];
    uint8_t trackVolumes_[MAX_TRACKS];
    bool trackMutes_[MAX_TRACKS];
    
    uint16_t bpm_;
    uint8_t stepCount_;
    uint8_t currentStep_;
    bool playing_;
    
    uint32_t ticksPerStep_;
    uint32_t tickCounter_;
    uint32_t lastTriggerTime_;
    
    void calculateTicksPerStep();
    void advanceStep();
};

#endif