#include "StepSequencer.h"
#include <cstring>

// Default clock implementation for embedded use
class SystemClock : public IClock {
public:
    uint32_t getCurrentTime() const override {
        static uint32_t time = 0;
        return time++;
    }
    
    void delay(uint32_t milliseconds) override {
        volatile uint32_t count = milliseconds * 1000;
        while(count--);
    }
    
    void reset() override {
        // Reset implementation if needed
    }
};

StepSequencer::StepSequencer() 
    : bpm_(120)
    , stepCount_(16)
    , currentStep_(0)
    , playing_(true)
    , tickCounter_(0)
    , lastStepTime_(0)
    , ownsClock_(false) {
    
    std::memset(pattern_, 0, sizeof(pattern_));
    
    for (int i = 0; i < MAX_TRACKS; i++) {
        trackVolumes_[i] = 127;
        trackMutes_[i] = false;
    }
    
    static SystemClock defaultClock;
    clock_ = &defaultClock;
    ownsClock_ = false; // Static instance, don't delete
    
    calculateTicksPerStep();
}

StepSequencer::StepSequencer(Dependencies deps) 
    : bpm_(120)
    , stepCount_(16)
    , currentStep_(0)
    , playing_(true)
    , tickCounter_(0)
    , lastStepTime_(0)
    , ownsClock_(false) {
    
    std::memset(pattern_, 0, sizeof(pattern_));
    
    for (int i = 0; i < MAX_TRACKS; i++) {
        trackVolumes_[i] = 127;
        trackMutes_[i] = false;
    }
    
    if (deps.clock) {
        clock_ = deps.clock;
        ownsClock_ = false;
    } else {
        static SystemClock defaultClock;
        clock_ = &defaultClock;
        ownsClock_ = false; // Static instance, don't delete
    }
    
    calculateTicksPerStep();
}

StepSequencer::~StepSequencer() {
    if (ownsClock_ && clock_) {
        delete clock_;
    }
}

void StepSequencer::init(uint16_t bpm, uint8_t steps) {
    bpm_ = bpm;
    stepCount_ = (steps <= MAX_STEPS) ? steps : MAX_STEPS;
    calculateTicksPerStep();
    reset();
}

void StepSequencer::calculateTicksPerStep() {
    // Convert BPM to milliseconds per 16th note
    // 120 BPM = 120 beats/min = 2 beats/sec = 500ms per beat
    // 16th note = beat/4 = 125ms at 120 BPM
    ticksPerStep_ = (60000 / bpm_) / 4;
}

void StepSequencer::tick() {
    if (!playing_) {
        return;
    }
    
    uint32_t currentTime = clock_->getCurrentTime();
    
    if (currentTime - lastStepTime_ >= ticksPerStep_) {
        advanceStep();
        lastStepTime_ = currentTime;
    }
}

void StepSequencer::advanceStep() {
    currentStep_ = (currentStep_ + 1) % stepCount_;
    tickCounter_++;
}

void StepSequencer::start() {
    playing_ = true;
    lastStepTime_ = clock_->getCurrentTime();
}

void StepSequencer::stop() {
    playing_ = false;
}

void StepSequencer::reset() {
    currentStep_ = 0;
    tickCounter_ = 0;
    lastStepTime_ = clock_->getCurrentTime();
}

void StepSequencer::toggleStep(uint8_t track, uint8_t step) {
    if (track < MAX_TRACKS && step < stepCount_) {
        pattern_[track][step] = !pattern_[track][step];
    }
}

bool StepSequencer::isStepActive(uint8_t track, uint8_t step) const {
    if (track < MAX_TRACKS && step < MAX_STEPS) {
        return pattern_[track][step];
    }
    return false;
}

void StepSequencer::setTempo(uint16_t bpm) {
    bpm_ = bpm;
    calculateTicksPerStep();
}

void StepSequencer::setTrackVolume(uint8_t track, uint8_t volume) {
    if (track < MAX_TRACKS) {
        trackVolumes_[track] = volume;
    }
}

void StepSequencer::setTrackMute(uint8_t track, bool mute) {
    if (track < MAX_TRACKS) {
        trackMutes_[track] = mute;
    }
}

uint8_t StepSequencer::getTrackVolume(uint8_t track) const {
    return (track < MAX_TRACKS) ? trackVolumes_[track] : 0;
}

bool StepSequencer::isTrackMuted(uint8_t track) const {
    return (track < MAX_TRACKS) ? trackMutes_[track] : true;
}

StepSequencer::TrackTrigger StepSequencer::getTriggeredTracks() {
    TrackTrigger trigger = {0, 0, false};
    
    // Check if we just advanced to a new step
    for (uint8_t track = 0; track < MAX_TRACKS; track++) {
        if (!trackMutes_[track] && pattern_[track][currentStep_]) {
            trigger.track = track;
            trigger.velocity = trackVolumes_[track];
            trigger.triggered = true;
            break; // Return first triggered track
        }
    }
    
    return trigger;
}