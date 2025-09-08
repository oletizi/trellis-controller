# Separation of Concerns Architectural Refactoring - Implementation Workplan

## Executive Summary

This workplan details the completion of the architectural refactoring to achieve proper separation of concerns in the StepSequencer system. The core semantic interface (`processMessage()`) has been implemented, but dependent code still uses the legacy button-handling interface. This plan systematically addresses all remaining compilation errors and architectural violations while implementing a comprehensive Input Layer Abstraction for cross-platform compatibility.

## Current State Analysis

### Completed ✅
- **StepSequencer core logic cleaned**: Removed direct button handling from `StepSequencer.cpp`
- **Semantic ControlMessage interface**: Complete `processMessage()` implementation for all sequencer operations
- **Input dependencies removed**: StepSequencer no longer depends on `IInput` interface
- **Parameter lock system decoupled**: Uses semantic messages instead of direct button events

### Compilation Errors Identified 🚨
1. **NonRealtimeSequencer.cpp** (Line 435): `sequencer_->handleButton()` - method removed
2. **NonRealtimeSequencer.cpp** (Line 56): Dependencies struct references removed `input` field
3. **main.cpp (simulation)** (Line 271): `sequencer_->handleButton()` - method removed
4. **JsonState.cpp** (Line 294): `sequencer.getButtonTracker()` - method removed

### Architectural Issues Remaining 🔧
1. **Missing InputController layer**: No component translates ButtonEvents to semantic ControlMessages
2. **Missing Input Layer Abstraction**: No platform-agnostic input system for cross-platform compatibility
3. **Platform-specific code**: `#ifdef ARDUINO` directives violate platform abstraction
4. **Button tracking in serialization**: JsonState still assumes button tracking in StepSequencer
5. **Mixed responsibilities**: Input handling code mixed with business logic in main.cpp

## Implementation Strategy

### Phase 1A: Input Layer Abstraction (Priority: CRITICAL)
**Estimated Time**: 3-4 hours

#### Task 1A.1: Create Core Input Layer Interfaces
**Dependencies**: None
**Files to Create**:
- `/include/core/IInputLayer.h`
- `/include/core/InputEvent.h`
- `/include/core/InputSystemConfiguration.h`

**Core Abstraction Specification**:
```cpp
// Platform-agnostic input event
struct InputEvent {
    enum class Type {
        BUTTON_PRESS,
        BUTTON_RELEASE,
        ENCODER_TURN,
        MIDI_INPUT,
        SYSTEM_EVENT
    };
    
    Type type;
    uint8_t deviceId;           // Which button/encoder/device
    uint32_t timestamp;         // When the event occurred
    int32_t value;              // Button state, encoder delta, MIDI data
    uint32_t context;           // Additional context data
};

// Core abstraction - platform agnostic
class IInputLayer {
public:
    virtual ~IInputLayer() = default;
    virtual void poll() = 0;
    virtual bool getNextEvent(InputEvent& event) = 0;
    virtual bool hasEvents() const = 0;
    virtual void setConfiguration(const InputConfiguration& config) = 0;
};
```

#### Task 1A.2: Create Platform-Specific Input Layer Implementations
**Dependencies**: Task 1A.1
**Files to Create**:
- `/include/embedded/NeoTrellisInputLayer.h` & `/src/embedded/NeoTrellisInputLayer.cpp`
- `/include/simulation/CursesInputLayer.h` & `/src/simulation/CursesInputLayer.cpp`
- `/include/test/MockInputLayer.h` & `/src/test/MockInputLayer.cpp`

**Implementation Requirements**:
- **NeoTrellisInputLayer**: Hardware I2C communication, interrupt handling, debouncing
- **CursesInputLayer**: Keyboard mapping, ncurses integration, visual feedback
- **MockInputLayer**: Event preloading, recording mode, deterministic testing

#### Task 1A.3: Create Input Layer Factory
**Dependencies**: Task 1A.2
**Files to Create**:
- `/include/core/InputLayerFactory.h`
- `/src/core/InputLayerFactory.cpp`

**Factory Pattern Implementation**:
```cpp
class InputLayerFactory {
public:
    enum class Platform {
        EMBEDDED_NEOTRELLIS,
        SIMULATION_CURSES,
        TESTING_MOCK,
        AUTO_DETECT
    };
    
    static std::unique_ptr<IInputLayer> create(
        Platform platform,
        const InputSystemConfiguration& config,
        const InputLayerDependencies& deps);
};
```

### Phase 1B: Enhanced InputController (Priority: CRITICAL)
**Estimated Time**: 2-3 hours

#### Task 1B.1: Create InputController Interface and Implementation
**Dependencies**: Phase 1A
**Files to Create**:
- `/include/core/IInputController.h`
- `/include/core/InputController.h` 
- `/src/core/InputController.cpp`

**Enhanced Specification**:
```cpp
class IInputController {
public:
    virtual ~IInputController() = default;
    virtual void update() = 0;  // Process input layer and detect gestures
    virtual bool getNextMessage(ControlMessage::Message& message) = 0;
    virtual bool hasMessages() const = 0;
    virtual void setConfiguration(const InputSystemConfiguration& config) = 0;
};

class InputController : public IInputController {
private:
    IInputLayer* inputLayer_;
    std::queue<ControlMessage::Message> messageQueue_;
    GestureDetector gestureDetector_;
    ButtonTracker buttonTracker_;
    
public:
    struct Dependencies {
        IInputLayer* inputLayer = nullptr;
        IClock* clock = nullptr;
        uint32_t holdThresholdMs = 500;
        uint32_t doubleTapThresholdMs = 300;
    };
    
    explicit InputController(Dependencies deps);
    void update() override;
    bool getNextMessage(ControlMessage::Message& message) override;
    bool hasMessages() const override;
    void setConfiguration(const InputSystemConfiguration& config) override;
};
```

#### Task 1B.2: Create Advanced Gesture Detection System
**Dependencies**: Task 1B.1
**Files to Create**:
- `/include/core/GestureDetector.h`
- `/src/core/GestureDetector.cpp`

**Gesture Detection Capabilities**:
```cpp
class GestureDetector {
public:
    enum class GestureType {
        TAP,                // Quick press and release
        HOLD,               // Press and hold for threshold time
        HOLD_RELEASE,       // Release after hold
        DOUBLE_TAP,         // Two taps within time window
        CHORD,              // Multiple simultaneous button presses
        SEQUENCE            // Specific button sequence pattern
    };
    
    struct Gesture {
        GestureType type;
        uint8_t buttonId;
        uint8_t trackId;
        uint8_t stepId;
        uint32_t timestamp;
        std::vector<uint8_t> chordButtons;  // For chord gestures
    };
    
    void processInputEvent(const InputEvent& event);
    void update(uint32_t currentTime);
    bool getNextGesture(Gesture& gesture);
};
```

#### Task 1B.3: Create ButtonTracker Standalone Component
**Dependencies**: Task 1B.1
**Files to Create**:
- `/include/core/ButtonTracker.h`
- `/src/core/ButtonTracker.cpp`

**Rationale**: Extract button tracking from StepSequencer into reusable component for InputController.

### Phase 2: Fix Compilation Errors (Priority: HIGH)  
**Estimated Time**: 2-3 hours

#### Task 2.1: Fix NonRealtimeSequencer.cpp
**Dependencies**: Task 1.1
**Files to Modify**: `/src/core/NonRealtimeSequencer.cpp`

**Changes Required**:
```cpp
// Remove line 56: .input = nullInput_.get()
StepSequencer::Dependencies deps{
    .clock = clock_.get(),
    .midiOutput = nullMidi_.get(),
    .midiInput = nullptr,
    .display = nullDisplay_.get(),
    .debugOutput = nullptr  // Remove input field
};

// Replace line 435: sequencer_->handleButton(button, pressed, virtualTime_);
// With semantic message processing:
ControlMessage::Message msg;
if (pressed) {
    msg = ControlMessage::Message::keyPress(button, virtualTime_);
} else {
    msg = ControlMessage::Message::keyRelease(button, virtualTime_);
}
bool success = sequencer_->processMessage(msg);
```

**Testing**: Verify NonRealtimeSequencer compiles and basic message processing works.

#### Task 2.2: Fix main.cpp (simulation)  
**Dependencies**: Task 1.1, Task 2.1
**Files to Modify**: `/src/simulation/main.cpp`

**Changes Required**:
```cpp
// Add InputController to SimulationApp
std::unique_ptr<InputController> inputController_;

// In constructor, add:
InputController::Dependencies inputDeps{
    .clock = clock_.get(),
    .holdThresholdMs = 500
};
inputController_ = std::make_unique<InputController>(inputDeps);

// Replace line 271: sequencer_->handleButton(buttonIndex, event.pressed, currentTime);
// With InputController usage:
inputController_->processButtonEvent(event);
ControlMessage::Message message;
while (inputController_->getNextMessage(message)) {
    sequencer_->processMessage(message);
}
```

**Testing**: Verify simulation compiles, runs, and button events work correctly.

#### Task 2.3: Fix JsonState.cpp Serialization
**Dependencies**: Task 1.2
**Files to Modify**: `/src/core/JsonState.cpp`

**Changes Required**:
```cpp
// Remove line 294: const auto& buttonTracker = sequencer.getButtonTracker();
// Replace with InputController state capture (if needed)
// Or remove button state from serialization entirely for clean separation

// Option 1: Remove button serialization entirely (recommended)
// - Remove buttons array from JsonState::Snapshot
// - Remove button serialization/deserialization code
// - Update validation to not check button states

// Option 2: Move button tracking to InputController
// - Modify InputController to provide state access
// - Update JsonState to capture InputController state instead
```

**Recommendation**: Remove button state serialization for cleaner separation. Button states are transient input concerns, not core sequencer state.

### Phase 3: Architectural Cleanup (Priority: MEDIUM)
**Estimated Time**: 1-2 hours

#### Task 3.1: Remove Platform-Specific Code
**Dependencies**: Phase 2 Complete
**Files to Modify**: All files with `#ifdef ARDUINO`

**Approach**: Move all Arduino-specific code to platform implementation directories:
- `/src/embedded/ArduinoInputController.cpp` - Arduino-specific input handling
- `/src/simulation/SimulationInputController.cpp` - Simulation input handling
- Remove all `#ifdef ARDUINO` from core business logic

#### Task 3.2: Update Dependencies Injection
**Dependencies**: Task 3.1
**Files to Modify**: All main.cpp and initialization files

**Changes Required**:
```cpp
// Update Dependencies structures to include InputController
struct ApplicationDependencies {
    IClock* clock;
    IDisplay* display;
    IInputController* inputController;
    IMidiOutput* midiOutput;
    // Remove IInput* - replace with IInputController*
};
```

### Phase 4: Integration and Testing (Priority: HIGH)
**Estimated Time**: 2-4 hours

#### Task 4.1: Integration Testing
**Dependencies**: Phase 3 Complete
**Test Scenarios**:
1. **Button Tap Handling**: Verify taps generate `TOGGLE_STEP` messages
2. **Button Hold Handling**: Verify holds generate `ENTER_PARAM_LOCK`/`EXIT_PARAM_LOCK` messages  
3. **Parameter Adjustment**: Verify parameter locks work with new message flow
4. **State Persistence**: Verify serialization works without button tracking
5. **Cross-Platform**: Test both simulation and embedded builds

#### Task 4.2: Regression Testing
**Dependencies**: Task 4.1
**Test Coverage**:
- All existing unit tests pass
- Parameter lock functionality preserved
- Real-time and non-real-time modes work
- MIDI output functions correctly
- Display updates work properly

### Phase 5: Documentation and Cleanup (Priority: LOW)
**Estimated Time**: 1 hour

#### Task 5.1: Update Architecture Documentation
**Files to Update**:
- `/CLAUDE.md` - Update architectural guidelines
- Code comments - Update interface documentation
- `/README.md` - Update build instructions if needed

#### Task 5.2: Code Quality Review
**Checklist**:
- All files under 500 lines ✅
- No business logic in platform code ✅
- Clean dependency injection throughout ✅
- No fallback code outside tests ✅
- Proper error handling with descriptive messages ✅

## Detailed Implementation Guidelines

### InputController Design Patterns

#### Message Translation Strategy
```cpp
void InputController::processButtonEvent(const ButtonEvent& event) {
    // Update button tracker state
    buttonTracker_.updateButton(event.buttonIndex, event.pressed, event.timestamp);
    
    // Check for completed gestures
    if (event.pressed) {
        // Button press - no immediate action, wait for release or hold
        return;
    }
    
    // Button release - determine gesture type
    const auto& buttonState = buttonTracker_.getButtonState(event.buttonIndex);
    
    if (buttonState.wasHeld) {
        // Hold completed - generate EXIT_PARAM_LOCK
        ControlMessage::Message msg = ControlMessage::Message::exitParamLock(event.timestamp);
        messageQueue_.push(msg);
    } else {
        // Tap completed - generate TOGGLE_STEP
        uint8_t track, step;
        if (buttonIndexToTrackStep(event.buttonIndex, track, step)) {
            ControlMessage::Message msg = ControlMessage::Message::toggleStep(track, step, event.timestamp);
            messageQueue_.push(msg);
        }
    }
}
```

#### Hold Detection Logic
```cpp
void InputController::checkForHolds() {
    for (uint8_t i = 0; i < 32; ++i) {
        auto& buttonState = buttonTracker_.getButtonState(i);
        
        if (buttonState.pressed && !buttonState.holdProcessed) {
            uint32_t currentTime = clock_->getCurrentTime();
            if (currentTime - buttonState.pressTime >= holdThresholdMs_) {
                // Hold detected - generate ENTER_PARAM_LOCK
                uint8_t track, step;
                if (buttonIndexToTrackStep(i, track, step)) {
                    ControlMessage::Message msg = ControlMessage::Message::enterParamLock(track, step, currentTime);
                    messageQueue_.push(msg);
                    buttonState.holdProcessed = true;
                }
            }
        }
    }
}
```

### Error Handling Strategy

#### Compilation Error Prevention
- **Interface Contracts**: All interfaces clearly define expected behavior
- **Dependency Validation**: Constructor validation for all required dependencies
- **Message Validation**: Input validation in `processMessage()` implementations

#### Runtime Error Management
```cpp
bool InputController::processMessage(const ControlMessage::Message& message) {
    try {
        // Validate message parameters
        if (!validateMessage(message)) {
            throw std::invalid_argument("Invalid message parameters");
        }
        
        // Process message
        return sequencer_->processMessage(message);
        
    } catch (const std::exception& e) {
        if (debugOutput_) {
            debugOutput_->log("InputController error: " + std::string(e.what()));
        }
        return false;
    }
}
```

### Testing Strategy

#### Unit Test Requirements
1. **InputController Tests**:
   - Button tap → `TOGGLE_STEP` message generation  
   - Button hold → `ENTER_PARAM_LOCK`/`EXIT_PARAM_LOCK` message generation
   - Message queue management
   - Hand preference handling

2. **Integration Tests**:
   - End-to-end button event processing
   - Parameter lock workflow with new message flow
   - State persistence without button tracking

3. **Regression Tests**:
   - All existing functionality preserved
   - Performance characteristics maintained
   - Memory usage within embedded constraints

#### Test Implementation Pattern
```cpp
TEST_CASE("InputController generates correct messages for button taps") {
    MockClock clock;
    InputController::Dependencies deps{&clock, 500};
    InputController controller(deps);
    
    // Simulate button tap
    ButtonEvent pressEvent{0, 0, true, 100};
    ButtonEvent releaseEvent{0, 0, false, 150};
    
    controller.processButtonEvent(pressEvent);
    controller.processButtonEvent(releaseEvent);
    
    // Verify TOGGLE_STEP message generated
    ControlMessage::Message message;
    REQUIRE(controller.getNextMessage(message));
    REQUIRE(message.type == ControlMessage::Type::TOGGLE_STEP);
    REQUIRE(message.param1 == 0); // track
    REQUIRE(message.param2 == 0); // step
}
```

## Risk Assessment and Mitigation

### High Risk Items
1. **Breaking Existing Functionality**: Comprehensive regression testing required
   - **Mitigation**: Incremental changes with testing at each step
   
2. **Performance Impact**: Additional message queuing overhead
   - **Mitigation**: Lightweight message structures, bounded queues

3. **Memory Usage**: Additional InputController and message queue memory
   - **Mitigation**: Fixed-size queues, memory pool allocation

### Medium Risk Items  
1. **Integration Complexity**: Multiple components need coordinated changes
   - **Mitigation**: Phase-based implementation with clear dependencies

2. **Platform Differences**: Different behavior between simulation and embedded
   - **Mitigation**: Standardized interfaces, comprehensive cross-platform testing

## Success Criteria

### Functional Requirements ✅
- [ ] All compilation errors resolved
- [ ] Button taps generate `TOGGLE_STEP` messages  
- [ ] Button holds generate parameter lock messages
- [ ] Sequencer functionality preserved
- [ ] State persistence works correctly
- [ ] Both simulation and embedded builds work

### Architectural Requirements ✅
- [ ] Clean separation between input handling and business logic
- [ ] No platform-specific code in core business logic
- [ ] Semantic message interface used throughout
- [ ] Dependency injection maintained
- [ ] No fallback code outside tests
- [ ] Files remain under 500 lines

### Quality Requirements ✅
- [ ] All existing tests pass
- [ ] New unit tests for InputController
- [ ] Memory usage fits embedded constraints  
- [ ] Performance requirements met
- [ ] Code review checklist satisfied

## Implementation Timeline

| Phase | Duration | Dependencies | Deliverables |
|-------|----------|--------------|-------------|
| Phase 1 | 2-3 hrs | None | InputController interface and implementation |
| Phase 2 | 2-3 hrs | Phase 1 | All compilation errors fixed |
| Phase 3 | 1-2 hrs | Phase 2 | Platform-specific code removed |
| Phase 4 | 2-4 hrs | Phase 3 | Integration testing complete |
| Phase 5 | 1 hr | Phase 4 | Documentation updated |

**Total Estimated Time**: 8-13 hours

## Post-Implementation Verification

### Architectural Validation
1. **Component Boundaries**: Verify clean interfaces between components
2. **Dependency Flow**: Confirm dependency injection works correctly  
3. **Platform Abstraction**: Ensure no business logic in platform code
4. **Message Flow**: Validate semantic message processing end-to-end

### Functional Validation
1. **Feature Parity**: All original functionality preserved
2. **Performance**: Real-time constraints maintained
3. **Memory**: Embedded memory constraints satisfied
4. **Cross-Platform**: Both simulation and embedded builds work identically

This comprehensive workplan provides a clear path to completing the architectural refactoring while maintaining system functionality and meeting all quality requirements. Each phase has clearly defined deliverables and success criteria, enabling systematic progress tracking and risk mitigation.