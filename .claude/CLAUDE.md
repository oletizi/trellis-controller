# Trellis Controller - Advanced Multi-Platform C++ Sequencer

## Project Overview
This is a sophisticated embedded C++ project implementing a step sequencer for the NeoTrellis M4 with a **multi-platform architecture**. The project features complete hardware abstraction, dependency injection patterns, and dual build targets (embedded + simulation) using modern C++17 practices.

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

### Dependency Injection Pattern
**Mandatory pattern** throughout the codebase:

```cpp
// All classes accept dependencies via constructor
struct Dependencies {
    IClock* clock = nullptr;
    IAudioOutput* audio = nullptr;
    IMidiOutput* midi = nullptr;
};

StepSequencer sequencer(Dependencies{&customClock, &audioOut});
```

## Hardware Specifications

### NeoTrellis M4 (SAMD51J19A)
- **CPU**: ARM Cortex-M4F @ 120MHz with FPU
- **RAM**: 192KB
- **Flash**: 512KB
- **Interface**: 4×8 capacitive touch grid (32 buttons)
- **Display**: 32 RGB NeoPixels
- **Communication**: I2C Seesaw protocol
- **Audio**: I2S capable
- **Bootloader**: UF2 compatible

## Build System Architecture

### Dual Build System
**Critical Understanding**: This project uses **two complementary build systems**:

1. **Arduino CLI** (Primary for hardware)
   - Hardware deployment to NeoTrellis M4
   - UF2 bootloader integration
   - Adafruit SAMD core
   - Native Arduino library ecosystem

2. **CMake** (Development & Testing)
   - Host simulation with ncurses
   - Unit testing with Catch2
   - Code coverage analysis
   - Cross-platform development

### CMake Configuration
**Key pattern**: Single `CMakeLists.txt` with conditional compilation:

```cmake
option(BUILD_SIMULATION "Build simulation for host platform" OFF)

if(BUILD_SIMULATION)
    # Host build with ncurses simulation
    add_executable(trellis_simulation ${SIMULATION_SOURCES})
else()
    # Embedded build for SAMD51
    add_executable(TrellisStepSequencer.elf ${EMBEDDED_SOURCES})
endif()
```

## Code Quality Standards

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

### Memory Management
- **No dynamic allocation** in real-time paths
- **RAII for resource management**
- **Static buffers** for predictable memory usage
- **Memory pools** for dynamic-like behavior

### C++17 Embedded Guidelines
- **No exceptions** in embedded builds (`-fno-exceptions`)
- **No RTTI** (`-fno-rtti`)
- **Const correctness** throughout
- **Template metaprogramming** for zero-overhead abstractions
- **Fixed-point math** for performance-critical calculations

### File Organization
- **300-500 line maximum** per file
- **Platform-specific code** isolated to implementation directories
- **Interfaces in `/include/core/`**
- **Business logic in `/src/core/`**

## Directory Structure

```text
trellis-controller/
├── CMakeLists.txt          # Unified build configuration
├── Makefile               # High-level build automation
├── src/
│   ├── core/              # Platform-agnostic business logic
│   │   ├── StepSequencer.cpp   # Main sequencer engine
│   │   └── ShiftControls.cpp   # Control handling
│   ├── simulation/        # Host simulation (ncurses)
│   │   ├── main.cpp
│   │   ├── CursesDisplay.cpp
│   │   └── CursesInput.cpp
│   └── embedded/          # NeoTrellis M4 hardware
│       ├── main.cpp
│       ├── NeoTrellisDisplay.cpp
│       ├── NeoTrellisInput.cpp
│       ├── SeesawI2C.cpp
│       └── SAMD51_I2C.cpp
├── include/
│   ├── core/              # Platform-agnostic interfaces
│   ├── simulation/        # Simulation headers
│   └── embedded/          # Hardware headers
├── test/                  # Unit tests with mocks
├── arduino_trellis/       # Arduino CLI project
├── build/                 # Embedded build outputs
├── build-simulation/      # Simulation build outputs
└── build-test/           # Test build outputs
```

## Development Workflows

### Common Build Commands

```bash
# Simulation development
make simulation          # Build and run simulation
make simulation-build    # Build only
make simulation-run      # Run existing build

# Embedded development (Arduino CLI - Primary)
cd arduino_trellis
arduino-cli compile --fqbn adafruit:samd:adafruit_trellis_m4 .
arduino-cli upload --fqbn adafruit:samd:adafruit_trellis_m4 -p /dev/cu.usbmodem* .

# Alternative CMake embedded build (development/testing)
make build               # Build embedded target
# Note: Use Arduino CLI for actual hardware flashing

# Testing and quality
make test               # Run unit tests
make coverage           # Generate coverage report
make format             # Format code
make lint               # Static analysis
```

### Hardware Debugging
- **Serial debugging**: `DebugSerial.cpp` for embedded output
- **Arduino CLI flashing**: Primary deployment method via UF2 bootloader
- **SWD debugging**: Hardware breakpoints via J-Link/Black Magic Probe (if available)
- **Memory profiling**: Stack usage monitoring

## Real-Time Constraints

### Timing Requirements
- **Sequencer precision**: ±0.1ms step timing
- **Button scan rate**: Minimum 100Hz
- **LED refresh**: 30-60 FPS
- **I2C timing**: Respect Seesaw protocol constraints
- **Audio callback**: <5ms latency for future audio features

### Interrupt Safety
- **Minimal ISRs**: Keep interrupt handlers short
- **Atomic operations**: For shared variables
- **Lock-free algorithms**: Avoid blocking in real-time paths
- **Circular buffers**: For event queuing

## Architecture Patterns

### Hardware Abstraction Implementation
```cpp
class NeoTrellisDisplay : public IDisplay {
    // Concrete hardware implementation
    void setPixel(uint8_t index, uint32_t color) override;
    void show() override;
};

class CursesDisplay : public IDisplay {
    // Simulation implementation
    void setPixel(uint8_t index, uint32_t color) override;
    void show() override;
};
```

### Dependency Injection in Practice
```cpp
// Application setup (platform-specific)
auto display = std::make_unique<NeoTrellisDisplay>();  // or CursesDisplay
auto input = std::make_unique<NeoTrellisInput>();      // or CursesInput

StepSequencer::Dependencies deps{
    .display = display.get(),
    .input = input.get()
};

auto sequencer = std::make_unique<StepSequencer>(deps);
```

## Agent Collaboration Guidelines

### For Agent Teams Working on This Project:

1. **cpp-pro**: 
   - Focus on modern C++17 patterns suitable for embedded
   - Implement template metaprogramming for zero-overhead
   - Ensure const correctness and RAII patterns

2. **embedded-systems**: 
   - Respect SAMD51 hardware constraints
   - Implement interrupt-safe code
   - Optimize for deterministic execution

3. **build-engineer**: 
   - Maintain CMake and Arduino CLI compatibility
   - Ensure conditional compilation works correctly
   - Keep build artifacts properly organized

4. **test-automator**: 
   - Create hardware simulation tests
   - Maintain unit test coverage >80%
   - Implement mock objects for dependencies

5. **architect-reviewer**: 
   - Validate platform abstraction boundaries
   - Ensure dependency injection patterns
   - Review interface design consistency

6. **debugger**: 
   - Use hardware-aware debugging techniques
   - Leverage serial debugging for embedded
   - Monitor memory and stack usage

7. **performance-engineer**: 
   - Optimize for real-time constraints
   - Profile memory usage patterns
   - Minimize interrupt latency

8. **code-reviewer**: 
   - Enforce error handling philosophy (no fallbacks)
   - Check platform abstraction violations
   - Verify file size limits (300-500 lines)

## Critical Don'ts

❌ **NEVER implement fallbacks or mock data** outside test code  
❌ **NEVER tie business logic to specific platforms**  
❌ **NEVER use dynamic allocation** in real-time paths  
❌ **NEVER comment out CMake targets** - use conditional compilation  
❌ **NEVER ignore the dependency injection pattern**  
❌ **NEVER bypass the build system** - use provided targets  
❌ **NEVER create files >500 lines** - refactor for modularity  
❌ **NEVER use blocking operations** in main loop  
❌ **NEVER ignore memory constraints** - SAMD51 has limited RAM  

## Success Criteria for AI Agents

✅ **Code compiles** on both simulation and embedded targets  
✅ **Build system works** - both Arduino CLI and CMake  
✅ **Platform abstraction maintained** - no business logic in platform code  
✅ **Dependency injection used** throughout  
✅ **Error messages are descriptive** with context  
✅ **No fallbacks implemented** - errors thrown instead  
✅ **Files appropriately sized** (<500 lines)  
✅ **Memory usage fits** SAMD51 constraints  
✅ **Real-time constraints met**  
✅ **Tests maintain coverage** >80%  

## Future Expansion

### Planned Features
- MIDI output support
- Pattern save/load to flash
- Multiple pattern chains
- Per-track parameters
- Audio synthesis integration

### Architecture Ready For
- **Additional platforms**: Easy to add new IDisplay/IInput implementations  
- **New sequencer modes**: Business logic isolated from platform concerns  
- **Enhanced testing**: Mock-friendly architecture  
- **Performance optimization**: Platform-specific optimizations without core changes  

## Resources

- **SAMD51 Datasheet**: Hardware register programming
- **Adafruit NeoTrellis M4 Guide**: Hardware-specific features
- **Arduino SAMD Core**: Library compatibility
- **Seesaw Protocol**: I2C communication details
- **ARM Cortex-M4 Programming Manual**: Processor optimization
- **C++ Core Guidelines**: Modern C++ practices