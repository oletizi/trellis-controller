# Parameter Lock Test Suite

This directory contains comprehensive automated tests for the parameter lock functionality in the Trellis Controller project.

## Overview

The test suite eliminates the need for manual testing by providing automated validation of the complete parameter lock workflow:

1. **Hold detection** (500ms threshold via AdaptiveButtonTracker)
2. **State transitions** (normal → parameter lock → normal via SequencerStateManager)
3. **Parameter storage** (ParameterLockPool allocation/deallocation)
4. **Control grid mapping** (ControlGrid button assignment)
5. **Visual feedback** (LED color mapping)
6. **Simulation support** (CursesInput hold logic)

## Test Structure

### Unit Tests (Individual Components)
- `test_AdaptiveButtonTracker.cpp` - Hold detection timing and learning
- `test_SequencerStateManager.cpp` - Mode transitions and timeout handling
- `test_ParameterLockPool.cpp` - Memory pool allocation and parameter storage
- `test_ControlGrid.cpp` - Button mapping and ergonomic validation

### Integration Tests (Component Interaction)
- `test_ParameterLockIntegration.cpp` - Cross-component workflows
- `test_CursesInputSimulation.cpp` - Simulation-specific hold logic

### End-to-End Tests (Complete Workflows)
- `test_ParameterLockEndToEnd.cpp` - Full automated parameter lock sessions

## Running Tests

### Quick Start
```bash
# From project root directory
make test
```

### Detailed Test Execution
```bash
# From test directory
cd test
make help          # Show all available test commands
make build         # Build test suite
make run           # Run all tests
make detailed      # Run with verbose output
```

### Specific Test Categories
```bash
make unit          # Unit tests only
make integration   # Integration tests only
make end-to-end    # End-to-end workflow tests
make timing        # Timing-sensitive tests
make performance   # Performance benchmarks
```

### Test Coverage
```bash
make coverage      # Generate code coverage report
# View: build-test/coverage_html/index.html
```

## Key Features Tested

### Hold Detection Accuracy
- ✅ 500ms threshold precision (±1ms accuracy)
- ✅ Multiple button handling (first-held priority)
- ✅ Hold processing flags (prevent duplicate events)
- ✅ Learning system adaptation
- ✅ Edge cases (rapid press/release, time wraparound)

### State Management
- ✅ Mode transitions (normal ↔ parameter lock)
- ✅ Parameter lock context validation
- ✅ Control grid calculation (left/right side mapping)
- ✅ Timeout handling (configurable auto-exit)
- ✅ Force exit and error recovery

### Parameter Storage
- ✅ Memory pool allocation/deallocation
- ✅ Parameter type bitmask operations
- ✅ Data persistence and validation
- ✅ Pool exhaustion handling
- ✅ Integrity checks and statistics

### Control Grid Mapping
- ✅ Ergonomic button placement
- ✅ Hand preference adaptation
- ✅ Parameter type mapping (note, velocity, length, etc.)
- ✅ Visual feedback colors
- ✅ Usage pattern learning

### Simulation Testing
- ✅ CursesInput key mapping (uppercase=press, lowercase=release)
- ✅ Hold simulation patterns for all 32 buttons
- ✅ Complete workflow documentation
- ✅ Automated test sequences

## Test Results Verification

### Success Criteria
All tests must pass with:
- **Hold timing accuracy**: ±1ms of 500ms threshold
- **State consistency**: All components validate successfully
- **Memory integrity**: No leaks or corruption
- **Performance targets**: <50ms average operation time
- **Coverage goals**: >80% code coverage

### Automated Validation
The test suite automatically verifies:
- Component interactions work correctly
- Timing requirements are met
- Memory is properly managed
- Error conditions are handled gracefully
- Performance targets are achieved

## Test Data and Patterns

### Button Mapping (CursesInput Simulation)
```
Row 0: 1 2 3 4 | 5 6 7 8     (numbers/symbols)
Row 1: Q W E R | T Y U I     (QWERTY row) 
Row 2: A S D F | G H J K     (ASDF row)
Row 3: Z X C V | B N M <     (ZXCV row)

Hold Pattern: UPPERCASE = press, lowercase = release
Example: 'R' (press button 11) → wait 500ms → 'r' (release button 11)
```

### Control Grid Logic
- **Left-side held steps** (0-3): Use right-side columns (4-7) for control
- **Right-side held steps** (4-7): Use left-side columns (0-3) for control
- **Parameter mapping**: Note up/down, velocity up/down, length, probability
- **Visual feedback**: Color-coded buttons for each parameter type

## Integration with Manual Testing

While this automated test suite eliminates most manual testing needs, it also **documents exactly how to manually test** the parameter lock system:

1. **Manual Testing Guide**: See test comments for step-by-step procedures
2. **Key Combinations**: All press/release patterns documented
3. **Expected Behaviors**: Automated tests define correct responses
4. **Edge Cases**: Covers scenarios difficult to test manually

## Debugging Failed Tests

### Common Issues
1. **Timing failures**: Check system load, adjust tolerances if needed
2. **State inconsistency**: Verify component initialization order
3. **Memory leaks**: Run with coverage tools for detailed analysis
4. **Interface mismatches**: Ensure all components use consistent interfaces

### Debug Mode
```bash
make debug         # Build with debug symbols and run with verbose output
```

### Test-Specific Debugging
```bash
make hold          # Test only hold detection
make state         # Test only state management  
make pool          # Test only parameter lock pool
make grid          # Test only control grid
```

## Performance Benchmarking

The test suite includes performance benchmarks that measure:
- **Hold detection latency**: Should be <1ms
- **State transition time**: Should be <10ms
- **Pool operations**: Should be <1ms per allocation/deallocation
- **Control grid mapping**: Should be <5ms per calculation
- **Complete workflow**: Should be <50ms end-to-end

## Continuous Integration

These tests are designed to run in CI/CD pipelines:
- **No interactive input required**
- **Deterministic timing** (uses mock clocks)
- **Clear pass/fail results**
- **JUnit XML output** for integration with CI systems
- **Coverage reports** for quality gates

## Future Enhancements

The test framework is extensible for future features:
- **Visual regression testing** for LED patterns
- **Audio testing** for MIDI output validation  
- **Hardware-in-the-loop** testing with real devices
- **Stress testing** for long-term reliability
- **User experience testing** for ergonomic validation

---

*This test suite represents a comprehensive approach to embedded systems testing, providing both development confidence and documentation of expected behavior.*