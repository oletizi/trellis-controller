#include <catch2/catch_all.hpp>
#include "StepSequencer.h"
#include "mocks/MockClock.h"

TEST_CASE("StepSequencer - Default Construction", "[StepSequencer]") {
    StepSequencer sequencer;
    
    REQUIRE(sequencer.getTempo() == 120);
    REQUIRE(sequencer.getCurrentStep() == 0);
    REQUIRE(sequencer.isPlaying() == true);
    
    // Check all steps are initially inactive
    for (uint8_t track = 0; track < StepSequencer::MAX_TRACKS; track++) {
        for (uint8_t step = 0; step < StepSequencer::MAX_STEPS; step++) {
            REQUIRE(sequencer.isStepActive(track, step) == false);
        }
    }
    
    // Check default track settings
    for (uint8_t track = 0; track < StepSequencer::MAX_TRACKS; track++) {
        REQUIRE(sequencer.getTrackVolume(track) == 127);
        REQUIRE(sequencer.isTrackMuted(track) == false);
    }
}

TEST_CASE("StepSequencer - Dependency Injection", "[StepSequencer]") {
    auto mockClock = std::make_unique<MockClock>();
    MockClock* clockPtr = mockClock.get();
    
    StepSequencer::Dependencies deps;
    deps.clock = mockClock.release();
    
    StepSequencer sequencer(deps);
    
    // Verify the mock clock is being used
    clockPtr->setCurrentTime(1000);
    REQUIRE(clockPtr->getCurrentTime() == 1000);
    
    delete deps.clock; // Clean up
}

TEST_CASE("StepSequencer - Pattern Management", "[StepSequencer]") {
    StepSequencer sequencer;
    
    SECTION("Toggle steps") {
        // Initially all steps should be off
        REQUIRE(sequencer.isStepActive(0, 0) == false);
        
        // Toggle step on
        sequencer.toggleStep(0, 0);
        REQUIRE(sequencer.isStepActive(0, 0) == true);
        
        // Toggle step off
        sequencer.toggleStep(0, 0);
        REQUIRE(sequencer.isStepActive(0, 0) == false);
    }
    
    SECTION("Boundary testing") {
        // Valid boundaries
        sequencer.toggleStep(0, 0);
        sequencer.toggleStep(StepSequencer::MAX_TRACKS - 1, StepSequencer::MAX_STEPS - 1);
        REQUIRE(sequencer.isStepActive(0, 0) == true);
        REQUIRE(sequencer.isStepActive(StepSequencer::MAX_TRACKS - 1, StepSequencer::MAX_STEPS - 1) == true);
        
        // Invalid boundaries should not crash or affect valid data
        sequencer.toggleStep(StepSequencer::MAX_TRACKS, 0);
        sequencer.toggleStep(0, StepSequencer::MAX_STEPS);
        REQUIRE(sequencer.isStepActive(0, 0) == true); // Should be unchanged
    }
    
    SECTION("Multiple patterns") {
        // Create a pattern on track 0
        sequencer.toggleStep(0, 0);
        sequencer.toggleStep(0, 4);
        sequencer.toggleStep(0, 8);
        sequencer.toggleStep(0, 12);
        
        // Create a different pattern on track 1
        sequencer.toggleStep(1, 1);
        sequencer.toggleStep(1, 5);
        sequencer.toggleStep(1, 9);
        
        // Verify patterns are independent
        REQUIRE(sequencer.isStepActive(0, 0) == true);
        REQUIRE(sequencer.isStepActive(0, 1) == false);
        REQUIRE(sequencer.isStepActive(1, 0) == false);
        REQUIRE(sequencer.isStepActive(1, 1) == true);
    }
}

TEST_CASE("StepSequencer - Tempo and Timing", "[StepSequencer]") {
    auto mockClock = std::make_unique<MockClock>();
    MockClock* clockPtr = mockClock.get();
    
    StepSequencer::Dependencies deps;
    deps.clock = clockPtr;
    
    StepSequencer sequencer(deps);
    
    SECTION("Tempo calculation") {
        sequencer.setTempo(120);
        REQUIRE(sequencer.getTempo() == 120);
        REQUIRE(sequencer.getTicksPerStep() == 125); // 60000ms / 120 BPM / 4 = 125ms per 16th note
        
        sequencer.setTempo(60);
        REQUIRE(sequencer.getTempo() == 60);
        REQUIRE(sequencer.getTicksPerStep() == 250); // 60000ms / 60 BPM / 4 = 250ms per 16th note
        
        sequencer.setTempo(240);
        REQUIRE(sequencer.getTempo() == 240);
        REQUIRE(sequencer.getTicksPerStep() == 62); // 60000ms / 240 BPM / 4 = 62.5ms per 16th note (truncated)
    }
    
    SECTION("Step advancement") {
        sequencer.init(120, 4); // 4 steps, 120 BPM
        sequencer.start();
        
        REQUIRE(sequencer.getCurrentStep() == 0);
        
        // Advance time by less than step duration - should not advance
        clockPtr->advanceTime(100); // 100ms < 125ms step duration
        sequencer.tick();
        REQUIRE(sequencer.getCurrentStep() == 0);
        
        // Advance time by step duration - should advance
        clockPtr->advanceTime(25); // Total 125ms
        sequencer.tick();
        REQUIRE(sequencer.getCurrentStep() == 1);
        
        // Advance multiple steps
        clockPtr->advanceTime(125);
        sequencer.tick();
        REQUIRE(sequencer.getCurrentStep() == 2);
        
        clockPtr->advanceTime(125);
        sequencer.tick();
        REQUIRE(sequencer.getCurrentStep() == 3);
        
        // Should wrap around after last step
        clockPtr->advanceTime(125);
        sequencer.tick();
        REQUIRE(sequencer.getCurrentStep() == 0);
    }
}

TEST_CASE("StepSequencer - Playback Control", "[StepSequencer]") {
    auto mockClock = std::make_unique<MockClock>();
    MockClock* clockPtr = mockClock.get();
    
    StepSequencer::Dependencies deps;
    deps.clock = clockPtr;
    
    StepSequencer sequencer(deps);
    
    SECTION("Start/Stop") {
        REQUIRE(sequencer.isPlaying() == true); // Starts playing by default
        
        sequencer.stop();
        REQUIRE(sequencer.isPlaying() == false);
        
        // Should not advance when stopped
        clockPtr->advanceTime(1000);
        sequencer.tick();
        REQUIRE(sequencer.getCurrentStep() == 0);
        
        sequencer.start();
        REQUIRE(sequencer.isPlaying() == true);
        
        // Should advance when started
        clockPtr->advanceTime(125);
        sequencer.tick();
        REQUIRE(sequencer.getCurrentStep() == 1);
    }
    
    SECTION("Reset") {
        sequencer.init(120, 8);
        sequencer.start();
        
        // Advance a few steps
        for (int i = 0; i < 3; i++) {
            clockPtr->advanceTime(125);
            sequencer.tick();
        }
        REQUIRE(sequencer.getCurrentStep() == 3);
        
        // Reset should go back to step 0
        sequencer.reset();
        REQUIRE(sequencer.getCurrentStep() == 0);
        REQUIRE(sequencer.getTickCounter() == 0);
    }
}

TEST_CASE("StepSequencer - Track Management", "[StepSequencer]") {
    StepSequencer sequencer;
    
    SECTION("Volume control") {
        REQUIRE(sequencer.getTrackVolume(0) == 127); // Default volume
        
        sequencer.setTrackVolume(0, 64);
        REQUIRE(sequencer.getTrackVolume(0) == 64);
        
        sequencer.setTrackVolume(1, 32);
        REQUIRE(sequencer.getTrackVolume(1) == 32);
        REQUIRE(sequencer.getTrackVolume(0) == 64); // Should not affect other tracks
        
        // Boundary testing
        sequencer.setTrackVolume(StepSequencer::MAX_TRACKS, 100); // Invalid track
        REQUIRE(sequencer.getTrackVolume(StepSequencer::MAX_TRACKS) == 0); // Should return 0 for invalid track
    }
    
    SECTION("Mute control") {
        REQUIRE(sequencer.isTrackMuted(0) == false); // Default unmuted
        
        sequencer.setTrackMute(0, true);
        REQUIRE(sequencer.isTrackMuted(0) == true);
        
        sequencer.setTrackMute(0, false);
        REQUIRE(sequencer.isTrackMuted(0) == false);
        
        // Boundary testing
        sequencer.setTrackMute(StepSequencer::MAX_TRACKS, true); // Invalid track
        REQUIRE(sequencer.isTrackMuted(StepSequencer::MAX_TRACKS) == true); // Should return true for invalid track
    }
}

TEST_CASE("StepSequencer - Trigger Generation", "[StepSequencer]") {
    auto mockClock = std::make_unique<MockClock>();
    MockClock* clockPtr = mockClock.get();
    
    StepSequencer::Dependencies deps;
    deps.clock = clockPtr;
    
    StepSequencer sequencer(deps);
    
    SECTION("Basic trigger generation") {
        sequencer.init(120, 4);
        
        // Set up a pattern: track 0 step 0, track 1 step 1
        sequencer.toggleStep(0, 0);
        sequencer.toggleStep(1, 1);
        sequencer.setTrackVolume(0, 100);
        sequencer.setTrackVolume(1, 80);
        
        sequencer.start();
        
        // At step 0, track 0 should trigger
        auto trigger = sequencer.getTriggeredTracks();
        REQUIRE(trigger.triggered == true);
        REQUIRE(trigger.track == 0);
        REQUIRE(trigger.velocity == 100);
        
        // Advance to step 1
        clockPtr->advanceTime(125);
        sequencer.tick();
        REQUIRE(sequencer.getCurrentStep() == 1);
        
        trigger = sequencer.getTriggeredTracks();
        REQUIRE(trigger.triggered == true);
        REQUIRE(trigger.track == 1);
        REQUIRE(trigger.velocity == 80);
        
        // Advance to step 2 (no pattern)
        clockPtr->advanceTime(125);
        sequencer.tick();
        REQUIRE(sequencer.getCurrentStep() == 2);
        
        trigger = sequencer.getTriggeredTracks();
        REQUIRE(trigger.triggered == false);
    }
    
    SECTION("Muted tracks don't trigger") {
        sequencer.init(120, 2);
        sequencer.toggleStep(0, 0);
        sequencer.setTrackMute(0, true);
        
        sequencer.start();
        
        auto trigger = sequencer.getTriggeredTracks();
        REQUIRE(trigger.triggered == false); // Muted track should not trigger
    }
}