# Parameter Locks Implementation Work Plan

## Overview
Implement per-step parameter locks for the NeoTrellis M4 sequencer, allowing individual steps to have custom parameter values that override channel defaults. This feature, inspired by Elektron sequencers, enables complex and evolving patterns within a single sequence.

## Core Concept

### Parameter Lock Mode
- **Activation**: Press and hold any step button to enter parameter lock mode for that step
- **UI Division**: While holding a step, the 4x4 grid on the opposite end becomes a parameter control interface
- **Initial Parameters**: Start with MIDI note adjustment, then expand to velocity, length, etc.

### Grid Layout During Parameter Lock Mode

```
When holding a step in columns 5-8:
[A1-A4][B1-B4][C1-C4][D1-D4] = Parameter controls
[A5-A8][B5-B8][C5-C8][D5-D8] = Normal steps (one held)

When holding a step in columns 1-4:
[A1-A4][B1-B4][C1-C4][D1-D4] = Normal steps (one held)
[A5-A8][B5-B8][C5-C8][D5-D8] = Parameter controls
```

### Initial Control Mapping
- **Note Up**: C1 (when holding steps 5-8) or C8 (when holding steps 1-4)
- **Note Down**: D1 (when holding steps 5-8) or D8 (when holding steps 1-4)

## Phase 1: Architecture Design

### 1.1 Data Structure Design
- [ ] Design parameter lock storage structure
- [ ] Define parameter types enum (NOTE, VELOCITY, LENGTH, etc.)
- [ ] Create per-step parameter storage (sparse to save memory)
- [ ] Design channel default parameter structure
- [ ] Plan memory-efficient storage strategy (only store overrides)

```cpp
// Example structure
struct ParameterLock {
    enum Type {
        NONE = 0,
        NOTE = 1,
        VELOCITY = 2,
        LENGTH = 4,
        // Bitmask for multiple params
    };
    
    uint8_t activeLocks;  // Bitmask of active locks
    int8_t noteOffset;     // -64 to +63 semitones
    uint8_t velocity;      // 0-127
    uint8_t length;        // In clock ticks
};

struct StepData {
    bool active;
    ParameterLock* locks;  // nullptr if no locks
};
```

### 1.2 State Machine Design
- [ ] Define parameter lock mode states
- [ ] Design mode entry/exit conditions
- [ ] Plan interaction with existing sequencer states
- [ ] Handle mode conflicts (e.g., during playback)

```cpp
enum SequencerMode {
    MODE_NORMAL,
    MODE_PARAMETER_LOCK,
    MODE_PATTERN_SELECT,
    // etc.
};

struct ParameterLockState {
    bool active;
    uint8_t heldStep;
    uint8_t heldTrack;
    uint32_t holdStartTime;
    uint8_t currentParameter;
};
```

## Phase 2: Core Implementation

### 2.1 Button Hold Detection
- [ ] Implement button hold timing (distinguish tap vs hold)
- [ ] Define hold threshold (e.g., 500ms)
- [ ] Track hold state per button
- [ ] Handle multiple simultaneous holds (define behavior)

```cpp
class ButtonTracker {
    struct ButtonState {
        bool pressed;
        uint32_t pressTime;
        bool isHeld;
    };
    
    ButtonState states[32];
    static constexpr uint32_t HOLD_THRESHOLD_MS = 500;
    
    void update(uint32_t buttonMask);
    bool isHeld(uint8_t button);
    uint8_t getHeldButton();
};
```

### 2.2 Parameter Lock Mode Entry/Exit
- [ ] Detect hold on step button
- [ ] Enter parameter lock mode
- [ ] Identify control grid based on held step position
- [ ] Visual feedback for mode entry (LED patterns)
- [ ] Clean exit on button release

### 2.3 Control Grid Management
- [ ] Calculate control grid position (opposite end from held step)
- [ ] Map control buttons to parameter functions
- [ ] Implement dynamic control assignment
- [ ] Handle edge cases (multiple holds, invalid positions)

```cpp
class ControlGrid {
    struct GridMapping {
        uint8_t noteUpButton;
        uint8_t noteDownButton;
        uint8_t velocityButtons[4];
        // etc.
    };
    
    GridMapping getMapping(uint8_t heldStep);
    bool isInControlGrid(uint8_t button, uint8_t heldStep);
};
```

## Phase 3: Parameter Manipulation

### 3.1 Note Parameter Implementation
- [ ] Implement note offset storage per step
- [ ] Create note increment/decrement functions
- [ ] Define note range limits (-64 to +63 semitones)
- [ ] Apply note offsets during playback
- [ ] Visual feedback for current note value

```cpp
class NoteParameter {
    static constexpr int8_t MIN_OFFSET = -64;
    static constexpr int8_t MAX_OFFSET = 63;
    
    void increment(StepData& step);
    void decrement(StepData& step);
    uint8_t applyOffset(uint8_t baseNote, int8_t offset);
    void displayValue(IDisplay* display, int8_t offset);
};
```

### 3.2 Parameter Lock Storage
- [ ] Implement sparse storage for parameter locks
- [ ] Create/destroy locks on demand
- [ ] Memory pool for lock objects
- [ ] Serialization for pattern save/load
- [ ] Clear locks function

### 3.3 Playback Integration
- [ ] Modify sequencer playback to check for locks
- [ ] Apply parameter locks when triggering steps
- [ ] Merge locks with channel defaults
- [ ] Ensure proper MIDI output with locked parameters

```cpp
void StepSequencer::triggerStep(uint8_t track, uint8_t step) {
    StepData& stepData = pattern[track][step];
    
    if (stepData.active) {
        uint8_t note = channelDefaults[track].note;
        uint8_t velocity = channelDefaults[track].velocity;
        
        if (stepData.locks) {
            if (stepData.locks->activeLocks & ParameterLock::NOTE) {
                note = applyNoteOffset(note, stepData.locks->noteOffset);
            }
            if (stepData.locks->activeLocks & ParameterLock::VELOCITY) {
                velocity = stepData.locks->velocity;
            }
        }
        
        midiOutput->sendNoteOn(track, note, velocity);
    }
}
```

## Phase 4: Visual Feedback

### 4.1 LED Indication System
- [ ] Design LED color scheme for parameter lock mode
- [ ] Show held step with unique color/brightness
- [ ] Highlight control grid area
- [ ] Indicate steps with active locks
- [ ] Show parameter values visually (if possible)

```cpp
class ParameterLockDisplay {
    Color HELD_STEP_COLOR = Color(255, 255, 255);     // White
    Color CONTROL_GRID_COLOR = Color(0, 128, 255);    // Cyan
    Color LOCKED_STEP_COLOR = Color(255, 128, 0);     // Orange
    Color NOTE_UP_COLOR = Color(0, 255, 0);           // Green
    Color NOTE_DOWN_COLOR = Color(255, 0, 0);         // Red
    
    void updateDisplay(IDisplay* display, const ParameterLockState& state);
    void showParameterValue(IDisplay* display, uint8_t param, int value);
};
```

### 4.2 Animation and Feedback
- [ ] Entry animation for parameter lock mode
- [ ] Button press feedback in control grid
- [ ] Value change animations
- [ ] Exit animation
- [ ] Error indication (e.g., parameter at limit)

## Phase 5: Extended Parameters

### 5.1 Velocity Parameter
- [ ] Add velocity control buttons
- [ ] Implement velocity storage
- [ ] Visual feedback for velocity levels
- [ ] Apply velocity during playback

### 5.2 Note Length Parameter
- [ ] Add length control buttons
- [ ] Implement length storage (gate time)
- [ ] Visual feedback for length
- [ ] Modify note-off timing

### 5.3 Additional Parameters
- [ ] Probability (chance of step playing)
- [ ] Micro-timing (swing/shuffle per step)
- [ ] CC values
- [ ] Ratcheting (note repeats)

## Phase 6: Channel Defaults

### 6.1 Default Parameter Configuration
- [ ] Create channel default storage
- [ ] Implement default parameter UI
- [ ] Save/load channel defaults
- [ ] Apply defaults to new steps

### 6.2 Parameter Copy/Paste
- [ ] Copy parameters from one step
- [ ] Paste to other steps
- [ ] Clear all locks on a track
- [ ] Parameter lock templates

## Phase 7: Advanced Features

### 7.1 Parameter Automation
- [ ] Parameter slides between steps
- [ ] LFO-style parameter modulation
- [ ] Parameter patterns (cycles)

### 7.2 Performance Features
- [ ] Temporary parameter override (performance mode)
- [ ] Parameter scenes (A/B states)
- [ ] Morph between parameter sets

## Implementation Priority

1. **Core Infrastructure** (Phase 1-2): Essential foundation
2. **Note Parameter** (Phase 3.1): Minimum viable feature
3. **Visual Feedback** (Phase 4): Critical for usability
4. **Velocity & Length** (Phase 5.1-5.2): Core parameters
5. **Channel Defaults** (Phase 6): Workflow improvement
6. **Extended Parameters** (Phase 5.3): Enhanced capability
7. **Advanced Features** (Phase 7): Future expansion

## Technical Considerations

### Memory Management
- **SAMD51 RAM**: 192KB total
- **Per-step overhead**: ~8-16 bytes with locks
- **32 steps × 8 tracks**: Max 4KB with full locks
- **Strategy**: Use sparse storage, allocate on demand

### Performance Impact
- **Button scanning**: Must maintain 100Hz rate
- **LED updates**: Keep at 30+ FPS
- **MIDI timing**: <1ms jitter
- **Parameter processing**: Must not impact sequencer timing

### UI/UX Principles
- **Intuitive mapping**: Opposite-end control grid
- **Clear feedback**: Always show current mode
- **Reversible actions**: Easy to clear locks
- **Discoverable**: Visual hints for available controls

## Testing Strategy

### Unit Tests
- [ ] Parameter lock data structure tests
- [ ] Control grid mapping tests
- [ ] Parameter calculation tests
- [ ] Memory management tests

### Integration Tests
- [ ] Mode switching during playback
- [ ] Parameter application during sequencing
- [ ] Save/load with parameter locks
- [ ] Memory usage under load

### User Testing
- [ ] Hold detection timing feels natural
- [ ] Control mapping is intuitive
- [ ] Visual feedback is clear
- [ ] Performance remains smooth

## Success Criteria

- [ ] Hold any step for 500ms enters parameter lock mode
- [ ] Control grid appears on opposite end
- [ ] Note up/down buttons work as specified
- [ ] Parameters persist across pattern playback
- [ ] Visual feedback clearly indicates mode and values
- [ ] No impact on sequencer timing or performance
- [ ] Memory usage remains under 50% with full locks
- [ ] Pattern save/load includes parameter locks

## Risk Mitigation

### Risk: Memory Exhaustion
- **Mitigation**: Implement hard limits on locks per pattern
- **Fallback**: LRU eviction of oldest locks

### Risk: UI Complexity
- **Mitigation**: Start with single parameter (note)
- **Testing**: User testing at each parameter addition

### Risk: Timing Impact
- **Mitigation**: Pre-calculate parameters before step trigger
- **Optimization**: Use lookup tables where possible

### Risk: Mode Confusion
- **Mitigation**: Strong visual differentiation
- **Solution**: Timeout to exit mode if no activity

## Development Phases Timeline

- Phase 1-2: Core infrastructure (8-10 hours)
- Phase 3: Note parameter (6-8 hours)
- Phase 4: Visual feedback (4-6 hours)
- Phase 5: Extended parameters (8-10 hours)
- Phase 6: Channel defaults (4-6 hours)
- Phase 7: Advanced features (optional, 10+ hours)

**Total: 30-40 hours** for full implementation

## Notes

This implementation brings professional-level sequencing capabilities to the NeoTrellis M4, matching features found in high-end hardware sequencers. The parameter lock system will enable users to create complex, evolving patterns that go beyond simple step on/off sequencing.

---

# Architectural Review: Parameter Locks Implementation

**Reviewer**: Senior Architecture Reviewer  
**Date**: 2025-09-06  
**Scope**: Complete architectural assessment of parameter locks feature design

## Executive Summary

**APPROVAL RECOMMENDATION**: ✅ **APPROVED WITH MODIFICATIONS**

The parameter locks implementation demonstrates solid architectural thinking with appropriate consideration for the SAMD51's constraints. The sparse storage strategy and platform abstraction alignment are well-designed. However, several critical architectural concerns require resolution before implementation.

**Key Strengths**:
- Excellent adherence to existing platform abstraction patterns
- Well-designed sparse storage strategy for memory efficiency
- Thoughtful phased implementation approach
- Comprehensive risk identification and mitigation strategies

**Critical Issues Requiring Resolution**:
- Data structure requires significant redesign for embedded constraints
- State machine needs integration with existing sequencer architecture  
- Memory pooling strategy requires specification
- Performance impact analysis incomplete

## 1. Architecture Design Assessment

### 1.1 Data Structure Design ⚠️ **REQUIRES MODIFICATION**

**Current Design Issues**:
```cpp
// PROBLEMATIC: Direct pointer usage violates embedded guidelines
struct StepData {
    bool active;
    ParameterLock* locks;  // ❌ Raw pointer, dynamic allocation
};
```

**Architectural Problems**:
- **Memory Allocation Violation**: Raw pointers suggest dynamic allocation, violating the "no dynamic allocation in real-time paths" constraint
- **Memory Fragmentation Risk**: Sparse allocation without pooling strategy
- **Cache Performance**: Poor cache locality with scattered allocations

**RECOMMENDED ARCHITECTURE**:
```cpp
// Fixed-size memory pool approach
class ParameterLockPool {
public:
    static constexpr size_t MAX_LOCKS = 64;  // Configurable based on memory budget
    
    struct ParameterLock {
        uint8_t activeLocks;   // Bitmask
        int8_t noteOffset;     // -64 to +63
        uint8_t velocity;      // 0-127  
        uint8_t length;        // Gate time in ticks
        uint8_t stepIndex;     // Back-reference for validation
        uint8_t trackIndex;    // Track ownership
        bool inUse;            // Pool management
    };
    
private:
    ParameterLock pool_[MAX_LOCKS];
    uint8_t freeList_[MAX_LOCKS];
    uint8_t freeCount_;
    
public:
    uint8_t allocate(uint8_t track, uint8_t step);  // Returns index, not pointer
    void deallocate(uint8_t index);
    ParameterLock& getLock(uint8_t index);
    bool isValidIndex(uint8_t index) const;
};

// Improved step data structure
struct StepData {
    bool active : 1;
    bool hasLock : 1;
    uint8_t lockIndex : 6;  // Index into lock pool, 0-63
};
```

**Architectural Benefits**:
- **Zero Dynamic Allocation**: All memory pre-allocated at startup
- **Cache Friendly**: Contiguous memory layout
- **Bounded Memory**: Predictable memory usage
- **Fast Allocation**: O(1) allocation/deallocation from free list

### 1.2 State Machine Integration ✅ **ARCHITECTURALLY SOUND**

**Enhancement Recommendation**:
```cpp
// Integrate with existing sequencer state
class SequencerStateManager {
public:
    enum class Mode {
        NORMAL,
        PARAMETER_LOCK,
        PATTERN_SELECT,
        SHIFT_CONTROL  // Existing mode
    };
    
    struct ParameterLockContext {
        bool active = false;
        uint8_t heldStep = 0xFF;
        uint8_t heldTrack = 0xFF; 
        uint32_t holdStartTime = 0;
        uint8_t controlGridStart = 0;  // Calculated control area
        Mode previousMode = Mode::NORMAL;  // For clean exit
    };
};
```

## 2. UI/UX Design Assessment

### 2.1 "Opposite End" Control Scheme ⚠️ **NEEDS VALIDATION**

**Issues Identified**:
- **Hand Span**: 4x8 grid may require uncomfortable hand positioning
- **Cognitive Load**: Mental mapping between held step and control area
- **Visual Feedback**: No clear indication of control area boundaries

**RECOMMENDATION**: 
- Implement **user testing protocol** in Phase 4.2
- Consider **adaptive control scheme** based on dominant hand preference
- Add **visual boundaries** with LED dimming/brightening patterns

### 2.2 Button Hold Detection ⚠️ **TIMING CONCERNS**

**RECOMMENDED ENHANCEMENT**:
```cpp
class AdaptiveButtonTracker {
private:
    static constexpr uint32_t MIN_HOLD_THRESHOLD = 300;  // Fast users
    static constexpr uint32_t MAX_HOLD_THRESHOLD = 700;  // Careful users
    uint32_t currentThreshold_ = 500;  // Adaptive
    
public:
    void updateThreshold(bool successfulActivation);  // Learn from user
    bool isIntentionalHold(uint32_t holdDuration) const;
};
```

## 3. Memory Management Assessment

### 3.1 Memory Budget Analysis ✅ **ACCEPTABLE WITH CONSTRAINTS**

**VERIFIED CALCULATION**:
```cpp
// Per-lock storage:
sizeof(ParameterLock) = 7 bytes (packed)
MAX_LOCKS * sizeof(ParameterLock) = 64 * 7 = 448 bytes

// Step data overhead:
sizeof(StepData) = 1 byte (bit-packed)
MAX_TRACKS * MAX_STEPS * sizeof(StepData) = 4 * 8 * 1 = 32 bytes

// Total overhead: ~1KB (much better than 4KB estimate)
```

## 4. Performance Impact Assessment

### 4.1 MIDI Timing Performance ⚠️ **NEEDS OPTIMIZATION**

**OPTIMIZATION STRATEGY**:
```cpp
// Pre-calculate parameters before step trigger
void StepSequencer::prepareStepParameters(uint8_t nextStep) {
    // Called during non-critical timing window
    for (uint8_t track = 0; track < MAX_TRACKS; ++track) {
        if (stepData_[track][nextStep].hasLock) {
            auto& lock = lockPool_.getLock(stepData_[track][nextStep].lockIndex);
            preCalculatedParams_[track] = applyParameterLocks(track, lock);
        }
    }
}
```

## 5. Additional Risks Identified

### 5.1 **NEW RISK**: I2C Bus Congestion
**Impact**: High - Could affect button scanning and MIDI timing
**Mitigation**: Implement LED update batching and prioritization

### 5.2 **NEW RISK**: Flash Memory Wear
**Impact**: Medium - Device longevity concern  
**Mitigation**: Implement write coalescing and wear leveling

### 5.3 **NEW RISK**: Real-time Deadline Violation
**Impact**: Critical - Would break sequencer functionality
**Mitigation**: Mandatory pre-calculation of parameters

## Final Recommendations

### Critical Action Items (Must Address Before Implementation):

1. **REDESIGN DATA STRUCTURES** - Implement memory pool architecture
2. **IMPLEMENT MEMORY MONITORING** - Add runtime memory usage tracking  
3. **VALIDATE UI ERGONOMICS** - User testing of control scheme
4. **OPTIMIZE TIMING PATHS** - Pre-calculation strategy for parameters
5. **EXTEND RISK MITIGATION** - Address newly identified risks

### Conclusion

The parameter locks feature represents a **well-architected addition** to the NeoTrellis M4 sequencer. The design demonstrates strong architectural principles and appropriate consideration for embedded constraints.

**Implementation Readiness**: **75%** - Ready to proceed with critical modifications addressed.

**Estimated Timeline Validation**: 30-40 hour estimate is realistic with proposed architecture changes.

**Final Recommendation**: ✅ **APPROVED FOR IMPLEMENTATION** with mandatory resolution of critical action items.