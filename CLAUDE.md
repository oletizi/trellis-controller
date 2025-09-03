# Claude AI Agent Guidelines for Trellis Controller

This document provides guidelines for AI agents (including Claude Code) working on the Trellis Controller project. It ensures consistency with the project's architectural principles, quality standards, and development patterns.

## Overview

Trellis Controller is an embedded C++ application framework for the AdaFruit NeoTrellis M4 platform focused on:

- C++ firmware for SAMD51 microcontrollers
- Step sequencer and musical instrument applications
- Real-time button/LED interaction via I2C/Seesaw protocol
- CMake-based cross-compilation for ARM Cortex-M4
- Audio synthesis and MIDI output capabilities
- Pattern-based music sequencing

## Core Requirements

### Error Handling

- **Never implement fallbacks or use mock data outside of test code**
- **Throw errors with descriptive messages** instead of fallbacks
- Errors let us know that something isn't implemented
- Fallbacks and mock data are bug factories

```cpp
// ✅ GOOD: Throw descriptive errors
if (!trellis.begin()) {
    throw std::runtime_error("Failed to initialize NeoTrellis hardware");
}

// ❌ BAD: Using fallbacks
bool trellisReady = trellis.begin() || simulateHardware();
```

### Code Quality Standards

- **C++17 standard required** (configured in CMakeLists.txt)
- **RAII for resource management** - no manual memory management
- **Const correctness** - use const wherever possible
- **No dynamic allocation in real-time paths**
- All code must be testable via dependency injection

### File Size Limits

- **Code files should be no larger than 300-500 lines**
- Anything larger should be refactored for readability and modularity
- Split large files into smaller, focused modules

### Repository Hygiene

- **Build artifacts ONLY in `build/` directory** (configured in .gitignore)
- NO temporary scripts, logs, or generated files committed to git
- **Never bypass pre-commit or pre-push hooks** - fix issues instead
- Clean repository is mandatory

## Implementation Patterns

### Dependency Injection Pattern

```cpp
// Good: Constructor injection with interfaces
class StepSequencer {
public:
    struct Dependencies {
        IClock* clock = nullptr;
        IAudioOutput* audio = nullptr;
        IMidiOutput* midi = nullptr;
    };
    
    explicit StepSequencer(Dependencies deps = {})
        : clock_(deps.clock ? deps.clock : &defaultClock_)
        , audio_(deps.audio ? deps.audio : &defaultAudio_)
        , midi_(deps.midi ? deps.midi : &defaultMidi_) {
    }
    
private:
    IClock* clock_;
    IAudioOutput* audio_;
    IMidiOutput* midi_;
    
    // Default implementations
    static SystemClock defaultClock_;
    static NullAudioOutput defaultAudio_;
    static NullMidiOutput defaultMidi_;
};
```

### Hardware Abstraction

```cpp
// Abstract hardware interface for testing
class IHardware {
public:
    virtual ~IHardware() = default;
    virtual bool readButton(uint8_t index) = 0;
    virtual void setLED(uint8_t index, uint32_t color) = 0;
    virtual void updateDisplay() = 0;
};

// Concrete implementation
class NeoTrellisHardware : public IHardware {
    // Actual hardware interaction
};

// Test implementation
class MockHardware : public IHardware {
    // Mock for unit tests
};
```

## Project Structure

```text
trellis-controller/
├── CMakeLists.txt        # Build configuration
├── src/                  # Source files
│   ├── main.cpp         # Application entry point
│   ├── apps/            # Different applications (sequencer, synth, etc.)
│   ├── drivers/         # Hardware drivers (NeoTrellis, I2C, etc.)
│   └── core/            # Core utilities and abstractions
├── include/             # Header files
│   ├── apps/            # Application interfaces
│   ├── drivers/         # Driver interfaces
│   └── core/            # Core interfaces
├── test/                # Unit tests
├── lib/                 # Third-party libraries
└── cmake/               # CMake modules and toolchain files
```

## Embedded-Specific Guidelines

### Memory Management
- **Fixed memory allocation**: No dynamic allocation after initialization
- **Stack usage awareness**: Monitor stack depth for recursive functions
- **Static buffers**: Use compile-time sized arrays
- **Memory pools**: Pre-allocate pools for dynamic-like behavior

### Real-time Constraints
- **Deterministic execution**: Avoid unbounded loops
- **ISR safety**: Keep interrupt handlers minimal
- **Lock-free algorithms**: Use atomic operations where possible
- **Fixed timing**: Respect sequencer timing requirements

### Hardware Integration
- **I2C timing**: Respect bus timing and avoid blocking
- **LED refresh rates**: Balance update frequency with CPU usage
- **Button debouncing**: Implement proper debounce algorithms
- **Power management**: Consider sleep modes when idle

## Development Workflow for AI Agents

### Before Making Changes

1. **Read existing code** to understand patterns and conventions
2. **Check CMakeLists.txt** to understand build configuration
3. **Review hardware datasheets** for SAMD51 and NeoTrellis
4. **Understand memory constraints** of the target platform

### When Writing Code

1. **Use dependency injection** - pass dependencies via constructor
2. **Follow RAII principles** for resource management
3. **Write testable code** with clear interfaces
4. **Avoid dynamic allocation** in performance-critical paths
5. **Use descriptive error messages** with context
6. **Throw errors instead of fallbacks** outside of test code
7. **Keep files under 300-500 lines** - refactor if larger

### Before Completing Tasks

1. **Build the project**: `./build.sh`
2. **Check compilation**: `cmake --build build`
3. **Verify no warnings**: Build should be warning-free
4. **Test on hardware** if possible

## Common Commands

```bash
# Build commands
./build.sh              # Quick build script
mkdir build && cd build
cmake ..                # Configure build
make -j4                # Build with 4 cores

# Flash to device
make flash              # Flash via BOSSAC

# Clean build
rm -rf build/
mkdir build && cd build
cmake .. && make

# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

## Error Handling Pattern

```cpp
try {
    if (!hardware.initialize()) {
        throw std::runtime_error("Hardware initialization failed: check I2C connections");
    }
    return true;
} catch (const std::exception& e) {
    Serial.printf("Error: %s\n", e.what());
    // Blink error pattern on LEDs
    hardware.showErrorPattern();
    throw;
}
```

## Critical Don'ts for AI Agents

❌ **NEVER implement fallbacks or mock data** outside of test code - throw descriptive errors instead  
❌ **NEVER use dynamic allocation** in interrupt handlers or real-time paths  
❌ **NEVER ignore hardware timing constraints** - respect I2C/SPI timing  
❌ **NEVER bypass the build system** - use CMake for all builds  
❌ **NEVER commit build artifacts** - only source code  
❌ **NEVER create files larger than 500 lines** - refactor for modularity  
❌ **NEVER use blocking delays** in main loop - use state machines  
❌ **NEVER ignore memory constraints** - SAMD51 has limited RAM  
❌ **NEVER skip error checking** for hardware operations  
❌ **NEVER use recursion** without bounded depth  

## Success Criteria

An AI agent has successfully completed work when:

- ✅ Code compiles without warnings
- ✅ Build system (CMake) works correctly
- ✅ Code follows RAII and const-correctness
- ✅ No dynamic allocation in real-time paths
- ✅ Dependency injection used for testability
- ✅ Hardware abstractions properly implemented
- ✅ Error messages are descriptive
- ✅ Files are appropriately sized (under 500 lines)
- ✅ Memory usage fits within SAMD51 constraints
- ✅ Timing requirements are met
- ✅ No blocking operations in main loop

## Hardware Specifications

### NeoTrellis M4 (SAMD51J19A)
- **CPU**: ARM Cortex-M4F @ 120MHz
- **RAM**: 192KB
- **Flash**: 512KB
- **FPU**: Hardware floating point
- **Buttons**: 32 (4x8 grid)
- **LEDs**: 32 RGB NeoPixels
- **Communication**: I2C Seesaw protocol
- **Audio**: I2S output capable

## When in Doubt

- Look at existing code in the project for patterns
- Check ARM Cortex-M4 documentation for processor specifics
- Follow C++ Core Guidelines for modern C++ practices
- Use dependency injection for testability
- Prioritize deterministic behavior over convenience
- Throw errors with context instead of using fallbacks
- Always use CMake targets instead of manual compilation
- Document hardware-specific requirements in code comments