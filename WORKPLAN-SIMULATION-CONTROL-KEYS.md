# Implementation Plan: Simulation Control Keys & Parameter Locks

!!!IMPORTANT: CRITICAL ISSUE

## Overview

This document tracks the implementation of simulation control keys for the Trellis Controller, focusing on parameter lock functionality and timing requirements.

## Addendum: Parameter Lock Issue Analysis (2025-09-10)

### Issue Discovered

During testing of parameter lock functionality in simulation, the following behavior was observed:

**Test Case**: Hold 'a' key + press 'b' key to trigger parameter lock mode
**Expected**: Parameter lock mode should activate when 'a' is held for ≥500ms, then 'b' should function as a control key
**Actual**: Parameter lock mode never activates; both keys are treated as independent button presses

### Root Cause Analysis

Investigation revealed a critical timing mismatch in the `CursesInputLayer` implementation:

1. **Parameter Lock Requirement**: `TrellisGestureDetector` requires a key to be held continuously for **≥500ms** to enter parameter lock mode
2. **Simulation Auto-Release**: `CursesInputLayer` automatically releases keys after **200ms** via `KEY_RELEASE_TIMEOUT_MS`
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

### Recommended Solution

**Primary Fix**: Increase `KEY_RELEASE_TIMEOUT_MS` to allow parameter lock detection:

```cpp
// In CursesInputLayer.cpp - increase timeout
const uint32_t KEY_RELEASE_TIMEOUT_MS = 800;  // 300ms safety margin above 500ms threshold
```

**Enhanced Solution**: Make timeout configurable with intelligent defaults:

```cpp
// Configuration-based approach
class CursesInputLayer {
private:
    uint32_t keyReleaseTimeoutMs;
    
public:
    // Constructor with configurable timeout
    explicit CursesInputLayer(uint32_t keyTimeout = 800) 
        : keyReleaseTimeoutMs(keyTimeout) {}
        
    // Allow runtime adjustment for testing
    void setKeyReleaseTimeout(uint32_t timeoutMs) {
        keyReleaseTimeoutMs = timeoutMs;
    }
};
```

### Implementation Approach

#### Phase 1: Immediate Fix (Minimal Change)
1. **File**: `src/simulation/CursesInputLayer.cpp`
2. **Change**: Update `KEY_RELEASE_TIMEOUT_MS` from `200` to `800`
3. **Testing**: Verify parameter lock mode activates with hold+press sequences
4. **Validation**: Ensure normal key press/release still works correctly

#### Phase 2: Enhanced Solution (Optional)
1. **Add configurability** to CursesInputLayer constructor
2. **Update Dependencies struct** to pass timeout configuration
3. **Add unit tests** for different timeout scenarios
4. **Document** the timing relationship in code comments

### Compatibility Considerations

**Existing Functionality Preserved**:
- Normal key press/release detection unaffected
- Quick tap gestures (< 200ms) still work correctly
- Input event pipeline remains unchanged
- No breaking changes to public interfaces

**Performance Impact**:
- Minimal: Only affects timeout comparison logic
- Memory: No additional allocations
- Real-time safety: Maintains deterministic execution

### Testing Strategy

**Manual Testing**:
1. Hold 'a' for 1 second, press 'b' → Should enter parameter lock mode
2. Quick tap 'a' → Should toggle step normally
3. Hold 'q' for 1 second, press multiple right-bank keys → Should adjust parameters
4. Rapid key sequences → Should not cause input lag

**Automated Testing**:
```cpp
// Test case for parameter lock timing
TEST_CASE("CursesInputLayer parameter lock timing") {
    CursesInputLayer input(800);  // 800ms timeout
    
    // Simulate holding key for parameter lock duration
    input.simulateKeyHold('a', 600);  // > 500ms threshold
    
    InputEvent event;
    REQUIRE(input.getNextEvent(event));
    REQUIRE(event.type == InputEventType::BUTTON_PRESS);
    
    // Verify no auto-release before timeout
    REQUIRE_FALSE(input.hasEvents());  // No release event yet
}
```

### Implementation Priority

**High Priority**: This issue completely blocks parameter lock functionality in simulation, which is critical for development and testing workflows.

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

**Issue**: CursesInputLayer treats every key input as instantaneous, but documentation requires proper press/hold/release detection

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

**Core Issue**: Currently, the CursesInputLayer treats every key as an instantaneous event based on ncurses getch(), but the documentation requires proper physical key press/hold/release detection. This needs to be replaced with proper key state tracking that can distinguish between initial press, continuous hold, and release events.

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

**Core Issue**: The InputStateEncoder exists but isn't properly integrated into the CursesInputLayer to provide proper state management.

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