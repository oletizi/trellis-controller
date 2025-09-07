# Non-Realtime Sequencer System - Architecture & Implementation Plan

## Overview

This document outlines the architecture and implementation plan for a non-realtime sequencer system that enables deterministic testing and pattern save/load functionality.

## System Goals

1. **Deterministic Testing**: Remove real-time dependencies to enable reproducible tests
2. **Pattern Save/Load**: Comprehensive state persistence for user pattern management
3. **Bug Verification**: Test recently fixed parameter lock issues systematically
4. **Future Testing**: Make testing every sequencer feature easier going forward

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                 NonRealtimeSequencer                        │
├─────────────────────────────────────────────────────────────┤
│ • Control Message Processing                                │
│ • State Capture & Restore                                   │
│ • Batch Execution                                          │
│ • File I/O Management                                       │
└─────────────────┬───────────────────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────────────────┐
│                  StepSequencer                              │
│              (Existing, Unmodified)                         │
├─────────────────────────────────────────────────────────────┤
│ • Parameter Lock System                                     │
│ • Pattern Management                                        │
│ • MIDI Output                                              │
│ • Display Updates                                          │
└─────────┬───────────────┬───────────────┬─────────────────┘
          │               │               │
┌─────────▼─────┐ ┌──────▼──────┐ ┌──────▼──────┐
│  NullClock    │ │ NullDisplay │ │  NullMidi   │
│               │ │             │ │             │
│ • Virtual     │ │ • No Output │ │ • No Output │
│   Time        │ │             │ │             │
└───────────────┘ └─────────────┘ └─────────────┘
```

## Component Design

### 1. Control Message System

**File**: `include/core/ControlMessage.h`

Discrete events that drive the sequencer deterministically:
- Key Press/Release events
- Clock ticks
- Time advancement
- Sequencer controls (start/stop/reset)
- State management (save/load/verify)

### 2. State Persistence System  

**File**: `include/core/SequencerState.h`

Complete serialization of all sequencer state to JSON format.

## Detailed Component Design

### 1. ControlMessage System

```cpp
namespace ControlMessage {
    enum class Type {
        KEY_PRESS, KEY_RELEASE,    // Button events
        CLOCK_TICK, TIME_ADVANCE,  // Time control
        START, STOP, RESET,        // Sequencer control
        SAVE_STATE, LOAD_STATE,    // State management
        VERIFY_STATE, QUERY_STATE  // Testing utilities
    };
    
    struct Message {
        Type type;
        uint32_t timestamp;        // Virtual time
        uint32_t param1, param2;   // Parameters
        std::string stringParam;   // File paths, JSON
    };
}
```

**Key Features**:
- Factory methods for common messages
- String serialization for logging
- Timestamp ordering for batch execution

### 2. SequencerState System

**Purpose**: Complete serialization of sequencer state to JSON

**State Components**:
```cpp
struct Snapshot {
    // Core sequencer state
    uint16_t bpm;
    uint8_t currentStep;
    bool playing;
    uint32_t currentTime;
    
    // Pattern data (4x8 grid)
    StepState pattern[4][8];
    
    // Parameter locks (64 slots)
    ParameterLockState parameterLocks[64];
    
    // Button states (32 buttons)
    ButtonState buttons[32];
    
    // Parameter lock mode state
    bool inParameterLockMode;
    uint8_t heldTrack, heldStep;
    
    // Track settings
    uint8_t trackVolumes[4];
    bool trackMutes[4];
    uint8_t trackNotes[4];
    uint8_t trackChannels[4];
};
```

### 3. NonRealtimeSequencer Controller

**Purpose**: Orchestrates control message processing and state management

**Key Methods**:
- `processMessage()` - Execute single control message
- `processBatch()` - Execute sequence of messages  
- `getCurrentState()` / `setState()` - State access
- `saveState()` / `loadState()` - File persistence

## JSON Schema Design

### Complete Sequencer State Format

```json
{
  "sequencer": {
    "bpm": 120,
    "stepCount": 8,
    "currentStep": 2,
    "playing": true,
    "currentTime": 5000,
    "tickCounter": 48
  },
  "pattern": [
    [
      {"active": true, "hasLock": false, "lockIndex": 255},
      {"active": false, "hasLock": false, "lockIndex": 255},
      {"active": true, "hasLock": true, "lockIndex": 5}
    ]
  ],
  "parameterLocks": [
    {
      "inUse": true,
      "stepIndex": 2,
      "trackIndex": 0,
      "activeLocks": 3,
      "noteOffset": 7,
      "velocity": 110,
      "length": 16
    }
  ],
  "buttons": [
    {
      "pressed": false,
      "wasPressed": false, 
      "wasReleased": true,
      "pressTime": 4800,
      "releaseTime": 4950,
      "isHeld": false,
      "holdProcessed": true,
      "holdDuration": 150
    }
  ],
  "parameterLockMode": {
    "active": false,
    "heldTrack": 255,
    "heldStep": 255
  },
  "tracks": [
    {
      "volume": 100,
      "muted": false,
      "note": 36,
      "channel": 9
    }
  ]
}
```

## Implementation Phases

### Phase 1: Foundation (Core Infrastructure)
**Files to Create**:
- `src/core/ControlMessage.cpp` ✓
- `src/core/SequencerState.cpp` 
- JSON utility functions

**Deliverables**:
- Control message system working
- Basic JSON serialization for simple types
- State snapshot creation (no persistence yet)

**Time Estimate**: 2-3 hours

### Phase 2: State Serialization (JSON I/O)
**Files to Create**:
- Complete `SequencerState::Snapshot::toJson()`
- Complete `SequencerState::Snapshot::fromJson()`  
- File I/O utilities

**Deliverables**:
- Full state serialization to/from JSON
- File save/load functionality
- State comparison utilities

**Time Estimate**: 3-4 hours

### Phase 3: NonRealtimeSequencer (Controller)
**Files to Create**:
- `src/core/NonRealtimeSequencer.cpp`
- Null interface implementations (NullDisplay, etc.)

**Deliverables**:
- Control message processing
- State capture/restore integration with StepSequencer
- Batch execution capability

**Time Estimate**: 2-3 hours

### Phase 4: Testing Infrastructure
**Files to Create**:
- Test harness executable
- Example control scripts
- CMake integration

**Deliverables**:
- Parameter lock bug verification tests
- Comprehensive test suite template
- Documentation and examples

**Time Estimate**: 1-2 hours

## Integration Strategy

### With Existing StepSequencer
- **No Modifications Required**: Use dependency injection
- **Null Interfaces**: Provide NullDisplay, NullMidi, etc.
- **State Access**: Add getter methods to StepSequencer if needed

### With CMake Build System
- New executable target: `trellis_test_harness`
- Conditional compilation flags for testing features
- Link with existing core libraries

## Risk Assessment & Mitigation

### High Risk: JSON Parsing Complexity
- **Risk**: Hand-written JSON parsing is error-prone
- **Mitigation**: Use JSON schema with off-the-shelf parser/generator
- **Implementation**: Define JSON schema, use nlohmann/json or similar library
- **Benefits**: Type safety, validation, automatic serialization

### Medium Risk: State Capture Completeness  
- **Risk**: Missing state components leads to incorrect restoration
- **Mitigation**: Systematic review of all StepSequencer member variables
- **Verification**: Round-trip tests (save → load → compare)

### Medium Risk: Control Message Complexity
- **Risk**: Complex message sequences hard to debug
- **Mitigation**: Verbose logging, message validation
- **Tools**: Message sequence visualization

### Low Risk: Performance
- **Risk**: JSON serialization could be slow for large states
- **Mitigation**: Profile and optimize hot paths
- **Note**: Non-realtime usage makes this less critical

## Testing Strategy Using New System

### Parameter Lock Bug Verification
```cpp
// Test 1: Verify parameter lock mode doesn't exit on control press
std::vector<ControlMessage::Message> test = {
    ControlMessage::Message::keyPress(0),           // Press step 0
    ControlMessage::Message::timeAdvance(600),      // Hold beyond threshold  
    ControlMessage::Message::keyPress(20),          // Press control button
    ControlMessage::Message::keyRelease(20),        // Release control button
    ControlMessage::Message::verifyState("inParameterLockMode: true"),
    ControlMessage::Message::keyRelease(0),         // Release held step
    ControlMessage::Message::verifyState("inParameterLockMode: false")
};
```

### Comprehensive Feature Testing Template
- **Pattern Editing**: Step activate/deactivate sequences
- **Parameter Locks**: Complex adjustment sequences
- **Timing**: BPM changes, clock synchronization
- **MIDI**: Note triggers, velocity changes
- **State Persistence**: Save/load verification

## Success Criteria

1. **All Parameter Lock Bugs Fixed**: Deterministic tests pass
2. **Pattern Save/Load Working**: Complete state round-trip 
3. **Test Infrastructure Ready**: Easy to write new tests
4. **Documentation Complete**: Clear usage examples
5. **Integration Clean**: No modifications to existing code

## Next Steps

1. Start with Phase 1 implementation
2. Create simple test to verify basic functionality
3. Iterate through phases with testing at each step
4. Use system immediately to verify parameter lock fixes

---

**Total Estimated Implementation Time**: 8-12 hours
**Value**: Enables deterministic testing of all features + pattern save/load user feature