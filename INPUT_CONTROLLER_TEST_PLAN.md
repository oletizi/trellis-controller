# Input Controller Integration Test Plan

## Overview

This document outlines the comprehensive deterministic testing strategy for verifying that the Input Layer Abstraction refactoring works correctly. The testing strategy addresses the complete pipeline:

```
CursesInputLayer → InputEvents → GestureDetector → ControlMessages → StepSequencer
```

## Problem Statement

The trellis controller recently underwent a massive architectural refactoring implementing Input Layer Abstraction. The current simulation at `src/simulation/main.cpp` uses a temporary workaround that bypasses the new InputController architecture. **Parameter locks are NOT working correctly in the current implementation.**

## Test Architecture

### 1. Integration Test Framework

**File**: `test/integration/InputControllerIntegrationTest.cpp`

This comprehensive test suite uses Catch2 framework and mock objects to test the complete input pipeline deterministically:

- **MockInputLayer**: Provides controlled input event injection
- **MockClock**: Enables deterministic time control for timing-dependent tests
- **MockDebugOutput**: Captures debug output for verification
- **InputControllerIntegrationTestFixture**: Test harness that orchestrates the complete pipeline

### 2. Standalone Integration Test

**File**: `test_input_controller_integration.cpp`

A standalone C++ program that can be compiled and run independently to verify core functionality:

```bash
g++ -std=c++17 -I./include/core -I./include/simulation \
    test_input_controller_integration.cpp \
    src/core/InputController.cpp \
    src/core/GestureDetector.cpp \
    -o test_input_integration
./test_input_integration
```

### 3. Updated Simulation Implementation

**File**: `src/simulation/main_with_input_controller.cpp`

A complete replacement for the current main.cpp that demonstrates proper InputController integration instead of the temporary workaround.

## Test Scenarios

### 1. Step Toggle Testing (Tap Detection)

**Objective**: Verify tap-based step toggling works correctly

**Test Cases**:
- Single tap generates `ControlMessage::Type::TOGGLE_STEP`
- Multiple taps generate multiple toggle messages with correct track/step parameters
- Tap timing under hold threshold (< 500ms) is recognized as tap

**Expected Behavior**:
- Press duration < 500ms → TOGGLE_STEP message
- Message contains correct track and step indices
- No parameter lock entry occurs

### 2. Parameter Lock Entry Testing (Hold Detection)

**Objective**: Verify hold-based parameter lock entry works correctly

**Test Cases**:
- Hold generates `ControlMessage::Type::ENTER_PARAM_LOCK`
- Hold timing over threshold (≥ 500ms) triggers parameter lock
- InputController reports `isInParameterLockMode() == true`
- Correct track/step parameters in message

**Expected Behavior**:
- Press duration ≥ 500ms → ENTER_PARAM_LOCK message
- System enters parameter lock mode
- Message contains locked track and step indices

### 3. Parameter Lock Exit Testing

**Objective**: Verify parameter lock exits correctly

**Test Cases**:
- Releasing locked button exits parameter lock mode
- Generates `ControlMessage::Type::EXIT_PARAM_LOCK`
- InputController reports `isInParameterLockMode() == false`
- System returns to normal operation

**Expected Behavior**:
- Tap on locked button → EXIT_PARAM_LOCK message
- System exits parameter lock mode
- Subsequent taps generate TOGGLE_STEP messages

### 4. Parameter Adjustment Testing

**Objective**: Verify parameter adjustments work in lock mode

**Test Cases**:
- Button presses in parameter lock mode generate `ControlMessage::Type::ADJUST_PARAMETER`
- Different buttons map to different parameter types
- Parameter delta values are calculated correctly
- Activity resets timeout timer

**Expected Behavior**:
- Any button press in lock mode → ADJUST_PARAMETER message
- Message contains parameter type and delta value
- Timeout timer resets on each adjustment

### 5. Parameter Lock Timeout Testing

**Objective**: Verify automatic parameter lock timeout

**Test Cases**:
- Parameter lock times out after 5 seconds of inactivity
- Generates `ControlMessage::Type::EXIT_PARAM_LOCK`
- System automatically returns to normal mode
- No manual intervention required

**Expected Behavior**:
- 5 seconds without activity → EXIT_PARAM_LOCK message
- System automatically exits parameter lock mode
- Timeout is configurable

### 6. Complex Interaction Sequences

**Objective**: Verify mixed interaction patterns work correctly

**Test Cases**:
- Sequence: Taps → Hold → Adjustments → Exit → Taps
- Overlapping gestures handled correctly
- State transitions work as expected
- No messages are lost or duplicated

**Expected Behavior**:
- Complete sequence generates correct message types in order
- All state transitions occur properly
- System remains consistent throughout

## Deterministic Testing Features

### Time Control

All tests use `MockClock` to provide deterministic time progression:

```cpp
// Simulate exact timing for hold detection
fixture.getClock()->setCurrentTime(baseTime + 600); // 600ms hold
fixture.getInputController()->poll(); // Trigger timing update
```

### Event Injection

Tests inject precise input events to simulate user interactions:

```cpp
// Simulate tap (quick press/release)
fixture.simulateTap(buttonId, timestamp);

// Simulate hold (long press/release)  
fixture.simulateHold(buttonId, timestamp, 600); // 600ms duration
```

### Message Verification

Tests verify exact message content and sequencing:

```cpp
auto message = fixture.getLastMessage();
REQUIRE(message.type == ControlMessage::Type::TOGGLE_STEP);
REQUIRE(message.param1 == expectedTrack);
REQUIRE(message.param2 == expectedStep);
```

## Automated Test Execution

### Build Integration

Add to CMakeLists.txt for automated testing:

```cmake
# Integration tests
add_executable(input_controller_integration_test
    test/integration/InputControllerIntegrationTest.cpp
    src/core/InputController.cpp
    src/core/GestureDetector.cpp
    # ... other dependencies
)
target_link_libraries(input_controller_integration_test Catch2::Catch2WithMain)

# Standalone test
add_executable(input_controller_standalone_test
    test_input_controller_integration.cpp
    src/core/InputController.cpp
    src/core/GestureDetector.cpp
)
```

### Continuous Integration

```bash
# Run all input controller tests
make test

# Run standalone integration test
./test_input_integration

# Run simulation with new architecture
make simulation-with-input-controller
```

## Manual Testing Protocol

### 1. Simulation Testing

Run the updated simulation to manually verify functionality:

```bash
cd src/simulation
g++ -std=c++17 main_with_input_controller.cpp -o sim_with_input_controller
./sim_with_input_controller
```

**Manual Test Steps**:
1. Press lowercase keys quickly (tap) → Should toggle steps
2. Hold uppercase keys for >500ms → Should enter parameter lock
3. In parameter lock mode, press other keys → Should adjust parameters
4. Release locked key → Should exit parameter lock
5. Wait 5 seconds without activity → Should timeout and exit

### 2. Interactive Test Scenarios

**Scenario A: Basic Step Toggling**
```
Input: '1' (quick tap on track 0, step 0)
Expected: Step LED turns on/off, TOGGLE_STEP message in debug
```

**Scenario B: Parameter Lock Entry**
```
Input: Hold 'Q' for 600ms (track 1, step 0)
Expected: "Entering parameter lock" debug message, LED changes to white
```

**Scenario C: Parameter Adjustment**
```
Input: While in parameter lock, press '2' (adjust parameter)
Expected: "Adjusting parameter" debug message, parameter value changes
```

**Scenario D: Parameter Lock Exit**
```
Input: Tap 'q' (release locked button)
Expected: "Exiting parameter lock" debug message, return to normal mode
```

## Success Criteria

### Functional Requirements

✅ **Tap Detection**: Press duration < 500ms generates TOGGLE_STEP messages
✅ **Hold Detection**: Press duration ≥ 500ms generates ENTER_PARAM_LOCK messages  
✅ **Parameter Lock Mode**: System correctly tracks parameter lock state
✅ **Parameter Adjustment**: Button presses in lock mode adjust parameters
✅ **Lock Exit**: Releasing locked button exits parameter lock mode
✅ **Timeout Handling**: Automatic exit after 5 seconds of inactivity
✅ **Message Sequencing**: All messages generated in correct chronological order
✅ **State Consistency**: System state remains consistent across all operations

### Performance Requirements

✅ **Response Time**: Input to message generation < 20ms
✅ **Memory Usage**: No memory leaks or unbounded growth
✅ **Timing Accuracy**: Hold detection within ±10ms of threshold
✅ **Queue Management**: No message queue overflows under normal operation

### Integration Requirements

✅ **Architecture Compliance**: Uses proper InputController instead of workaround
✅ **Platform Abstraction**: Same logic works across embedded and simulation
✅ **Dependency Injection**: All components properly inject dependencies
✅ **Error Handling**: Graceful handling of invalid inputs and edge cases

## Test Execution Results

### Expected Output from Automated Tests

```
Input Controller Integration Tests...

Testing Step Toggle (Tap Detection)... PASSED
Testing Parameter Lock Entry (Hold Detection)... PASSED
Testing Parameter Lock Exit... PASSED
Testing Parameter Adjustment in Lock Mode... PASSED
Testing Parameter Lock Timeout... PASSED
Testing Complex Interaction Sequence... PASSED

Test Results: 6/6 passed

✓ ALL TESTS PASSED - Input Controller Integration Working Correctly!
```

### Expected Simulation Behavior

When running the updated simulation:
- Lowercase keys (1,2,3,q,w,e,a,s,d,z,x,c) should toggle steps immediately
- Uppercase keys (Q,W,E,A,S,D,Z,X,C) held for >500ms should enter parameter lock
- In parameter lock mode, other keys should adjust parameters
- Parameter lock should exit when locked key is released
- Debug output should show all message types and state transitions

## Maintenance and Evolution

### Test Coverage Goals

- **Code Coverage**: >90% of InputController and GestureDetector code
- **Branch Coverage**: >85% of all conditional branches
- **Edge Case Coverage**: All timeout, overflow, and error conditions

### Future Test Enhancements

1. **Performance Testing**: Add latency and throughput measurements
2. **Stress Testing**: High-frequency input event simulation
3. **Regression Testing**: Automated testing of known bug scenarios
4. **Cross-Platform Testing**: Verify consistent behavior across platforms

### Documentation Maintenance

- Keep test scenarios synchronized with feature changes
- Update expected behaviors when requirements change  
- Maintain example code snippets with API changes
- Document any test-specific configuration requirements

---

## Files Created/Modified

1. `test/integration/InputControllerIntegrationTest.cpp` - Comprehensive integration test suite
2. `test/mocks/MockInputLayer.h` - Mock input layer for controlled testing
3. `test/mocks/MockClock.h` - Mock clock for deterministic time control
4. `src/simulation/main_with_input_controller.cpp` - Updated simulation with proper InputController integration
5. `test_input_controller_integration.cpp` - Standalone integration test
6. `INPUT_CONTROLLER_TEST_PLAN.md` - This comprehensive test plan

## Conclusion

This comprehensive testing strategy provides:

1. **Deterministic Testing**: Predictable, repeatable test execution
2. **Complete Coverage**: Tests all aspects of the input pipeline  
3. **Automated Verification**: Can be integrated into CI/CD pipeline
4. **Manual Validation**: Interactive testing capabilities for human verification
5. **Architecture Validation**: Proves that the Input Layer Abstraction refactoring works correctly

The testing strategy addresses the critical issue that **parameter locks are NOT working correctly** by providing comprehensive test coverage that will reveal and help fix the underlying problems in the current implementation.