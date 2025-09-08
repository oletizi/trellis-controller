# Parameter Lock System Test Harness

This document describes the comprehensive test harness for verifying the parameter lock system bug fixes.

## Overview

The parameter lock test harness (`test/test_parameter_locks.cpp`) provides automated testing for critical parameter lock functionality that was recently fixed:

### Bug Fixes Tested

1. **Parameter Lock Exit Bug (BUG FIX #1 & #3)**: System should NOT exit parameter lock mode when pressing control buttons
2. **Step Toggle Bug (BUG FIX #2)**: Steps should NOT toggle when released after being used for parameter lock
3. **Control Grid Functionality (BUG FIX #4)**: Proper routing to `adjustParameter()` method
4. **State Persistence**: Save/load state during parameter lock operations

## Test Architecture

The test harness uses the `NonRealtimeSequencer` infrastructure to provide:

- **Deterministic Testing**: Removes real-time dependencies
- **State Capture**: Before/after state comparison
- **Message Sequencing**: Precise timing control for button press/release/hold sequences
- **Comprehensive Logging**: Detailed operation logs for debugging
- **File Persistence**: State files saved for manual inspection

## Building and Running Tests

### Build Test Executable

```bash
# Navigate to project root
cd trellis-controller

# Configure for tests
cmake -B build-test -DBUILD_TESTS=ON .

# Build tests
cmake --build build-test

# Run parameter lock tests specifically
./build-test/test_parameter_locks

# Run in standalone mode (detailed logging)
./build-test/test_parameter_locks --standalone

# Run via CMake test target
cd build-test && make test-parameter-locks-standalone
```

### Test Execution Options

1. **Catch2 Integration**: `./test_parameter_locks` - Standard test runner
2. **Standalone Mode**: `./test_parameter_locks --standalone` - Detailed logging
3. **CMake Target**: `make test-parameter-locks-standalone` - Build system integration

## Test Cases

### Test 1: Parameter Lock Mode Persistence

**Objective**: Verify parameter lock mode persists when control buttons are pressed

**Sequence**:
1. Activate step (quick press/release)
2. Hold step button for 650ms (trigger parameter lock)
3. Press/release various control buttons while holding step
4. Verify system stays in parameter lock mode
5. Release step button (should exit parameter lock)

**Expected**: No premature exit from parameter lock mode

### Test 2: No Step Toggle After Parameter Lock

**Objective**: Verify steps don't toggle when released after parameter lock usage

**Sequence**:
1. Activate step
2. Hold step for parameter lock (650ms)
3. Adjust parameter via control grid
4. Release step button
5. Verify step remains active (no toggle)

**Expected**: Step stays active after parameter lock release

### Test 3: Control Grid Functionality

**Objective**: Test control grid boundaries and parameter adjustment routing

**Sequence**:
1. Enter parameter lock mode
2. Press various control grid buttons (NOTE, VELOCITY, LENGTH parameters)
3. Verify proper parameter adjustment routing
4. Exit parameter lock mode

**Expected**: Successful parameter adjustments for valid control buttons

### Test 4: State Persistence

**Objective**: Test save/load state during parameter lock operations

**Sequence**:
1. Set up pattern with active steps
2. Save initial state
3. Enter parameter lock and adjust parameters
4. Save final state
5. Load initial state and verify restoration

**Expected**: State properly saved/loaded during parameter lock operations

### Test 5: Edge Cases and Error Conditions

**Objective**: Test boundary conditions and error scenarios

**Sequence**:
1. Invalid button indices
2. Rapid press/release cycles
3. Overlapping button presses
4. Multiple simultaneous holds

**Expected**: Graceful handling of edge cases without crashes

## Test Output and Analysis

### State Files

Tests generate state files in `test_states_parameter_locks/`:
- `test1_initial.json` - Initial state before test
- `test1_final.json` - Final state after test
- `param_lock_state.json` - Saved states for persistence testing

### Logging Output

In standalone mode, detailed logs show:
- Button press/release events
- Parameter lock mode transitions
- Control grid button mappings
- Parameter adjustment results
- State changes and validations

### Success Criteria

- ✅ All 5 test cases pass
- ✅ No crashes or exceptions
- ✅ State consistency maintained
- ✅ Parameter lock mode behaves correctly
- ✅ No unwanted step toggles

## Integration with Build System

The test harness is integrated into CMakeLists.txt with:

```cmake
# Dedicated Parameter Lock test executable
add_executable(test_parameter_locks ...)
target_link_libraries(test_parameter_locks PRIVATE Catch2::Catch2)

# Standalone mode target
add_custom_target(test-parameter-locks-standalone ...)
```

## Debugging Failed Tests

1. **Check State Files**: Examine JSON state files for unexpected changes
2. **Review Logs**: Use standalone mode for detailed operation logs
3. **Timing Issues**: Verify hold durations and sequence timing
4. **Button Mapping**: Confirm control grid button indices
5. **Parameter Values**: Check parameter adjustment values and ranges

## Future Enhancements

- **Performance Metrics**: Add timing measurements
- **Visual Output**: Generate test result diagrams
- **Regression Testing**: Automated CI/CD integration
- **Extended Scenarios**: More complex parameter lock sequences
- **Hardware Validation**: Real hardware testing integration

## Dependencies

- **NonRealtimeSequencer**: Deterministic test execution
- **ControlMessage**: Message factory methods
- **JsonState**: State serialization and comparison
- **Catch2**: Test framework
- **C++17**: Modern C++ features
- **Filesystem**: State file management

The test harness provides comprehensive validation of the parameter lock system fixes and serves as a regression testing foundation for future development.