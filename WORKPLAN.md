# NeoTrellis M4 MIDI over USB Support - Implementation Workplan

## Project Overview

This workplan documents the implementation of USB MIDI class-compliant support for the NeoTrellis M4 step sequencer. The goal is to enable the sequencer to send and receive MIDI data over USB to control external MIDI instruments and receive MIDI input for synchronization and control.

## Technical Foundation

### Current Architecture
- **Platform**: NeoTrellis M4 with SAMD51J19A microcontroller
- **Development**: Arduino CLI with Adafruit SAMD core
- **Framework**: Arduino with NeoTrellis M4 library
- **Core Logic**: Existing `StepSequencer` class with platform abstraction

### MIDI Capabilities
- **USB MIDI Device**: Native USB port supports class-compliant MIDI
- **Hardware**: 120MHz Cortex-M4F with hardware floating point
- **Memory**: 512KB flash, 192KB SRAM (sufficient for MIDI buffering)
- **Limitations**: Device only (not USB host) - can send/receive from computers/tablets

## Implementation Strategy

### Phase 1: MIDI Abstraction Layer
Create platform-agnostic MIDI interfaces following the existing architecture patterns.

#### 1.1 MIDI Interface Design
```cpp
// Core MIDI abstraction interface
class IMidiOutput {
public:
    virtual ~IMidiOutput() = default;
    virtual void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) = 0;
    virtual void sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) = 0;
    virtual void sendControlChange(uint8_t channel, uint8_t control, uint8_t value) = 0;
    virtual void sendClock() = 0;
    virtual void sendStart() = 0;
    virtual void sendStop() = 0;
    virtual void sendContinue() = 0;
    virtual bool isConnected() = 0;
};

class IMidiInput {
public:
    virtual ~IMidiInput() = default;
    virtual void setNoteOnCallback(void (*callback)(uint8_t channel, uint8_t note, uint8_t velocity)) = 0;
    virtual void setNoteOffCallback(void (*callback)(uint8_t channel, uint8_t note, uint8_t velocity)) = 0;
    virtual void setControlChangeCallback(void (*callback)(uint8_t channel, uint8_t control, uint8_t value)) = 0;
    virtual void setClockCallback(void (*callback)()) = 0;
    virtual void setStartCallback(void (*callback)()) = 0;
    virtual void setStopCallback(void (*callback)()) = 0;
    virtual void processMidiInput() = 0;
};
```

#### 1.2 Arduino MIDI Implementation
```cpp
// Arduino MIDI library wrapper
class ArduinoMidiOutput : public IMidiOutput {
private:
    // Using Arduino MIDIUSB library
};

class ArduinoMidiInput : public IMidiInput {
private:
    // MIDI input processing and callback handling
};
```

### Phase 2: Arduino Library Integration

#### 2.1 Library Dependencies
- **MIDIUSB**: Native Arduino library for USB MIDI (built into SAMD core)
- **Adafruit_NeoTrellis_M4**: Existing hardware abstraction
- **Dependencies**: No additional external libraries required

#### 2.2 Arduino CLI Setup
```bash
# Verify existing setup
arduino-cli core list | grep adafruit:samd
arduino-cli lib list | grep "Adafruit NeoTrellis"

# MIDIUSB is built into SAMD core - no additional install needed
```

### Phase 3: StepSequencer MIDI Integration

#### 3.1 Enhanced StepSequencer Class
Extend the existing `StepSequencer` to support MIDI output:

```cpp
class StepSequencer {
public:
    struct Dependencies {
        IClock* clock = nullptr;
        IMidiOutput* midiOutput = nullptr;  // New dependency
        IMidiInput* midiInput = nullptr;    // New dependency
    };
    
    // Existing methods...
    
    // New MIDI methods
    void setTrackMidiNote(uint8_t track, uint8_t note);
    void setTrackMidiChannel(uint8_t track, uint8_t channel);
    void setMidiSync(bool enabled);
    
private:
    uint8_t trackMidiNotes_[MAX_TRACKS] = {36, 38, 42, 46};  // Kick, snare, hihat, crash
    uint8_t trackMidiChannels_[MAX_TRACKS] = {10, 10, 10, 10}; // Drum channel
    bool midiSyncEnabled_ = false;
    
    IMidiOutput* midiOutput_;
    IMidiInput* midiInput_;
    
    void sendMidiTriggers();
    void handleMidiClock();
};
```

#### 3.2 MIDI Trigger Implementation
```cpp
void StepSequencer::sendMidiTriggers() {
    if (!midiOutput_ || !midiOutput_->isConnected()) return;
    
    for (uint8_t track = 0; track < MAX_TRACKS; track++) {
        if (!trackMutes_[track] && pattern_[track][currentStep_]) {
            midiOutput_->sendNoteOn(
                trackMidiChannels_[track],
                trackMidiNotes_[track], 
                trackVolumes_[track]
            );
            
            // Schedule note off (typical for drum sounds)
            // Implementation will use timer or next tick
        }
    }
}
```

### Phase 4: Arduino Sketch Implementation

#### 4.1 Main Sketch Structure
```cpp
// arduino_trellis_midi/arduino_trellis_midi.ino
#include <MIDIUSB.h>
#include "Adafruit_NeoTrellis.h"
#include "StepSequencer.h"  // Port from core/

// MIDI implementations
ArduinoMidiOutput midiOut;
ArduinoMidiInput midiIn;

// Hardware
Adafruit_NeoTrellis trellis;

// Core sequencer with MIDI
StepSequencer::Dependencies deps = {
    .midiOutput = &midiOut,
    .midiInput = &midiIn
};
StepSequencer sequencer(deps);

void setup() {
    // Existing trellis setup...
    
    // MIDI setup
    midiOut.begin();
    midiIn.begin();
    
    // Configure default MIDI mapping
    sequencer.setTrackMidiNote(0, 36);  // Kick
    sequencer.setTrackMidiNote(1, 38);  // Snare  
    sequencer.setTrackMidiNote(2, 42);  // Closed hihat
    sequencer.setTrackMidiNote(3, 46);  // Open hihat
}

void loop() {
    // Existing sequencer logic...
    midiIn.processMidiInput();  // Handle incoming MIDI
    
    // MIDI clock sync if enabled
    if (sequencer.isMidiSyncEnabled()) {
        // Sync to external MIDI clock instead of internal timing
    }
}
```

### Phase 5: Testing and Validation

#### 5.1 Unit Testing Strategy
```cpp
// test/test_midi_sequencer.cpp
#include "catch2/catch.hpp"
#include "StepSequencer.h"
#include "mocks/MockMidiOutput.h"

class MockMidiOutput : public IMidiOutput {
    // Track sent MIDI messages for verification
    std::vector<MidiMessage> sentMessages;
};

TEST_CASE("StepSequencer sends MIDI notes on active steps") {
    MockMidiOutput mockMidi;
    StepSequencer::Dependencies deps = {.midiOutput = &mockMidi};
    StepSequencer sequencer(deps);
    
    // Configure pattern and verify MIDI output
}
```

#### 5.2 Hardware Testing
1. **MIDI Monitor**: Use computer MIDI monitor to verify USB MIDI output
2. **DAW Integration**: Test with Ableton Live, Logic Pro, or similar
3. **Hardware Synthesizers**: Test with MIDI-compatible drum machines/synths
4. **Bidirectional Testing**: Send MIDI clock/control to sequencer

### Phase 6: Advanced Features

#### 6.1 MIDI Clock Synchronization
```cpp
class MidiClockSync {
public:
    void handleMidiClock();
    void calculateTempo();
    bool isExternalClockStable();
private:
    uint32_t clockTimes_[24];  // 24 clocks per quarter note
    uint8_t clockIndex_ = 0;
    uint16_t calculatedBPM_ = 120;
};
```

#### 6.2 Pattern Recording from MIDI Input
```cpp
void StepSequencer::handleMidiNoteInput(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (recordMode_) {
        // Find track matching this MIDI note
        uint8_t track = findTrackForNote(note);
        if (track < MAX_TRACKS) {
            pattern_[track][currentStep_] = true;
            updateLED(track, currentStep_, true);
        }
    }
}
```

#### 6.3 MIDI Control Surface
- **CC mapping**: Map continuous controllers to tempo, track volume, swing
- **Program changes**: Switch between pattern banks
- **System exclusive**: Custom configuration messages

## Implementation Timeline

### Week 1: Foundation
- [ ] Design and implement MIDI abstraction interfaces
- [ ] Create Arduino MIDI wrapper classes
- [ ] Unit tests for MIDI interfaces

### Week 2: Integration  
- [ ] Extend StepSequencer with MIDI dependencies
- [ ] Implement MIDI trigger output
- [ ] Basic Arduino sketch with MIDI output

### Week 3: Testing and Polish
- [ ] Comprehensive testing with MIDI hardware/software
- [ ] MIDI input processing and clock sync
- [ ] Documentation and examples

### Week 4: Advanced Features
- [ ] Pattern recording from MIDI input
- [ ] MIDI control surface mapping
- [ ] Performance optimization

## Technical Considerations

### Memory Management
- **MIDI Buffer Size**: 64-byte ring buffer for incoming MIDI (sufficient for real-time)
- **Static Allocation**: No dynamic memory allocation in MIDI paths
- **Flash Usage**: Estimate +5KB for MIDI code (well within 512KB capacity)

### Real-time Performance
- **MIDI Latency**: Target <1ms latency for note triggers
- **USB Polling**: 1kHz USB polling rate on SAMD51
- **Interrupt Priority**: MIDI processing in main loop, not ISR

### Error Handling
```cpp
// Robust error handling for MIDI operations
if (!midiOutput_->isConnected()) {
    // Graceful degradation - continue sequencer without MIDI
    return;
}

try {
    midiOutput_->sendNoteOn(channel, note, velocity);
} catch (const MidiException& e) {
    // Log error but don't crash sequencer
    debugSerial.printf("MIDI error: %s\n", e.what());
}
```

### Power Management
- **USB Suspend**: Handle USB sleep/wake states
- **MIDI Keep-alive**: Send active sensing messages to maintain connections

## Success Criteria

### Minimum Viable Product (MVP)
1. ✅ **USB MIDI Output**: Send note on/off for each sequencer step
2. ✅ **Class Compliance**: Works with standard MIDI software/hardware  
3. ✅ **Real-time Performance**: No noticeable latency or timing jitter
4. ✅ **Integration**: MIDI functionality doesn't interfere with existing sequencer

### Full Feature Set
1. ✅ **Bidirectional MIDI**: Send and receive MIDI data
2. ✅ **Clock Synchronization**: Sync to external MIDI clock
3. ✅ **Pattern Recording**: Record patterns from MIDI input
4. ✅ **Control Surface**: Map CCs to sequencer parameters
5. ✅ **Multiple Channels**: Support different MIDI channels per track

## Risk Mitigation

### Technical Risks
- **USB Compatibility**: Test with multiple operating systems and MIDI hosts
- **Timing Accuracy**: Verify MIDI clock precision meets professional standards  
- **Memory Constraints**: Monitor RAM usage, especially with MIDI buffering
- **Arduino Library Dependencies**: Ensure stable, maintained libraries

### Development Risks
- **Scope Creep**: Implement MVP first, add features incrementally
- **Testing Complexity**: Invest in proper test infrastructure early
- **Hardware Availability**: Test with multiple MIDI devices/software

## Future Enhancements

### Long-term Vision
1. **MIDI 2.0 Support**: When Arduino libraries support MIDI 2.0
2. **Wireless MIDI**: WiFi or Bluetooth MIDI for wireless operation
3. **Multi-device Sync**: Chain multiple NeoTrellis M4 units
4. **Advanced Sequencing**: Polyrhythms, probability, generative patterns

The architecture's modular design enables these future enhancements while maintaining backward compatibility with the current implementation.

## Conclusion

This workplan leverages the existing Arduino CLI infrastructure and clean architecture to add professional-grade MIDI capabilities to the NeoTrellis M4 step sequencer. The approach prioritizes reliability, real-time performance, and maintainability while enabling both basic MIDI output and advanced synchronization features.

The implementation follows the project's established patterns of dependency injection, platform abstraction, and comprehensive testing, ensuring the MIDI functionality integrates seamlessly with the existing codebase.