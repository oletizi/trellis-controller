# Trellis Controller - Advanced Multi-Platform C++ Sequencer

## Project Overview
This is a sophisticated embedded C++ project implementing a step sequencer for the NeoTrellis M4 with a **multi-platform architecture**. The project features complete hardware abstraction, dependency injection patterns, and dual build targets (embedded + simulation) using modern C++17 practices.

**Key Features:**
- C++ firmware for SAMD51 microcontrollers
- Step sequencer and musical instrument applications  
- Real-time button/LED interaction via I2C/Seesaw protocol
- Dual build system: Arduino CLI + CMake
- Complete platform abstraction with simulation capability
- Audio synthesis and MIDI output capabilities
- Pattern-based music sequencing

## Architecture Principles

### Platform Abstraction Architecture
**Critical**: This project uses rigorous platform abstraction with **zero business logic tied to specific platforms**:

```cpp
// Core interfaces (platform-agnostic)
class IDisplay;     // LED/display abstraction
class IInput;       // Button/input abstraction  
class IClock;       // Timing abstraction

// Platform implementations
CursesDisplay;      // Terminal-based simulation
NeoTrellisDisplay;  // Hardware RGB LEDs
```

### Error Handling Philosophy
**CRITICAL**: Never implement fallbacks or mock data outside test code:

```cpp
// ✅ CORRECT: Throw descriptive errors
if (!trellis.begin()) {
    throw std::runtime_error("Failed to initialize NeoTrellis hardware: check I2C connections");
}

// ❌ WRONG: Using fallbacks
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

## CMake Best Practices

### Unified CMakeLists.txt with Multiple Targets

**IMPORTANT**: Always use a single CMakeLists.txt file with conditional compilation for different build targets rather than maintaining separate CMake files.

```cmake
# Good: Single CMakeLists.txt with options
option(BUILD_SIMULATION "Build simulation target" OFF)
option(BUILD_TESTS "Build test target" OFF)

if(BUILD_SIMULATION)
    add_executable(simulation_target ...)
elseif(BUILD_TESTS)
    add_executable(test_target ...)
else()
    add_executable(embedded_target ...)
endif()
```

### Creating Distinct Targets

**NEVER temporarily comment out code** to switch between configurations. Instead, create distinct CMake targets that can coexist:

```cmake
# Good: Multiple targets in same build
add_executable(trellis_embedded ${EMBEDDED_SOURCES})
add_executable(trellis_simulation ${SIMULATION_SOURCES})
add_executable(trellis_tests ${TEST_SOURCES})

# Bad: Commenting out targets
# add_executable(trellis_embedded ...)  # Commented for simulation
add_executable(trellis_simulation ...)
```

Benefits of this approach:
- All targets can be built from the same configuration
- No need to reconfigure when switching between targets
- CI/CD can build and test all targets in one pass
- Code completion tools understand all configurations
- Reduces configuration drift between targets

## When in Doubt

- Look at existing code in the project for patterns
- Check ARM Cortex-M4 documentation for processor specifics
- Follow C++ Core Guidelines for modern C++ practices
- Use dependency injection for testability
- Prioritize deterministic behavior over convenience
- Throw errors with context instead of using fallbacks
- Always use CMake targets instead of manual compilation
- Use unified CMakeLists.txt with conditional compilation for different platforms
- Create distinct targets instead of commenting out code
- Document hardware-specific requirements in code comments
- As much code as possible should be as platform-agnostic as possible. Platform specific touchpoints should be hidden from business logic by common interfaces that we can swap based on the intended build/execution context.
- We must have a simulated/virtualized environment we can use to run functionality outside of the embedded context.
- prefer typescript over python where possible