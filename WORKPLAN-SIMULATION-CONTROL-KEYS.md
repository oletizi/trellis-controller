# Implementation Plan: Simulation Control Keys & Parameter Locks

!!!IMPORTANT: CRITICAL ISSUE

## CRITICAL: Architecturally Correct Solution (2025-09-10)

### Architectural Violation in Previous Solution

**CRITICAL FLAW**: The previously proposed solution (sections below) introduces a **fundamental architectural violation** by creating multiple state containers (`currentPressedKeys_`, `previousPressedKeys_`) instead of maintaining the project's core architectural principle: **single bitwise-encoded state variable**.

#### Project's Sophisticated State Architecture

This project uses a **sophisticated bitwise state encoding** where ALL state information is maintained in a single 64-bit `InputState` variable:

```cpp
// Core Architecture: Single Source of Truth
struct InputState {
    uint64_t state;  // ALL state fits in 64 bits
    // Bitwise encoding for: button states, timing, parameter locks, etc.
};

// Pure functional state management
class InputStateEncoder {
    InputState encodeInputEvent(const InputState& current, const InputEvent& event);
    // All state transitions are pure functions
};
```

#### Architectural Principles Violated

The previous solution violates these **critical architectural principles**:

1. **Single Source of Truth**: One 64-bit InputState contains everything
2. **Pure Functional**: State transitions are pure functions without side effects
3. **Platform Abstraction**: Input layers detect inputs, don't manage application state
4. **Bitwise Encoding**: All state fits in the structured 64-bit value
5. **No Distributed State**: No separate state containers across components

#### Correct Architectural Solution

**PRINCIPLE**: Fix INPUT DETECTION, not state management. The issue is in `CursesInputLayer`'s ability to detect physical key state, NOT in the state management architecture.

##### What CursesInputLayer Should Do (CORRECT):

```cpp
class CursesInputLayer {
private:
    // COMPLETELY STATELESS - NO memory of previous polls
    // NO physicalKeysDetected_, NO previousPhysicalKeys_
    
public:
    bool poll() override {
        // Step 1: Detect ONLY what ncurses currently sees (stateless)
        std::vector<int> currentlyDetectedKeys = detectCurrentPhysicalKeys();
        
        // Step 2: Report current state to InputStateEncoder
        // InputStateEncoder compares against its bitwise state to determine press/release
        reportCurrentState(currentlyDetectedKeys);
        
        // Step 3: InputStateEncoder handles ALL state management and event generation
        return hasEvents();
    }
};
```

##### What InputStateEncoder Does (UNCHANGED):

```cpp
class InputStateEncoder {
    // Maintains THE ONLY state container
    InputState encodeInputEvent(const InputState& current, const InputEvent& event) {
        // Pure functional state transition
        // ALL state management happens here
        // Returns new InputState with all timing, locks, etc.
    }
};
```

#### Key Architectural Corrections

1. **CursesInputLayer Responsibility**: ONLY detect physical key presence for event generation
2. **InputStateEncoder Responsibility**: ALL state management in single bitwise InputState
3. **No Duplicate State**: No `currentPressedKeys_` alongside `InputState` - that's distributed state
4. **Pure Functions**: State transitions remain pure without side effects
5. **Platform Independence**: Business logic stays in core, platform code only detects inputs

#### Implementation Correction - Truly Stateless CursesInputLayer

**The correct architecture flow**:
1. **CursesInputLayer.poll()**: "Keys currently detected: [a, b, c]" (no memory)
2. **InputStateEncoder.updateFromCurrentDetection()**: Compares [a,b,c] against bitwise state
3. **InputStateEncoder**: Generates PRESS/RELEASE events and updates bitwise state
4. **InputStateEncoder**: Maintains ALL timing, locks, gesture state in single 64-bit value

**Fix in CursesInputLayer.cpp**:

```cpp
bool CursesInputLayer::poll() {
    // STATELESS: Only detect what ncurses sees NOW, no memory of previous polls
    std::vector<uint8_t> currentlyDetectedButtons;
    
    // Detect what ncurses sees right now (pure sensor reading)
    nodelay(stdscr, TRUE);
    int key;
    while ((key = getch()) != ERR) {
        uint8_t buttonId = mapKey(key);
        if (buttonId != INVALID_BUTTON) {
            currentlyDetectedButtons.push_back(buttonId);
        }
    }
    
    // Pass current detection to InputStateEncoder for comparison
    // InputStateEncoder compares against its bitwise state to generate events
    if (inputStateEncoder_) {
        inputStateEncoder_->updateFromCurrentDetection(currentlyDetectedButtons, clock_->getCurrentTimeMs());
    }
    
    // NO state storage, NO previousPhysicalKeys_, completely stateless
    return inputStateEncoder_ && inputStateEncoder_->hasEvents();
}
```

**Critical Distinction**:
- `CursesInputLayer::poll()` = **Pure sensor reading** (what ncurses sees RIGHT NOW)
- `InputStateEncoder.state` = **Stateful comparison** (detects press/release by comparing current vs previous)
- `InputState.state` = **Application state** (what the sequencer uses)
- CursesInputLayer has ZERO memory, InputStateEncoder has ALL the state

#### Why This Maintains Architecture Integrity

1. **Single State Source**: InputState remains the ONLY application state container
2. **Pure Functions**: InputStateEncoder.encodeInputEvent() remains pure
3. **Clear Separation**: Input detection ≠ application state management
4. **Testability**: Easy to mock input detection without affecting state logic
5. **Platform Independence**: Core logic unchanged, only input detection improved

### Success Criteria for Architectural Correction

- [ ] CursesInputLayer has ZERO memory of previous polls (completely stateless)
- [ ] CursesInputLayer only reports "what keys are detected RIGHT NOW"
- [ ] InputStateEncoder does ALL state comparison (current vs previous)
- [ ] InputStateEncoder generates ALL press/release events
- [ ] InputStateEncoder remains the ONLY state management component  
- [ ] All state transitions remain pure functions in single 64-bit InputState
- [ ] Platform abstraction boundaries maintained
- [ ] Parameter locks work with proper hold detection
- [ ] No artificial timeouts or state tracking in input layer

**IMPLEMENTATION PRIORITY**: This architectural correction must be implemented BEFORE any other changes. The solution below violates core architectural principles and must be replaced.

---

## Overview

This document tracks the implementation of simulation control keys for the Trellis Controller, focusing on parameter
lock functionality and timing requirements.

## Addendum: Parameter Lock Issue Analysis (2025-09-10)

### Issue Discovered

During testing of parameter lock functionality in simulation, the following behavior was observed:

**Test Case**: Hold 'a' key + press 'b' key to trigger parameter lock mode
**Expected**: Parameter lock mode should activate when 'a' is held for ≥500ms, then 'b' should function as a control key
**Actual**: Parameter lock mode never activates; both keys are treated as independent button presses

### Root Cause Analysis

Investigation revealed a critical timing mismatch in the `CursesInputLayer` implementation:

1. **Parameter Lock Requirement**: `TrellisGestureDetector` requires a key to be held continuously for **≥500ms** to
   enter parameter lock mode
2. **Simulation Auto-Release**: `CursesInputLayer` automatically releases keys after **200ms** via
   `KEY_RELEASE_TIMEOUT_MS`
3. **Timing Conflict**: 200ms auto-release < 500ms parameter lock threshold = **impossible to achieve parameter locks**

#### Technical Details

```cpp
// In CursesInputLayer.cpp
const uint32_t KEY_RELEASE_TIMEOUT_MS = 200;  // ← Too short!

// In TrellisGestureDetector.cpp  
const uint32_t PARAMETER_LOCK_HOLD_TIME_MS = 500;  // ← Required threshold
```

**Sequence of Events**:

1. User presses 'a' at T=0ms
2. CursesInputLayer generates BUTTON_PRESS event
3. At T=200ms, CursesInputLayer auto-generates BUTTON_RELEASE event
4. At T=500ms, user is still physically holding 'a', but system thinks it's released
5. Parameter lock threshold never reached

## Critical Architectural Correction (2025-09-10)

### Fundamental Flaw in Timeout-Based Approach

**CRITICAL REALIZATION**: The previously proposed "increase timeout to 800ms" solution is **fundamentally incorrect**
and incompatible with parameter lock mode requirements. This correction addresses a critical architectural
misunderstanding.

#### Why ANY Timeout is Incompatible with Parameter Locks

**Parameter Lock Requirements**:

- Users must hold keys for potentially **many seconds** or **indefinitely**
- Parameter adjustment requires continuous key holding during the entire session
- Hardware behavior: Keys remain pressed until physically released
- No artificial timeouts - keys stay pressed as long as user holds them

**Timeout-Based Problems**:

1. **200ms timeout**: Too short for ≥500ms parameter lock detection
2. **800ms timeout**: Still incompatible - what if user holds for 10 seconds?
3. **10 second timeout**: Still fails - user may hold indefinitely
4. **ANY timeout**: Fundamentally wrong - creates artificial releases that don't exist in hardware

#### Hardware vs Simulation Behavior Mismatch

**Hardware (NeoTrellis)**:

```
Key pressed → BUTTON_PRESS event
Key held → [no events, button remains in pressed state]
Key released → BUTTON_RELEASE event
```

**Current Simulation (INCORRECT)**:

```
Key pressed → BUTTON_PRESS event
[Timeout expires] → BUTTON_RELEASE event (ARTIFICIAL!)
Key still held → [system thinks released, but user still holding]
```

**Required Simulation (CORRECT)**:

```
Key pressed → BUTTON_PRESS event  
Key held → [no events, button remains in pressed state]
Key released → BUTTON_RELEASE event (only when actually released)
```

### Correct Solution: Explicit Key State Tracking

**Core Principle**: Track which keys are **actually detected** by ncurses in each poll cycle, compare with previous poll
to detect **real state changes**.

#### Implementation Approach

```cpp
class CursesInputLayer {
private:
    std::unordered_set<int> currentPressedKeys_;   // Keys detected this poll
    std::unordered_set<int> previousPressedKeys_;  // Keys detected last poll
    std::unordered_map<int, uint32_t> pressStartTimes_; // When each key was first pressed
    
public:
    bool poll() override {
        // Step 1: Detect which keys are currently being pressed
        std::unordered_set<int> detectedKeys;
        
        // Non-blocking key detection - check all possible keys
        nodelay(stdscr, TRUE);  // Non-blocking mode
        int key;
        while ((key = getch()) != ERR) {
            detectedKeys.insert(key);
        }
        
        // Step 2: Compare with previous poll to detect state changes
        uint32_t currentTime = clock_->getCurrentTimeMs();
        
        // Generate PRESS events for newly detected keys
        for (int key : detectedKeys) {
            if (previousPressedKeys_.find(key) == previousPressedKeys_.end()) {
                // Key newly pressed
                InputEvent event;
                event.type = InputEventType::BUTTON_PRESS;
                event.buttonId = mapKeyToButtonId(key);
                event.timestamp = currentTime;
                eventQueue_.push_back(event);
                
                pressStartTimes_[key] = currentTime;
            }
        }
        
        // Generate RELEASE events for keys no longer detected
        for (int key : previousPressedKeys_) {
            if (detectedKeys.find(key) == detectedKeys.end()) {
                // Key released
                InputEvent event;
                event.type = InputEventType::BUTTON_RELEASE;
                event.buttonId = mapKeyToButtonId(key);
                event.timestamp = currentTime;
                eventQueue_.push_back(event);
                
                pressStartTimes_.erase(key);
            }
        }
        
        // Step 3: Update state for next poll
        previousPressedKeys_ = detectedKeys;
        currentPressedKeys_ = detectedKeys;
        
        return !eventQueue_.empty();
    }
};
```

#### Key Implementation Changes Required

**1. Remove ALL Timeout Logic**:

```cpp
// REMOVE these completely:
const uint32_t KEY_RELEASE_TIMEOUT_MS = 800;  // ← DELETE
std::unordered_map<int, uint32_t> keyPressStartTimes_; // ← REPURPOSE for gesture detection only
```

**2. Implement True State Detection**:

```cpp
// ADD proper state tracking:
std::unordered_set<int> currentPressedKeys_;   // What ncurses sees NOW
std::unordered_set<int> previousPressedKeys_;  // What ncurses saw LAST poll
```

**3. Fix Poll Logic**:

```cpp
bool poll() {
    // Check what keys ncurses currently detects
    detectCurrentlyPressedKeys();
    
    // Generate events only for actual state changes
    generatePressEvents();   // For newly detected keys
    generateReleaseEvents(); // For no-longer-detected keys
    
    // NO artificial timeouts - only real state changes
    return hasEvents();
}
```

### Why This Mirrors Hardware Exactly

**Hardware Behavior**: Button matrix scan detects which buttons are currently pressed, generates events only on state
changes.

**Corrected Simulation**: ncurses key detection finds which keys are currently pressed, generates events only on state
changes.

**Perfect Match**: Both systems track **actual current state** and generate events only on **real transitions**.

### Implementation Priority

**CRITICAL**: This completely replaces the flawed timeout approach. The timeout-based solution must be **completely
removed** and replaced with proper state tracking.

**Files to Modify**:

- `src/simulation/CursesInputLayer.cpp` - Remove timeout logic, add state tracking
- `src/simulation/CursesInputLayer.h` - Update member variables and interface
- Remove any references to `KEY_RELEASE_TIMEOUT_MS`

### Success Criteria (Corrected)

- [ ] Keys can be held indefinitely without artificial releases
- [ ] Parameter lock mode works with any hold duration (500ms to minutes)
- [ ] Only actual key releases generate BUTTON_RELEASE events
- [ ] Simulation behavior matches hardware exactly
- [ ] No timeout-based logic anywhere in input system

### Previous Analysis Remains Valid (Below Timeout Fix)

The analysis below correctly identifies implementation gaps, but the **timeout-based solution was fundamentally wrong**.
The state tracking approach above addresses the core issue while the detailed implementation plan below remains valuable
for the complete feature set.

### Implementation Priority

**High Priority**: This issue completely blocks parameter lock functionality in simulation, which is critical for
development and testing workflows.

**Risk Level**: Low - change is isolated to simulation layer with clear fallback behavior

**Effort**: Small - single constant change with optional enhancements

### Success Criteria

- [ ] Parameter lock mode activates when holding any key ≥500ms
- [ ] Control keys function correctly during parameter lock mode
- [ ] Normal key press/release behavior unchanged
- [ ] No input lag or responsiveness degradation
- [ ] Documentation updated to reflect timing requirements

IMPORTANT: Ignore everything below this line. It's for historical reference only.
__END__

## Problem Analysis

After reviewing the simulation documentation (`docs/SIMULATION.md`) and current implementation, there are several
critical discrepancies between the documented behavior and actual implementation:

### Current Architecture Overview

The current input processing pipeline is:

```
CursesInputLayer → InputController → InputStateProcessor → ControlMessages
```

However, there are **fundamental gaps** in the current implementation:

1. **Missing State Encoder**: CursesInputLayer generates raw keyboard events but has no proper InputStateEncoder
   integration
2. **Missing Control Key Logic**: Parameter lock control keys (b/g/n/h vs z/a/x/s) aren't implemented
3. **Incomplete Hold/Release Detection**: The current timing-based parameter lock entry isn't working as documented
4. **Missing Physical Key State Tracking**: No distinction between key press, hold, and release events

### Critical Discrepancies

#### 1. **Parameter Lock Entry Method**

- **Documented**: Hold key ≥500ms, keep holding while adjusting parameters, release to exit
- **Current**: Release after ≥500ms triggers entry, but then immediately exits

#### 2. **Control Key Mapping**

- **Documented**: Bank-specific control keys (left bank uses right bank controls, vice versa)
- **Current**: No control key differentiation implemented

#### 3. **Physical Key State Tracking**

- **Documented**: Keys should support press, hold (≥500ms), and release states
- **Current**: No proper physical key state tracking - every key input is treated as instantaneous

#### 4. **Hold State Management**

- **Documented**: Must keep trigger key held during entire parameter lock session
- **Current**: No continuous hold state tracking

## Implementation Strategy

### Phase 1: Fix Core State Architecture (Priority 1)

#### 1.1 Fix CursesInputLayer Physical Key State Tracking

**Issue**: CursesInputLayer treats every key input as instantaneous, but documentation requires proper
press/hold/release detection

**Files to modify**:

- `/Users/orion/work/trellis-controller/src/simulation/CursesInputLayer.cpp`
- `/Users/orion/work/trellis-controller/include/simulation/CursesInputLayer.h`

**Changes**:

- Implement proper hold tracking per key (track press timestamps)
- Generate proper BUTTON_PRESS events immediately on key press
- Generate proper BUTTON_RELEASE events on key release with accurate duration
- Add physical key state tracking to detect actual press/release transitions
- Support continuous key hold detection for parameter lock entry

#### 1.2 Implement InputStateEncoder Integration

**Issue**: CursesInputLayer has encoder hooks but they're not properly used in main.cpp

**Files to modify**:

- `/Users/orion/work/trellis-controller/src/simulation/main.cpp`

**Changes**:

- Create and inject InputStateEncoder into CursesInputLayer
- Ensure proper encoder initialization and dependency injection

#### 1.3 Fix InputStateProcessor Parameter Lock Logic

**Issue**: Current logic triggers entry on release, but documentation requires continuous hold

**Files to modify**:

- `/Users/orion/work/trellis-controller/src/core/InputStateProcessor.cpp`

**Changes**:

- Modify parameter lock entry detection to trigger on button HOLD reaching threshold (not release)
- Modify parameter lock exit to trigger on trigger button RELEASE (while in lock mode)
- Ensure lock state persists while trigger button remains held

### Phase 2: Implement Control Key Mapping (Priority 2)

#### 2.1 Add Bank Detection Logic

**Issue**: No logic to determine left vs right bank positioning

**New file**:

- `/Users/orion/work/trellis-controller/include/simulation/ControlKeyMapper.h`
- `/Users/orion/work/trellis-controller/src/simulation/ControlKeyMapper.cpp`

**Functionality**:

- Determine which bank (left/right) a button belongs to
- Map control keys based on trigger button bank
- Validate control key presses during parameter lock

#### 2.2 Add Control Key Processing

**Files to modify**:

- `/Users/orion/work/trellis-controller/src/core/InputStateProcessor.cpp`

**Changes**:

- Add control key mapping logic for parameter adjustment
- Implement bank-specific control key validation
- Map control keys to parameter types (pitch, velocity, etc.)

### Phase 3: Implement Proper Hold Detection (Priority 3)

#### 3.1 Add Real-time Hold Detection

**Issue**: Current system only detects holds on release, not during continuous press

**Files to modify**:

- `/Users/orion/work/trellis-controller/src/simulation/CursesInputLayer.cpp`
- `/Users/orion/work/trellis-controller/src/core/InputStateEncoder.cpp`

**Changes**:

- Track press timestamps per button
- Generate timing events for ongoing button presses
- Enable real-time parameter lock entry (while button still held)

#### 3.2 Fix Timing State Updates

**Files to modify**:

- `/Users/orion/work/trellis-controller/src/core/InputController.cpp`

**Changes**:

- Enable proper timing-based state updates in `updateTimingState()` method
- Ensure continuous parameter lock state while trigger button is held

### Phase 4: Enhanced User Experience (Priority 4)

#### 4.1 Add Visual Feedback

**Files to modify**:

- `/Users/orion/work/trellis-controller/src/core/StepSequencer.cpp` (if parameter lock display logic exists)

**Changes**:

- Add proper visual indicators for parameter lock mode
- Show active control keys during parameter lock
- Disable visual feedback for inactive keys during parameter lock

#### 4.2 Add Debug Output Enhancement

**Files to modify**:

- All modified files

**Changes**:

- Add comprehensive debug logging for parameter lock state transitions
- Log control key mappings and parameter adjustments
- Add visual debugging for hold state tracking

## Detailed Implementation Steps

### Step 1: Fix Core Hold/Release Detection

**Target File**: `/Users/orion/work/trellis-controller/src/simulation/CursesInputLayer.cpp`

**Core Issue**: Currently, the CursesInputLayer treats every key as an instantaneous event based on ncurses getch(), but
the documentation requires proper physical key press/hold/release detection. This needs to be replaced with proper key
state tracking that can distinguish between initial press, continuous hold, and release events.

**Required Changes**:

```cpp
class CursesInputLayer {
private:
    std::unordered_map<int, uint32_t> keyPressStartTimes_;  // Track press timestamps
    std::unordered_set<int> currentlyPressedKeys_;          // Track held keys
    
    // New methods:
    bool isKeyCurrentlyPressed(int key) const;
    uint32_t getKeyHoldDuration(int key) const;
    void updateHoldingKeys();  // Generate timing updates for held keys
};
```

### Step 2: Fix InputStateEncoder Integration

**Target File**: `/Users/orion/work/trellis-controller/src/simulation/main.cpp`

**Core Issue**: The InputStateEncoder exists but isn't properly integrated into the CursesInputLayer to provide proper
state management.

**Required Changes**:

```cpp
void setupInputController() {
    // Create InputStateEncoder
    auto encoder = std::make_unique<InputStateEncoder>(InputStateEncoder::Dependencies{
        .clock = clock_.get(),
        .debugOutput = debugOutput_.get()
    });
    
    // Set encoder on CursesInputLayer
    inputLayer->setInputStateEncoder(encoder.release()); // Transfer ownership
}
```

### Step 3: Fix Parameter Lock State Logic

**Target File**: `/Users/orion/work/trellis-controller/src/core/InputStateProcessor.cpp`

**Core Issue**: Parameter lock should be entered when button is HELD for ≥500ms (not on release), and should remain
active while button is held.

**Current Logic Problems**:

- Entry triggers on button release after long hold → **WRONG**
- Exit triggers when lock button is released → **CORRECT**
- No logic for continuous hold during parameter lock → **MISSING**

**Required Changes**:

```cpp
bool InputStateProcessor::isParameterLockEntry(const InputState& current, const InputState& previous) const {
    // Entry: Transition from not-locked to locked
    // This should be triggered by InputStateEncoder when hold threshold is reached
    // while button is STILL HELD
    return current.isParameterLockActive() && !previous.isParameterLockActive();
}

bool InputStateProcessor::isParameterLockExit(const InputState& current, const InputState& previous) const {
    // Exit: Release of lock button while in parameter lock mode
    if (!previous.isParameterLockActive()) return false;
    
    uint8_t lockButtonId = previous.getLockButtonId();
    bool wasPressed = previous.isButtonPressed(lockButtonId);
    bool nowReleased = !current.isButtonPressed(lockButtonId);
    
    return wasPressed && nowReleased;
}
```

### Step 4: Implement Control Key Mapping

**New Files**:

- `/Users/orion/work/trellis-controller/include/simulation/ControlKeyMapper.h`
- `/Users/orion/work/trellis-controller/src/simulation/ControlKeyMapper.cpp`

**Functionality**:

```cpp
class ControlKeyMapper {
public:
    enum class Bank { LEFT, RIGHT };
    enum class ControlFunction { 
        PITCH_DOWN, PITCH_UP, 
        VELOCITY_DOWN, VELOCITY_UP 
    };
    
    Bank getButtonBank(uint8_t buttonId) const;
    Bank getOppositeBank(Bank bank) const;
    std::optional<ControlFunction> getControlFunction(uint8_t buttonId, Bank activeBank) const;
    bool isControlKeyActive(uint8_t buttonId, uint8_t lockButtonId) const;
};
```

### Step 5: Integration Points

**Files to modify for integration**:

1. **InputStateProcessor.cpp**: Add control key processing logic
2. **StepSequencer.cpp**: Add parameter adjustment handling
3. **CursesInputLayer.cpp**: Add hold state monitoring
4. **InputController.cpp**: Ensure timing updates work properly

## Testing Strategy

### Unit Tests Required:

1. **CursesInputLayer hold detection** - Test press/hold/release timing
2. **InputStateEncoder integration** - Test state transitions
3. **InputStateProcessor parameter lock logic** - Test entry/exit conditions
4. **ControlKeyMapper functionality** - Test bank detection and key mapping

### Integration Tests Required:

1. **Complete parameter lock flow** - Hold button → adjust parameters → release
2. **Control key validation** - Proper bank switching and key activation
3. **Visual feedback** - Parameter lock indicators and key highlighting

### Manual Testing Scenarios:

```
Scenario 1: Basic Parameter Lock
1. Run simulation
2. Hold 'q' for >500ms (should enter parameter lock, keep button held)
3. While holding 'q', press '5' (should adjust pitch up)
4. While holding 'q', press '6' (should adjust pitch up again)  
5. Release 'q' (should exit parameter lock)

Scenario 2: Bank Switching
1. Hold 'g' (right bank button) for >500ms
2. Control keys should be left bank: z/a/x/s
3. Press 'z' (should adjust pitch down)
4. Press 'a' (should adjust pitch up)
5. Release 'g' (should exit parameter lock)
```

## Success Criteria

### Phase 1 Complete:

- [ ] Parameter lock can be entered by holding any grid key for ≥500ms
- [ ] Parameter lock remains active while trigger key is held
- [ ] Parameter lock exits immediately when trigger key is released
- [ ] Debug output clearly shows state transitions

### Phase 2 Complete:

- [ ] Control keys are correctly mapped based on trigger button bank
- [ ] Left bank triggers use right bank control keys (5678/tyui/ghjk/bnm,)
- [ ] Right bank triggers use left bank control keys (1234/qwer/asdf/zxcv)
- [ ] Only control keys from opposite bank respond during parameter lock

### Phase 3 Complete:

- [ ] Real-time hold detection works during continuous key press
- [ ] Parameter lock can be entered without releasing trigger key
- [ ] Timing updates properly maintain lock state during hold

### Phase 4 Complete:

- [ ] Visual feedback shows parameter lock mode activation
- [ ] Control keys are highlighted during parameter lock
- [ ] Non-control keys are visually dimmed during parameter lock
- [ ] Clear console debug output for all state changes

## Risk Mitigation

### High-Risk Changes:

1. **CursesInputLayer hold detection** - Risk of breaking existing key input
    - **Mitigation**: Maintain backward compatibility, add extensive unit tests

2. **InputStateProcessor logic changes** - Risk of breaking existing gesture detection
    - **Mitigation**: Test all existing functionality, maintain existing test cases

3. **Integration with InputStateEncoder** - Risk of ownership/lifecycle issues
    - **Mitigation**: Follow RAII patterns, clear ownership semantics

### Testing Dependencies:

- Requires properly functioning CursesDisplay for simulation environment
- Requires InputController integration testing
- Requires StepSequencer parameter handling (may need separate implementation)

## Implementation Order Priority:

1. **Phase 1.1-1.3** (Critical): Fix core state architecture
2. **Phase 2.1-2.2** (High): Add control key mapping
3. **Phase 3.1-3.2** (Medium): Implement real-time hold detection
4. **Phase 4.1-4.2** (Low): Enhanced user experience

This plan addresses all documented parameter lock behavior and ensures the simulation conforms exactly to the
specification in `docs/SIMULATION.md`.