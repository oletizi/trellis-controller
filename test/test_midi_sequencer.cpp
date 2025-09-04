#include <catch2/catch_test_macros.hpp>
#include "StepSequencer.h"
#include "mocks/MockMidiOutput.h"
#include "mocks/MockMidiInput.h"
#include "mocks/MockClock.h"

TEST_CASE("StepSequencer MIDI functionality", "[midi]") {
    MockClock mockClock;
    MockMidiOutput mockMidiOutput;
    MockMidiInput mockMidiInput;
    
    StepSequencer::Dependencies deps = {
        .clock = &mockClock,
        .midiOutput = &mockMidiOutput,
        .midiInput = &mockMidiInput
    };
    
    StepSequencer sequencer(deps);
    sequencer.init(120, 8);
    
    SECTION("MIDI note configuration") {
        REQUIRE(sequencer.getTrackMidiNote(0) == 36);  // Default C2 (kick)
        REQUIRE(sequencer.getTrackMidiNote(1) == 37);  // C#2 (snare)
        REQUIRE(sequencer.getTrackMidiNote(2) == 38);  // D2 (hihat)
        REQUIRE(sequencer.getTrackMidiNote(3) == 39);  // D#2 (crash)
        
        REQUIRE(sequencer.getTrackMidiChannel(0) == 10);  // Drum channel
        
        sequencer.setTrackMidiNote(0, 40);
        REQUIRE(sequencer.getTrackMidiNote(0) == 40);
        
        sequencer.setTrackMidiChannel(0, 5);
        REQUIRE(sequencer.getTrackMidiChannel(0) == 5);
    }
    
    SECTION("MIDI output on pattern triggers") {
        sequencer.toggleStep(0, 0);  // Enable step 0 on track 0
        sequencer.toggleStep(1, 2);  // Enable step 2 on track 1
        
        sequencer.reset();
        mockMidiOutput.clearMessages();
        
        // Step to position 0 - should trigger track 0
        mockClock.advance(sequencer.getTicksPerStep());
        sequencer.tick();
        
        auto noteMessages = mockMidiOutput.getNoteMessages();
        REQUIRE(noteMessages.size() == 1);
        REQUIRE(noteMessages[0].isNoteOn == true);
        REQUIRE(noteMessages[0].channel == 10);  // Drum channel
        REQUIRE(noteMessages[0].note == 36);     // Default C2
        REQUIRE(noteMessages[0].velocity == 127); // Default volume
        
        mockMidiOutput.clearMessages();
        
        // Step to position 1 - should not trigger anything
        mockClock.advance(sequencer.getTicksPerStep());
        sequencer.tick();
        REQUIRE(mockMidiOutput.getNoteMessages().size() == 0);
        
        // Step to position 2 - should trigger track 1
        mockClock.advance(sequencer.getTicksPerStep());
        sequencer.tick();
        
        noteMessages = mockMidiOutput.getNoteMessages();
        REQUIRE(noteMessages.size() == 1);
        REQUIRE(noteMessages[0].isNoteOn == true);
        REQUIRE(noteMessages[0].channel == 10);
        REQUIRE(noteMessages[0].note == 37);     // C#2 for track 1
        REQUIRE(noteMessages[0].velocity == 127);
    }
    
    SECTION("MIDI sync configuration") {
        REQUIRE(sequencer.isMidiSync() == false);  // Default disabled
        
        sequencer.setMidiSync(true);
        REQUIRE(sequencer.isMidiSync() == true);
        
        sequencer.setMidiSync(false);
        REQUIRE(sequencer.isMidiSync() == false);
    }
    
    SECTION("Muted tracks don't send MIDI") {
        sequencer.toggleStep(0, 0);  // Enable step 0 on track 0
        sequencer.setTrackMute(0, true);  // Mute track 0
        
        sequencer.reset();
        mockMidiOutput.clearMessages();
        
        mockClock.advance(sequencer.getTicksPerStep());
        sequencer.tick();
        
        // No MIDI should be sent for muted track
        REQUIRE(mockMidiOutput.getNoteMessages().size() == 0);
    }
    
    SECTION("MIDI not sent when disconnected") {
        mockMidiOutput.setConnected(false);
        
        sequencer.toggleStep(0, 0);
        sequencer.reset();
        mockMidiOutput.clearMessages();
        
        mockClock.advance(sequencer.getTicksPerStep());
        sequencer.tick();
        
        // No MIDI should be sent when disconnected
        REQUIRE(mockMidiOutput.getNoteMessages().size() == 0);
    }
}