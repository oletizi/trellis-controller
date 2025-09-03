# NeoTrellis M4 Step Sequencer

A C++ implementation of a step sequencer for the AdaFruit NeoTrellis M4 platform.

## Features

- 4-track, 16-step sequencer
- Adjustable tempo (BPM)
- Visual feedback with RGB LEDs
- Real-time pattern editing
- Per-track volume and mute controls

## Hardware Requirements

- AdaFruit NeoTrellis M4 (SAMD51-based)
- ARM GCC toolchain (arm-none-eabi-gcc)
- BOSSAC for flashing

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