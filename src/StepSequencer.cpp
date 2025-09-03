#include "StepSequencer.h"
#include <string.h>

StepSequencer::StepSequencer() 
    : bpm_(120)
    , stepCount_(16)
    , currentStep_(0)
    , playing_(true)
    , tickCounter_(0)
    , lastTriggerTime_(0) {
    
    memset(pattern_, 0, sizeof(pattern_));
    
    for (int i = 0; i < MAX_TRACKS; i++) {
        trackVolumes_[i] = 127;
        trackMutes_[i] = false;
    }
    
    calculateTicksPerStep();
}

void StepSequencer::init(uint16_t bpm, uint8_t steps) {
    bpm_ = bpm;
    stepCount_ = (steps <= MAX_STEPS) ? steps : MAX_STEPS;
    calculateTicksPerStep();
    reset();
}

void StepSequencer::calculateTicksPerStep() {
    ticksPerStep_ = (60000 / bpm_) / 4;
}

void StepSequencer::tick() {
    if (!playing_) {
        return;
    }
    
    tickCounter_++;
    
    if (tickCounter_ >= ticksPerStep_) {
        advanceStep();
        tickCounter_ = 0;
    }
}

void StepSequencer::advanceStep() {
    currentStep_ = (currentStep_ + 1) % stepCount_;
}

void StepSequencer::start() {
    playing_ = true;
}

void StepSequencer::stop() {
    playing_ = false;
}

void StepSequencer::reset() {
    currentStep_ = 0;
    tickCounter_ = 0;
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

StepSequencer::TrackTrigger StepSequencer::getTriggeredTracks() {
    TrackTrigger trigger = {0, 0, false};
    
    if (tickCounter_ == 0) {
        for (uint8_t track = 0; track < MAX_TRACKS; track++) {
            if (!trackMutes_[track] && pattern_[track][currentStep_]) {
                trigger.track = track;
                trigger.velocity = trackVolumes_[track];
                trigger.triggered = true;
                break;
            }
        }
    }
    
    return trigger;
}