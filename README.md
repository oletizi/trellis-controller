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

## Building

### Prerequisites

Install the ARM toolchain:
```bash
# macOS
brew install gcc-arm-embedded

# Linux
sudo apt-get install gcc-arm-none-eabi

# Or download from ARM website
```

### Compilation

```bash
./build.sh
```

Or manually:
```bash
mkdir build
cd build
cmake ..
make
```

## Flashing

Connect your NeoTrellis M4 via USB and double-tap the reset button to enter bootloader mode. Then:

```bash
cd build
make flash
```

## Project Structure

```
.
├── CMakeLists.txt          # Build configuration
├── src/
│   ├── main.cpp           # Application entry point
│   ├── StepSequencer.cpp  # Sequencer logic
│   └── NeoTrellis.cpp     # Hardware abstraction
├── include/
│   ├── StepSequencer.h    # Sequencer interface
│   └── NeoTrellis.h       # NeoTrellis driver
└── build.sh               # Build script
```

## Usage

The 4x8 button grid represents:
- Rows 0-3: Four separate tracks
- Columns 0-15: 16 steps (using first 8 columns, can expand)

### Controls
- Press button: Toggle step on/off
- Red LED: Active step
- Blue LED: Current playhead position (inactive step)
- Green LED: Current playhead position (active step)

## Development Notes

The NeoTrellis wrapper provides a simplified interface to the hardware. The actual I2C communication and Seesaw protocol implementation would need to be completed based on the AdaFruit libraries or datasheet specifications.

Key areas for enhancement:
1. Complete I2C/Seesaw communication in NeoTrellis.cpp
2. Add MIDI output support
3. Implement pattern save/load
4. Add swing/shuffle timing
5. Support for different track lengths