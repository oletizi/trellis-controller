# NeoTrellis M4 Step Sequencer

A **multi-platform C++ implementation** of a step sequencer with hardware abstraction for true portability. Features both embedded ARM target (NeoTrellis M4) and host simulation environment.

## Features

### Core Sequencer Engine
- 4-track, 8-step sequencer (configurable)
- Adjustable tempo (120 BPM default)
- Visual feedback with RGB LEDs
- Real-time pattern editing
- Dependency injection architecture for testability

### Multi-Platform Support
- **Embedded Target**: AdaFruit NeoTrellis M4 (SAMD51J19A ARM Cortex-M4F)
- **Simulation Environment**: Cross-platform terminal interface with colored display
- **Shared Business Logic**: Platform-agnostic core with hardware abstraction

### Development Tools
- Comprehensive unit test suite with 81%+ code coverage
- Host-based simulation for development without hardware
- Automated build system with multiple targets

## Architecture

### Platform Abstraction
Following project guidelines, **all build-time and runtime environments are abstracted** from the core logic:

```cpp
// Core interfaces (platform-agnostic)
class IDisplay    // LED/display abstraction
class IInput      // Button/input abstraction  
class IClock      // Timing abstraction

// Platform implementations
CursesDisplay     // Terminal-based simulation
NeoTrellisDisplay // Hardware RGB LEDs

CursesInput       // Keyboard simulation
NeoTrellisInput   // Hardware button matrix
```

### Directory Structure
```
├── include/
│   ├── core/           # Platform-agnostic interfaces
│   ├── simulation/     # Host simulation implementations
│   └── embedded/       # Hardware implementations
├── src/
│   ├── core/           # Business logic (StepSequencer)
│   ├── simulation/     # Curses-based simulation
│   └── embedded/       # NeoTrellis M4 hardware code
├── test/               # Unit tests with mocks
└── build-*/            # Platform-specific build outputs
```

## Hardware Requirements

- **Target**: AdaFruit NeoTrellis M4 (SAMD51J19A, 120MHz ARM Cortex-M4F)
- **Memory**: 512KB Flash, 192KB RAM
- **Interface**: 4×8 capacitive touch grid with RGB LEDs
- **Toolchain**: ARM GCC cross-compiler (arm-none-eabi-gcc)
- **Flash Tool**: BOSSA bootloader

## Quick Start

### Prerequisites
```bash
# Install dependencies (macOS)
make install-deps

# Verify toolchain
make check
```

### Build & Run Simulation
```bash
# Build and run interactive simulation
make simulation

# Or build/run separately  
make simulation-build
make simulation-run
```

### Embedded Development
```bash
# Build firmware for NeoTrellis M4
make build

# Flash to device (connect via USB, double-tap reset)
make flash

# View firmware info
make size
```

### Testing & Quality
```bash
# Run unit tests
make test

# Generate coverage report 
make coverage
# View: build-test/coverage_html/index.html

# Code formatting & analysis
make format
make lint
```

## Usage

### Simulation Environment
The terminal simulation provides full step sequencer functionality:

```
NeoTrellis M4 Step Sequencer Simulator - 4x8 Grid
Step Sequencer: RED=Track0, GREEN=Track1, BLUE=Track2, YELLOW=Track3 | Press ESC to quit

##  ##  ##  ##  ##  ##  ##  ##     ← Colored blocks show LED states
##  ##  ##  ##  ##  ##  ##  ##     ← Bright = current playback position  
##  ##  ##  ##  ##  ##  ##  ##     ← Colors per track (Red/Green/Blue/Yellow)
##  ##  ##  ##  ##  ##  ##  ##

Controls: Press keys to toggle steps on/off. Bright colors = current playback position
Track 0 (RED):    1 2 3 4 5 6 7 8    |  Track 1 (GREEN):  Q W E R T Y U I
Track 2 (BLUE):   A S D F G H J K    |  Track 3 (YELLOW): Z X C V B N M ,
Sequencer is playing at 120 BPM with a demo pattern. Modify it!
```

**Keyboard Layout:**
```
Row 0 (Track 0): 1 2 3 4 5 6 7 8
Row 1 (Track 1): Q W E R T Y U I  
Row 2 (Track 2): A S D F G H J K
Row 3 (Track 3): Z X C V B N M ,
```

### Hardware Controls
**NeoTrellis M4 4×8 Grid:**
- **Button Press**: Toggle step on/off
- **LED Colors**: 
  - **Red**: Track 0 active steps
  - **Green**: Track 1 active steps  
  - **Blue**: Track 2 active steps
  - **Yellow**: Track 3 active steps
  - **Bright**: Current playback position
  - **Dim White**: Current position (inactive step)

## Technical Implementation

### Dependency Injection Pattern
The project uses **dependency injection** throughout for maximum testability:

```cpp
// StepSequencer accepts dependencies via constructor
struct Dependencies {
    IClock* clock = nullptr;
};
StepSequencer sequencer(deps);

// Platform-specific implementations injected at runtime
CursesDisplay display;           // Simulation
NeoTrellisDisplay display;       // Hardware
```

### Build System
**Dual CMake Configuration:**
- **Embedded**: Cross-compilation for ARM with custom linker script
- **Simulation**: Host build with ncurses for terminal interface  
- **Testing**: Host build with Catch2 framework and coverage analysis

**Self-Documenting Makefile:**
```bash
make help    # Shows all available targets
make check   # Verifies build environment
```

### Testing Architecture
- **Mock Objects**: `MockClock` for deterministic timing tests
- **Platform Isolation**: Core logic tested independently of hardware
- **Coverage Reporting**: HTML reports with line-by-line analysis
- **81%+ Coverage**: Exceeds project standards

### Multi-Platform Compatibility  
**Simulation Benefits:**
- Rapid development without hardware
- Visual debugging of sequencer patterns
- Cross-platform development (macOS/Linux)
- Real-time interaction testing

**Embedded Benefits:**
- Zero-copy LED updates with dirty tracking
- Event-driven input with circular buffer  
- Static memory allocation (no dynamic allocation)
- Optimized for 120MHz ARM Cortex-M4F

## Development Roadmap

### Phase 1: Core Infrastructure ✅
- [x] Platform abstraction interfaces
- [x] StepSequencer business logic  
- [x] Unit test framework with coverage
- [x] Curses simulation environment
- [x] Embedded hardware abstraction

### Phase 2: Hardware Integration ⚠️
- [x] NeoTrellis display implementation
- [x] NeoTrellis input implementation  
- [ ] Complete I2C/Seesaw protocol communication
- [ ] Hardware button matrix reading
- [ ] LED brightness and color calibration

### Phase 3: Features 🔄
- [ ] Pattern save/load to flash memory
- [ ] MIDI output support
- [ ] Swing/shuffle timing variations
- [ ] Per-track BPM and length settings
- [ ] Chain multiple patterns

### Phase 4: Polish 🔄
- [ ] User interface improvements
- [ ] Performance optimization
- [ ] Documentation completion
- [ ] Hardware testing and validation

## Contributing

The codebase follows **strict platform abstraction guidelines**:
1. **Core logic must remain platform-agnostic** 
2. **All hardware dependencies use interfaces**
3. **New features require unit tests**
4. **Simulation implementation required for development**

See `CLAUDE.md` for detailed coding standards.