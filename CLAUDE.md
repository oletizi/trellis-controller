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
class IDisplay;       // LED/display abstraction
class IInputLayer;    // Platform-agnostic input abstraction
class IGestureDetector; // High-level gesture recognition
class InputController; // Input processing coordinator
class IClock;         // Timing abstraction
class IMidiOutput;    // MIDI output abstraction
class IMidiInput;     // MIDI input abstraction

// Platform implementations
CursesDisplay;        // Terminal-based simulation
NeoTrellisDisplay;    // Hardware RGB LEDs
CursesInputLayer;     // Desktop keyboard simulation  
NeoTrellisInputLayer; // Hardware I2C button matrix
MockInputLayer;       // Testing with programmable events
ArduinoMidiOutput;    // Hardware USB MIDI
```

### 64-Bit State Encoding Architecture (MANDATORY)
**CRITICAL**: This project uses a sophisticated **single source of truth** state management system where ALL state information is encoded in a single 64-bit `InputState` variable:

```cpp
// MANDATORY ARCHITECTURE: Single 64-bit state container
struct InputState {
    union {
        uint64_t raw;           // Single atomic 64-bit state
        struct {
            uint32_t buttonStates;  // Bits 0-31: All 32 button states
            uint8_t modifiers;      // Bits 32-39: Parameter lock, shift flags
            uint8_t context;        // Bits 40-47: Lock button ID, context
            uint8_t timingInfo;     // Bits 48-55: Hold duration buckets
            uint8_t reserved;       // Bits 56-63: Future expansion
        };
    };
};

// Pure functional state transitions ONLY
class InputStateEncoder {
    InputState processInputEvent(const InputState& current, const InputEvent& event);
    // ALL state management goes through this single function
};
```

#### State Management Rules (MANDATORY FOR ALL AGENTS):
1. **Single Source of Truth**: ALL state exists ONLY in the 64-bit InputState
2. **No Distributed State**: NEVER create separate state containers (e.g., `currentKeys_`, `previousKeys_`)
3. **Pure Functional**: State transitions are pure functions with no side effects
4. **Platform Abstraction**: Input layers detect events, NEVER manage state
5. **Atomic Operations**: Entire state fits in single CPU register for race-free updates

#### What Goes in InputState (49 of 64 bits used):
- ✅ **Button States**: 32 bits (1 per NeoTrellis button)
- ✅ **Parameter Lock**: 1 bit active flag + 5 bits for trigger button ID
- ✅ **Timing Info**: 8 bits for hold duration (20ms resolution, 5.1s max)
- ✅ **Modifiers**: 3 bits for shift/uppercase/etc, 5 bits available
- ✅ **Future Use**: 15 bits available for expansion

#### What NEVER Goes in Separate Variables:
- ❌ **NO** `std::unordered_map<int, bool> keyStates`
- ❌ **NO** `std::unordered_set<int> pressedKeys`  
- ❌ **NO** `bool parameterLockActive` separate flag
- ❌ **NO** timing or duration variables outside InputState
- ❌ **NO** state comparison logic in platform layers

#### Agent Implementation Rules:
- **InputLayer**: ONLY report current detections, ZERO state memory
- **InputStateEncoder**: ONLY place that manages/modifies InputState
- **All Other Classes**: Receive InputState, never modify it
- **State Transitions**: Must be pure functions: `(oldState, event) → newState`

### Input Layer Abstraction Architecture (v2.0)
**Critical**: The input system uses sophisticated layered abstraction for complete separation of concerns:

```cpp
// Input Processing Pipeline:
// InputLayer → InputEvents → GestureDetector → ControlMessages → Sequencer

// 1. Platform-specific input layers
class IInputLayer {
    virtual bool poll() = 0;                    // Check hardware for events
    virtual bool getNextEvent(InputEvent&) = 0; // Retrieve platform events
    virtual bool hasEvents() const = 0;         // Non-blocking event check
};

// 2. Platform-agnostic gesture recognition
class IGestureDetector {
    virtual uint8_t processInputEvent(const InputEvent&, 
                                     std::vector<ControlMessage>&) = 0;
    virtual bool isInParameterLockMode() const = 0;
};

// 3. Coordinated input processing
class InputController {
    bool poll();                               // Process complete pipeline
    bool getNextMessage(ControlMessage&);      // Semantic output messages
};
```

#### Input Architecture Benefits:
- **Platform Independence**: Same gesture logic works on hardware, simulation, testing
- **Sophisticated Gestures**: Hold detection, parameter locks, timing-based recognition
- **Semantic Interface**: StepSequencer receives `TOGGLE_STEP`, `ENTER_PARAM_LOCK` messages
- **Real-time Safe**: No dynamic allocation, bounded execution time
- **Testable**: MockInputLayer provides programmable event sequences

#### Migration from Legacy IInput:
- **Before**: `sequencer->handleButton(id, pressed, time)` (platform-coupled)  
- **After**: `controller->poll(); controller->getNextMessage(msg); sequencer->processMessage(msg)` (semantic)

### Bank-Aware Parameter Lock Control System (MANDATORY)
**CRITICAL**: This project implements a sophisticated bank-aware control system where parameter lock controls depend on the spatial location of the trigger button:

```cpp
// MANDATORY ARCHITECTURE: Bank-aware parameter controls
struct ParameterLockBanks {
    // Left Bank Parameter Lock Keys: <1-4 qwer asdf zxcv> (16 keys)
    static constexpr uint8_t LEFT_BANK_BUTTONS[16] = {
        0,1,2,3,    // Row 0: 1-4
        8,9,10,11,  // Row 1: qwer  
        16,17,18,19, // Row 2: asdf
        24,25,26,27  // Row 3: zxcv
    };
    
    // Right Bank Parameter Lock Keys: <5678 tyui ghjk bnm,> (16 keys)
    static constexpr uint8_t RIGHT_BANK_BUTTONS[16] = {
        4,5,6,7,     // Row 0: 5678
        12,13,14,15, // Row 1: tyui
        20,21,22,23, // Row 2: ghjk  
        28,29,30,31  // Row 3: bnm,
    };
};

// Bank-aware control mapping
class BankAwareControls {
public:
    static ControlMapping getControlsForTrigger(uint8_t triggerButtonId) {
        bool triggerInLeftBank = isLeftBankButton(triggerButtonId);
        
        if (triggerInLeftBank) {
            // LEFT bank trigger → RIGHT bank controls
            return ControlMapping{
                .decrementNote = 28,    // 'b' button
                .incrementNote = 20,    // 'g' button  
                .decrementVelocity = 29, // 'n' button
                .incrementVelocity = 21  // 'h' button
            };
        } else {
            // RIGHT bank trigger → LEFT bank controls  
            return ControlMapping{
                .decrementNote = 24,    // 'z' button
                .incrementNote = 16,    // 'a' button
                .decrementVelocity = 25, // 'x' button
                .incrementVelocity = 17  // 's' button
            };
        }
    }
    
private:
    static bool isLeftBankButton(uint8_t buttonId) {
        uint8_t column = buttonId % 8;
        return column < 4;  // Columns 0-3 = left bank
    }
};
```

#### Bank-Aware Control Rules (MANDATORY FOR ALL AGENTS):

1. **Spatial Separation**: Parameter lock trigger and controls are ALWAYS in opposite banks
2. **Only 4 Control Functions**: 
   - Decrement MIDI note value
   - Increment MIDI note value  
   - Decrement MIDI velocity value
   - Increment MIDI velocity value
3. **Bank-Specific Mapping**: Control keys change based on trigger location
4. **NO Unauthorized Controls**: No octave, gate, probability, or other controls
5. **Deterministic Mapping**: Same trigger button always maps to same control buttons

#### Control Key Mappings:

**When parameter lock key is in LEFT bank → control keys are in RIGHT bank:**
- `b` (button 28) = Decrement MIDI note value
- `g` (button 20) = Increment MIDI note value  
- `n` (button 29) = Decrement MIDI velocity value
- `h` (button 21) = Increment MIDI velocity value

**When parameter lock key is in RIGHT bank → control keys are in LEFT bank:**
- `z` (button 24) = Decrement MIDI note value
- `a` (button 16) = Increment MIDI note value
- `x` (button 25) = Decrement MIDI velocity value
- `s` (button 17) = Increment MIDI velocity value

#### Implementation Requirements:
- **Visual Feedback**: Highlight active control keys based on current trigger bank
- **Inactive Keys**: All non-control keys must be inactive during parameter lock
- **Bank Detection**: Runtime determination of trigger bank and corresponding controls
- **Error Prevention**: Reject any control key presses not matching current bank mapping

#### Usage Examples:
```cpp
// Example: Left bank trigger
// SHIFT+q (button 9) enters parameter lock
// Available controls: b,g,n,h (right bank buttons 28,20,29,21)

// Example: Right bank trigger  
// SHIFT+5 (button 4) enters parameter lock
// Available controls: z,a,x,s (left bank buttons 24,16,25,17)
```

### Arduino Platform Guidelines
**CRITICAL**: Arduino `.ino` files must remain as thin as possible and contain ONLY device-specific translation code:

#### ✅ Allowed in Arduino .ino files:
- Hardware initialization (`trellis.begin()`, pin setup)
- Device-specific I/O translation (button indices → row/col)
- Platform-specific library includes (`<Adafruit_NeoTrellisM4.h>`, `<MIDIUSB.h>`)
- Simple data format conversion (uint32_t colors, MIDI message bytes)
- Hardware-specific timing constraints

#### ❌ NEVER allowed in Arduino .ino files:
- **Business logic** (sequencer patterns, BPM calculations, step advancement)
- **Application state** (pattern storage, current step, playing state)
- **MIDI logic** (note mapping, velocity calculations, timing)
- **Control flow** (start/stop logic, step sequencing)
- **Feature implementation** (shift controls, pattern editing)

#### Single Arduino File Rule
**MANDATORY**: Maintain exactly ONE Arduino `.ino` file per hardware target:
- **ONE** `arduino_trellis.ino` for NeoTrellis M4
- **NO** separate files for different features (MIDI, basic, etc.)
- Feature support controlled by dependency injection in core code
- Arduino file only provides hardware interface implementations

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
- **Template metaprogramming** for zero-overhead abstractions
- **No exceptions** in embedded builds (`-fno-exceptions`)
- **No RTTI** (`-fno-rtti`)
- All code must be testable via dependency injection

### File Organization
- **300-500 line maximum** per file
- **Platform-specific code** isolated to implementation directories
- **Interfaces in `/include/core/`**
- **Business logic in `/src/core/`**

### Repository Hygiene
- **Build artifacts ONLY in `build/` directory** (configured in .gitignore)
- **ABSOLUTELY NO temporary files in top-level directory** - NO build scripts, test scripts, logs, or ANY generated files
- **Keep top-level directory PRISTINE** - only essential project files (CMakeLists.txt, Makefile, README, etc.)
- **NO temporary scripts, logs, or generated files committed to git** - EVER
- **Never bypass pre-commit or pre-push hooks** - fix issues instead
- Clean repository is mandatory

#### 🚨 TOP-LEVEL DIRECTORY CLEANLINESS RULES:
- **NO** `*.sh` scripts except essential project scripts in proper locations
- **NO** temporary test files, build files, or debugging artifacts
- **NO** convenience scripts, quick builds, or one-off utilities
- **NO** generated files, logs, or build outputs
- **USE** proper directories: scripts in `scripts/`, tests in `test/`, builds in `build*/`
- **CLEAN UP** immediately after creating temporary files for development
- **VIOLATING** this rule creates technical debt and repository pollution

## Implementation Patterns

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

## Project Structure

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

## Embedded-Specific Guidelines

### Memory Management
- **No dynamic allocation** in real-time paths
- **RAII for resource management**
- **Static buffers** for predictable memory usage
- **Memory pools** for dynamic-like behavior
- **Fixed memory allocation**: No dynamic allocation after initialization
- **Stack usage awareness**: Monitor stack depth for recursive functions

### Real-Time Constraints
- **Sequencer precision**: ±0.1ms step timing
- **Button scan rate**: Minimum 100Hz
- **LED refresh**: 30-60 FPS
- **I2C timing**: Respect Seesaw protocol constraints
- **Audio callback**: <5ms latency for future audio features
- **Deterministic execution**: Avoid unbounded loops

### Interrupt Safety
- **Minimal ISRs**: Keep interrupt handlers short
- **Atomic operations**: For shared variables
- **Lock-free algorithms**: Avoid blocking in real-time paths
- **Circular buffers**: For event queuing

### Hardware Integration
- **I2C timing**: Respect bus timing and avoid blocking
- **LED refresh rates**: Balance update frequency with CPU usage
- **Button debouncing**: Implement proper debounce algorithms
- **Power management**: Consider sleep modes when idle

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

### AI Agent Development Workflow

#### Before Making Changes
1. **Read existing code** to understand patterns and conventions
2. **Check CMakeLists.txt** to understand build configuration
3. **Review hardware datasheets** for SAMD51 and NeoTrellis
4. **Understand memory constraints** of the target platform

#### When Writing Code
1. **Use dependency injection** - pass dependencies via constructor
2. **Follow RAII principles** for resource management
3. **Write testable code** with clear interfaces
4. **Avoid dynamic allocation** in performance-critical paths
5. **Use descriptive error messages** with context
6. **Throw errors instead of fallbacks** outside of test code
7. **Keep files under 300-500 lines** - refactor if larger

#### Before Completing Tasks
1. **Build the project**: Use appropriate build commands above
2. **Check compilation**: Build should be warning-free
3. **Test on hardware** if possible

## Agent Collaboration Guidelines

### Agent Delegation Patterns (Critical for Orchestrator)

**MANDATORY**: When using the `orchestrator` agent, follow these delegation patterns to avoid "phantom changes" where agents claim implementation but only perform analysis:

#### ✅ Implementation-Capable Agents (Can Write/Edit Files):
- **`cpp-pro`**: Full C++ implementation with Read, Write, MultiEdit, Bash + C++ tools
- **`embedded-systems`**: Hardware-specific implementation with comprehensive embedded tools
- **`build-engineer`**: Build system modifications with Read, Write, MultiEdit, Bash + build tools  
- **`ui-engineer`**: Frontend code implementation with full tool access (*)
- **`backend-typescript-architect`**: Backend TypeScript implementation with full tool access (*)
- **`python-backend-engineer`**: Python backend implementation with full tool access (*)
- **`senior-code-reviewer`**: Comprehensive code review and fixes with full tool access (*)
- **`general-purpose`**: Fallback for any implementation tasks with full tool access (*)

#### ❌ Analysis-Only Agents (Cannot Implement Changes):
- **`architect-reviewer`**: Only has Read, analysis tools - **CANNOT** Write/Edit files
- **`code-reviewer`**: Only has Read, Grep, analysis tools - **CANNOT** implement fixes
- **`debugger`**: Only has Read, debugging tools - **CANNOT** fix identified issues  
- **`performance-engineer`**: Only has Read, profiling tools - **CANNOT** optimize code
- **`test-automator`**: Has Write but **MISSING** Edit/MultiEdit - **CANNOT** modify existing files

#### Proper Delegation Chain:
1. **Analysis Phase**: Use analysis agents to identify what needs to be done
   ```
   orchestrator → architect-reviewer: "What architectural issues exist?"
   orchestrator → code-reviewer: "What code quality issues need fixing?"
   orchestrator → performance-engineer: "What optimizations are needed?"
   ```

2. **Implementation Phase**: Use implementation-capable agents for actual changes
   ```
   orchestrator → cpp-pro: "Implement these specific architectural changes: [details]"
   orchestrator → embedded-systems: "Optimize these performance bottlenecks: [details]"  
   orchestrator → build-engineer: "Fix these build system issues: [details]"
   ```

3. **Verification Phase**: Use review agents to validate changes
   ```
   orchestrator → senior-code-reviewer: "Review and validate these implementations"
   ```

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
   - **NOTE**: Can write new files but cannot edit existing ones

5. **architect-reviewer**: 
   - Validate platform abstraction boundaries
   - Ensure dependency injection patterns
   - Review interface design consistency
   - **NOTE**: Analysis only - cannot implement changes

6. **debugger**: 
   - Use hardware-aware debugging techniques
   - Leverage serial debugging for embedded
   - Monitor memory and stack usage
   - **NOTE**: Can identify issues but cannot fix them

7. **performance-engineer**: 
   - Optimize for real-time constraints
   - Profile memory usage patterns
   - Minimize interrupt latency
   - **NOTE**: Can analyze performance but cannot implement optimizations

8. **code-reviewer**: 
   - Enforce error handling philosophy (no fallbacks)
   - Check platform abstraction violations
   - Verify file size limits (300-500 lines)
   - **NOTE**: Analysis only - cannot fix identified issues

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

## Critical Don'ts

❌ **NEVER implement fallbacks or mock data** outside test code - throw descriptive errors instead  
❌ **NEVER tie business logic to specific platforms** - use platform abstraction  
❌ **NEVER put application logic in Arduino .ino files** - keep them thin, device-specific only  
❌ **NEVER create multiple .ino files for features** - use dependency injection instead  
❌ **NEVER duplicate core logic across platforms** - write once in `/src/core/`  
❌ **NEVER use dynamic allocation** in real-time paths  
❌ **NEVER comment out CMake targets** - use conditional compilation  
❌ **NEVER ignore the dependency injection pattern**  
❌ **NEVER bypass the build system** - use provided targets  
❌ **NEVER create files >500 lines** - refactor for modularity  
❌ **NEVER use blocking operations** in main loop - use state machines  
❌ **NEVER ignore memory constraints** - SAMD51 has limited RAM  
❌ **NEVER skip error checking** for hardware operations  
❌ **NEVER use recursion** without bounded depth  
❌ **NEVER EVER leave temporary files in the top-level directory** - CLEAN UP YOUR TURDS IMMEDIATELY  
❌ **NEVER implement unauthorized parameter controls** - only note up/down, velocity up/down allowed  
❌ **NEVER ignore the bank-aware control system** - controls must be in opposite bank from trigger  

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
✅ **Top-level directory remains pristine** - no temporary files or build turds left behind  
✅ **Bank-aware controls implemented** - parameter controls in opposite bank from trigger  
✅ **Only authorized controls** - exactly 4 control functions (note up/down, velocity up/down)  

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

## Key Principles Summary

- **Platform abstraction is mandatory** - zero business logic in platform code
- **Dependency injection throughout** - all classes accept dependencies via constructor
- **Error handling philosophy** - throw descriptive errors, never use fallbacks
- **Dual build system** - Arduino CLI for hardware, CMake for development/testing
- **Memory constraints matter** - SAMD51 has limited resources
- **Real-time performance required** - deterministic execution patterns
- **Testing via simulation** - ncurses-based host simulation for development
- **Code organization** - files <500 lines, clear separation of concerns