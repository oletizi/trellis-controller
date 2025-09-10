# SHIFT-Based Parameter Lock Implementation Plan

## Executive Summary

This document outlines a complete redesign of parameter lock controls using **SHIFT modifier keys** instead of
timing-based detection. This eliminates all race conditions, ncurses timing limitations, and provides deterministic,
user-friendly parameter lock functionality.

## Architecture Overview

### Current Problems with Timing-Based Approach

**CRITICAL FLAWS IDENTIFIED:**

- **Timing Race Conditions**: 200ms timeout < 500ms parameter lock threshold = impossible locks
- **ncurses Limitations**: Cannot reliably detect key holds beyond system repeat rates
- **Non-Deterministic**: Human timing variability makes parameter locks unreliable
- **Complex State Management**: Multiple timeout states create architectural complexity

### NEW APPROACH: SHIFT-Based Parameter Locks

**CORE PRINCIPLE**: Replace all timing-based logic with explicit SHIFT modifier key detection

#### Key Benefits:

- **Deterministic**: SHIFT+key always enters parameter lock, key alone always exits
- **No Timing Dependencies**: Works perfectly with ncurses key repeat behavior
- **User Friendly**: Clear, predictable interaction model familiar to all users
- **Architecturally Simple**: Clean state transitions with explicit user intent
- **Platform Agnostic**: Works identically on hardware and simulation

## Complete Key Mapping Design

### QWERTY-to-Trellis Layout

```
NeoTrellis 4x8 Grid Layout:
Row 0 (Track 0): [00][01][02][03][04][05][06][07] 
Row 1 (Track 1): [08][09][10][11][12][13][14][15]
Row 2 (Track 2): [16][17][18][19][20][21][22][23]
Row 3 (Track 3): [24][25][26][27][28][29][30][31]

QWERTY Keyboard Mapping:
Row 0 (RED):    1  2  3  4  5  6  7  8
Row 1 (GREEN):  q  w  e  r  t  y  u  i  
Row 2 (BLUE):   a  s  d  f  g  h  j  k
Row 3 (YELLOW): z  x  c  v  b  n  m  ,
```

### SHIFT-Based Parameter Lock Controls

#### Parameter Lock Entry/Exit Pattern:

```
SHIFT+<key> = Enter parameter lock mode for that specific step
<key> alone = Exit parameter lock mode and return to normal operation

Examples:
- SHIFT+q = Enter parameter lock mode for Track 1, Step 0 (green track, first step)
- q (alone) = Exit parameter lock mode  
- SHIFT+5 = Enter parameter lock mode for Track 0, Step 4 (red track, fifth step)
- 5 (alone) = Exit parameter lock mode
```

#### Bank-Aware Control Keys During Parameter Lock Mode:

```
The control keys available during parameter lock mode depend on which BANK the
parameter lock key belongs to:

Left Bank Parameter Lock Keys: <1-4 qwer asdf zxcv> (16 keys)
Right Bank Parameter Lock Keys: <5678 tyui ghjk bnm,> (16 keys)

When parameter lock key is in LEFT bank → control keys are in RIGHT bank:
- b = Decrement MIDI note value
- g = Increment MIDI note value  
- n = Decrement MIDI velocity value
- h = Increment MIDI velocity value

When parameter lock key is in RIGHT bank → control keys are in LEFT bank:
- z = Decrement MIDI note value
- a = Increment MIDI note value
- x = Decrement MIDI velocity value
- s = Increment MIDI velocity value

NOTE: Only these 4 control functions are authorized. All other keys
are inactive during parameter lock mode.
```

## Implementation Architecture

### 1. Enhanced Key Detection in CursesInputLayer

**NEW FUNCTIONALITY**: Detect and report SHIFT state alongside key presses

```cpp
// Enhanced CursesInputLayer changes
class CursesInputLayer : public IInputLayer {
private:
    bool shiftPressed_ = false;           // Current SHIFT key state
    std::set<int> currentKeysPressed_;    // All currently pressed keys
    
    // New methods for SHIFT detection
    bool isShiftKey(int key) const;       // Detect platform SHIFT keys
    void updateShiftState(int key, bool pressed);  // Track SHIFT state
    
public:
    // Enhanced event generation
    InputEvent createShiftKeyEvent(uint8_t buttonId, uint32_t timestamp, bool shiftModifier) const;
};

// Key processing with SHIFT awareness
void CursesInputLayer::processKeyInput(int key) {
    // 1. Update SHIFT state first
    if (isShiftKey(key)) {
        updateShiftState(key, true);
        return; // Don't generate button events for SHIFT itself
    }
    
    // 2. Process regular keys with current SHIFT state
    uint8_t row, col;
    if (getKeyMapping(key, row, col)) {
        uint8_t buttonId = getButtonIndex(row, col);
        
        // Generate press event with SHIFT modifier information
        InputEvent event = createShiftKeyEvent(buttonId, clock_->getCurrentTime(), shiftPressed_);
        eventQueue_.push(event);
        
        // Track key for release detection
        currentKeysPressed_.insert(key);
    }
}
```

### 2. SHIFT-Aware InputEvent Structure

**ENHANCED**: InputEvent carries SHIFT modifier information

```cpp
// Enhanced InputEvent with SHIFT support
struct InputEvent {
    enum Type {
        BUTTON_PRESS,
        BUTTON_RELEASE, 
        SHIFT_BUTTON_PRESS,    // NEW: Button press with SHIFT held
        SHIFT_BUTTON_RELEASE   // NEW: Button release with SHIFT held
    };
    
    Type type;
    uint8_t buttonId;
    uint32_t timestamp;
    int32_t param1;           // Duration for releases, modifier flags for presses
    int32_t param2;           // Future use
    
    // Convenience methods
    bool hasShiftModifier() const { return type == SHIFT_BUTTON_PRESS || type == SHIFT_BUTTON_RELEASE; }
    uint32_t getPressDuration() const { return static_cast<uint32_t>(param1); }
};
```

### 3. SHIFT-Based GestureDetector

**COMPLETE REWRITE**: Replace timing-based logic with SHIFT-based detection

```cpp
class ShiftBasedGestureDetector : public IGestureDetector {
private:
    struct ButtonState {
        bool pressed = false;
        bool isParameterLockTrigger = false;  // Was this button used to enter param lock?
    };
    
    std::array<ButtonState, 32> buttonStates_;
    
    struct ParameterLockState {
        bool active = false;
        uint8_t triggerButtonId = 255;  // Which button triggered parameter lock
        uint8_t lockedTrack = 255;
        uint8_t lockedStep = 255;
    } paramLockState_;
    
public:
    uint8_t processInputEvent(const InputEvent& event, 
                             std::vector<ControlMessage::Message>& messages) override {
        
        switch (event.type) {
        case InputEvent::SHIFT_BUTTON_PRESS:
            return handleShiftButtonPress(event, messages);
            
        case InputEvent::BUTTON_PRESS:
            if (paramLockState_.active) {
                return handleParameterControlPress(event, messages);
            } else {
                return handleNormalButtonPress(event, messages);
            }
            
        case InputEvent::BUTTON_RELEASE:
            return handleButtonRelease(event, messages);
            
        default:
            return 0;
        }
    }
    
private:
    uint8_t handleShiftButtonPress(const InputEvent& event, 
                                  std::vector<ControlMessage::Message>& messages) {
        // SHIFT+Button = Enter parameter lock for this step
        uint8_t track, step;
        buttonIndexToTrackStep(event.buttonId, track, step);
        
        // Exit any current parameter lock first
        if (paramLockState_.active) {
            messages.push_back(ControlMessage::Message::exitParamLock(event.timestamp));
        }
        
        // Enter parameter lock for new step
        paramLockState_.active = true;
        paramLockState_.triggerButtonId = event.buttonId;  
        paramLockState_.lockedTrack = track;
        paramLockState_.lockedStep = step;
        
        // Mark button as parameter lock trigger
        buttonStates_[event.buttonId].isParameterLockTrigger = true;
        buttonStates_[event.buttonId].pressed = true;
        
        messages.push_back(ControlMessage::Message::enterParamLock(track, step, event.timestamp));
        
        debugLog("SHIFT-based parameter lock entered: track=" + std::to_string(track) + 
                 ", step=" + std::to_string(step));
        
        return 1; // One message generated
    }
    
    uint8_t handleParameterControlPress(const InputEvent& event,
                                       std::vector<ControlMessage::Message>& messages) {
        // Any button press during parameter lock = parameter adjustment
        uint8_t paramType;
        int8_t delta;
        
        if (mapButtonToParameterControl(event.buttonId, paramType, delta)) {
            messages.push_back(ControlMessage::Message::adjustParameter(paramType, delta, event.timestamp));
            
            debugLog("Parameter adjustment: type=" + std::to_string(paramType) + 
                     ", delta=" + std::to_string(delta));
            
            return 1;
        }
        
        return 0;
    }
    
    uint8_t handleNormalButtonPress(const InputEvent& event,
                                   std::vector<ControlMessage::Message>& messages) {
        // Normal button press - just track state for step toggle on release
        buttonStates_[event.buttonId].pressed = true;
        return 0; // No immediate message - wait for release
    }
    
    uint8_t handleButtonRelease(const InputEvent& event,
                               std::vector<ControlMessage::Message>& messages) {
        
        ButtonState& buttonState = buttonStates_[event.buttonId];
        
        if (buttonState.isParameterLockTrigger) {
            // Release of parameter lock trigger button = exit parameter lock
            paramLockState_.active = false;
            paramLockState_.triggerButtonId = 255;
            
            buttonState.isParameterLockTrigger = false;
            buttonState.pressed = false;
            
            messages.push_back(ControlMessage::Message::exitParamLock(event.timestamp));
            
            debugLog("Parameter lock exited via trigger button release");
            
            return 1;
        } else if (buttonState.pressed && !paramLockState_.active) {
            // Normal step toggle (only when not in parameter lock mode)
            uint8_t track, step;
            buttonIndexToTrackStep(event.buttonId, track, step);
            
            buttonState.pressed = false;
            
            messages.push_back(ControlMessage::Message::toggleStep(track, step, event.timestamp));
            
            debugLog("Step toggle: track=" + std::to_string(track) + ", step=" + std::to_string(step));
            
            return 1;
        }
        
        // Release during parameter lock mode (not trigger button) = no action
        buttonState.pressed = false;
        return 0;
    }
    
    bool mapButtonToParameterControl(uint8_t buttonId, uint8_t& paramType, int8_t& delta) const {
        // Bank-aware parameter control mapping
        // Determine which bank the current parameter lock trigger is in
        bool triggerInLeftBank = isLeftBankButton(paramLockState_.triggerButtonId);
        
        // Control keys are in the opposite bank from the trigger
        if (triggerInLeftBank) {
            // Trigger in left bank → control keys in right bank
            switch (buttonId) {
            case 28: // 'b' = decrement MIDI note
                paramType = 1; // MIDI_NOTE parameter
                delta = -1;
                return true;
            case 20: // 'g' = increment MIDI note
                paramType = 1; // MIDI_NOTE parameter
                delta = +1;
                return true;
            case 29: // 'n' = decrement MIDI velocity
                paramType = 2; // VELOCITY parameter
                delta = -10;
                return true;
            case 21: // 'h' = increment MIDI velocity
                paramType = 2; // VELOCITY parameter
                delta = +10;
                return true;
            }
        } else {
            // Trigger in right bank → control keys in left bank
            switch (buttonId) {
            case 24: // 'z' = decrement MIDI note
                paramType = 1; // MIDI_NOTE parameter
                delta = -1;
                return true;
            case 16: // 'a' = increment MIDI note
                paramType = 1; // MIDI_NOTE parameter
                delta = +1;
                return true;
            case 25: // 'x' = decrement MIDI velocity
                paramType = 2; // VELOCITY parameter
                delta = -10;
                return true;
            case 17: // 's' = increment MIDI velocity
                paramType = 2; // VELOCITY parameter
                delta = +10;
                return true;
            }
        }
        
        return false; // Button doesn't map to a parameter control
    }
    
    bool isLeftBankButton(uint8_t buttonId) const {
        // Left bank: buttons 0-3, 8-11, 16-19, 24-27 (columns 0-3)
        uint8_t column = buttonId % 8;
        return column < 4;
    }
};
```

### 4. Platform SHIFT Key Detection

**PLATFORM-SPECIFIC**: Detect SHIFT keys across different platforms

```cpp
bool CursesInputLayer::isShiftKey(int key) const {
    // ncurses SHIFT key detection
    switch (key) {
    case KEY_SHIFT:        // If ncurses provides this
    case KEY_LEFT_SHIFT:   // Left shift key
    case KEY_RIGHT_SHIFT:  // Right shift key
        return true;
    default:
        // Also detect capital letters as implicit shift
        return (key >= 'A' && key <= 'Z');
    }
}

void CursesInputLayer::updateShiftState(int key, bool pressed) {
    if (key >= 'A' && key <= 'Z') {
        // Capital letter implies shift is pressed for this key
        shiftPressed_ = pressed;
    } else if (isShiftKey(key)) {
        // Explicit shift key
        shiftPressed_ = pressed;
    }
}

// Enhanced key processing with SHIFT detection
void CursesInputLayer::processKeyInput(int key) {
    bool isCapital = (key >= 'A' && key <= 'Z');
    bool isLower = (key >= 'a' && key <= 'z');
    bool isNumber = (key >= '0' && key <= '9');
    
    // Convert capital letters to lowercase for mapping, but preserve SHIFT info
    int mappingKey = key;
    bool hasShiftModifier = isCapital;
    
    if (isCapital) {
        mappingKey = key - 'A' + 'a'; // Convert to lowercase for mapping lookup
    }
    
    uint8_t row, col;
    if (getKeyMapping(mappingKey, row, col)) {
        uint8_t buttonId = getButtonIndex(row, col);
        uint32_t timestamp = clock_->getCurrentTime();
        
        // Generate appropriate event type based on SHIFT state
        InputEvent::Type eventType = hasShiftModifier ? InputEvent::SHIFT_BUTTON_PRESS : InputEvent::BUTTON_PRESS;
        
        InputEvent event(eventType, buttonId, timestamp, hasShiftModifier ? 1 : 0, 0);
        
        if (eventQueue_.size() < config_.performance.eventQueueSize) {
            eventQueue_.push(event);
            
            debugLog("Key '" + std::string(1, key) + "' -> " + 
                     (hasShiftModifier ? "SHIFT+" : "") + "button " + std::to_string(buttonId));
        } else {
            status_.eventsDropped++;
        }
    }
}
```

## User Experience Design

### Visual Feedback During Parameter Lock

**ENHANCED DISPLAY**: Clear indication of parameter lock mode and available controls

```cpp
class ParameterLockDisplay {
public:
    void updateParameterLockDisplay(IDisplay* display, const ParameterLockState& state) {
        if (!state.active) return;
        
        // 1. Highlight the locked step in distinctive color
        uint8_t lockedButtonId = state.triggerButtonId;
        display->setPixel(lockedButtonId, PARAM_LOCK_ACTIVE_COLOR); // Bright white
        
        // 2. Highlight available control keys
        highlightControlKeys(display);
        
        // 3. Dim non-control keys
        dimInactiveKeys(display, state);
    }
    
private:
    static constexpr uint32_t PARAM_LOCK_ACTIVE_COLOR = 0xFFFFFF;  // White
    static constexpr uint32_t CONTROL_KEY_COLOR = 0x00FF00;       // Green  
    static constexpr uint32_t DIMMED_KEY_COLOR = 0x202020;        // Dark gray
    
    void highlightControlKeys(IDisplay* display, const ParameterLockState& state) {
        // Highlight control keys based on which bank the trigger is in
        bool triggerInLeftBank = isLeftBankButton(state.triggerButtonId);
        
        if (triggerInLeftBank) {
            // Trigger in left bank → control keys in right bank
            display->setPixel(28, CONTROL_KEY_COLOR); // 'b' = decrement note
            display->setPixel(20, CONTROL_KEY_COLOR); // 'g' = increment note
            display->setPixel(29, CONTROL_KEY_COLOR); // 'n' = decrement velocity
            display->setPixel(21, CONTROL_KEY_COLOR); // 'h' = increment velocity
        } else {
            // Trigger in right bank → control keys in left bank
            display->setPixel(24, CONTROL_KEY_COLOR); // 'z' = decrement note
            display->setPixel(16, CONTROL_KEY_COLOR); // 'a' = increment note
            display->setPixel(25, CONTROL_KEY_COLOR); // 'x' = decrement velocity
            display->setPixel(17, CONTROL_KEY_COLOR); // 's' = increment velocity
        }
    }
    
    bool isLeftBankButton(uint8_t buttonId) const {
        // Left bank: buttons 0-3, 8-11, 16-19, 24-27 (columns 0-3)
        uint8_t column = buttonId % 8;
        return column < 4;
    }
    
    void dimInactiveKeys(IDisplay* display, const ParameterLockState& state) {
        for (uint8_t i = 0; i < 32; ++i) {
            if (i != state.triggerButtonId && !isControlKey(i)) {
                display->setPixel(i, DIMMED_KEY_COLOR);
            }
        }
    }
};
```

### Usage Examples

#### Example 1: Left Bank Parameter Lock Usage

```
1. User presses SHIFT+q (holds SHIFT, presses q)
   → Enters parameter lock mode for Track 1, Step 0
   → 'q' is in LEFT bank, so control keys are in RIGHT bank
   → LED grid shows: step (1,0) highlighted white, control keys (b,g,n,h) highlighted green

2. User presses 'g' (increment note control for right bank)  
   → MIDI note for step (1,0) increases by 1 semitone
   → No change in LED display (still in parameter lock mode)

3. User presses 'b' (decrement note control for right bank)
   → MIDI note for step (1,0) decreases by 1 semitone  
   → No change in LED display

4. User presses 'q' (without SHIFT)
   → Exits parameter lock mode
   → LED grid returns to normal sequencer display
```

#### Example 2: Right Bank Parameter Lock Usage

```
1. SHIFT+5 → Enter parameter lock for Track 0, Step 4
   → '5' is in RIGHT bank, so control keys are in LEFT bank
   → LED grid shows: step (0,4) highlighted white, control keys (z,a,x,s) highlighted green
2. a,a,a → Increase MIDI note by 3 semitones (using left bank controls)
3. s,s → Increase velocity by 20 (using left bank controls)
4. z → Decrease MIDI note by 1 semitone (using left bank controls)
5. Press '5' → Exit parameter lock mode
```

#### Example 3: Bank Switching Example

```
1. SHIFT+a → Enter parameter lock for Track 2, Step 0 (LEFT bank)
   → Control keys are in RIGHT bank (b,g,n,h)
2. g,g → Adjust note up using right bank controls
3. SHIFT+5 → Exit previous lock, enter parameter lock for Track 0, Step 4 (RIGHT bank)
   → Control keys switch to LEFT bank (z,a,x,s)
4. s,s,s → Adjust velocity up using left bank controls
5. Press '5' → Exit parameter lock mode
```

## Implementation Benefits

### 1. Eliminates All Timing Issues

- **No race conditions**: SHIFT state is deterministic
- **No timeout logic**: No artificial time-based releases
- **ncurses compatible**: Works perfectly with key repeat behavior
- **Platform agnostic**: Same logic works on hardware and simulation

### 2. Superior User Experience

- **Predictable**: SHIFT+key always enters, key alone always exits
- **Discoverable**: Standard keyboard modifier pattern familiar to all users
- **Fast**: No waiting for timeout thresholds
- **Precise**: No timing variability - works the same every time

### 3. Architectural Simplification

- **Stateless input layer**: CursesInputLayer just detects keys and SHIFT state
- **Clear separation**: GestureDetector handles all semantic interpretation
- **Testable**: Easy to mock SHIFT+key combinations
- **Maintainable**: Simple state machine with explicit transitions

### 4. Future Extensibility

- **Multiple modifiers**: Could add CTRL+key, ALT+key for different functions
- **Context-sensitive**: Different control mappings per parameter lock context
- **Scalable**: Easy to add new parameter types and control mappings

## Migration Strategy

### Phase 1: Core SHIFT Detection (Priority 1)

1. **Enhance CursesInputLayer**: Add SHIFT key detection and event generation
2. **Extend InputEvent**: Add SHIFT_BUTTON_PRESS/SHIFT_BUTTON_RELEASE types
3. **Test SHIFT detection**: Ensure capital letters and explicit SHIFT keys work

### Phase 2: SHIFT-Based GestureDetector (Priority 1)

1. **Create ShiftBasedGestureDetector**: Complete rewrite of gesture detection logic
2. **Remove timing logic**: Eliminate all timeout-based parameter lock code
3. **Implement control mapping**: Map buttons to parameter adjustment functions

### Phase 3: Integration & Testing (Priority 1)

1. **Update InputController**: Use new gesture detector
2. **Test complete flow**: SHIFT+key → parameter adjustment → key → exit
3. **Visual feedback**: Implement parameter lock display enhancements

### Phase 4: Documentation & Polish (Priority 2)

1. **Update SIMULATION.md**: Document new SHIFT-based controls
2. **Add usage examples**: Comprehensive usage guide
3. **Performance testing**: Ensure no latency or responsiveness issues

## Success Criteria

### Functional Requirements

- [ ] SHIFT+any step key enters parameter lock mode for that specific step
- [ ] Any step key (without SHIFT) exits parameter lock mode
- [ ] Control keys (b,n,v,m,g,h,f,j) adjust parameters during parameter lock
- [ ] Visual feedback clearly shows parameter lock state and available controls
- [ ] No timing dependencies - all behavior is deterministic
- [ ] Works reliably in ncurses simulation environment

### Performance Requirements

- [ ] Zero latency - immediate response to SHIFT+key combinations
- [ ] No dropped events - all SHIFT+key presses properly detected
- [ ] Smooth operation - no stuttering or delayed response
- [ ] Memory efficient - no memory leaks or excessive allocations

### User Experience Requirements

- [ ] Intuitive - parameter lock behavior is predictable and discoverable
- [ ] Consistent - same behavior every time, regardless of timing
- [ ] Forgiving - easy to exit parameter lock mode
- [ ] Visual - clear feedback about current mode and available actions

## Risk Assessment

### Low Risk Items

- **SHIFT detection**: Standard keyboard functionality, well-supported
- **Event generation**: Simple extension of existing InputEvent system
- **Control mapping**: Straightforward button-to-parameter mapping

### Medium Risk Items

- **Integration complexity**: Need to ensure clean integration with existing sequencer
- **Visual feedback**: May require display layer changes for highlighting
- **Testing scope**: Need comprehensive testing of all key combinations

### Mitigation Strategies

- **Incremental development**: Implement and test each phase independently
- **Backward compatibility**: Maintain existing interfaces during transition
- **Comprehensive testing**: Unit tests for all SHIFT+key combinations
- **User testing**: Validate user experience with actual users

## Technical Implementation Notes

### Memory Requirements

- **Minimal overhead**: Only need to track current SHIFT state (1 bool)
- **No timing buffers**: Eliminate timeout tracking structures
- **Efficient mapping**: Simple lookup tables for button-to-parameter mapping

### Performance Characteristics

- **O(1) SHIFT detection**: Simple boolean check per key event
- **O(1) parameter mapping**: Direct lookup table access
- **No polling overhead**: Event-driven design with no background timing

### Platform Compatibility

- **ncurses**: Works with standard capital letter detection
- **Hardware**: Can be extended to use physical modifier keys
- **Cross-platform**: Same logic works on all supported platforms

## Conclusion

The SHIFT-based parameter lock implementation represents a **fundamental architectural improvement** that eliminates all
timing-related issues while providing a superior user experience. By replacing complex timing logic with simple,
deterministic SHIFT key detection, we achieve:

1. **100% reliability** - No race conditions or timing dependencies
2. **Better UX** - Familiar, predictable modifier key behavior
3. **Simpler code** - Clean state machine with explicit transitions
4. **Future ready** - Extensible design for additional modifier combinations

This approach transforms parameter locks from a **timing-based challenge** into a **straightforward keyboard interface**
that works reliably across all platforms and usage scenarios.

**IMPLEMENTATION PRIORITY**: This should be implemented immediately as it solves the core architectural issues
identified in the current timing-based approach and provides a solid foundation for all future parameter lock
functionality.