# Trellis Controller Simulation Guide

## Overview

The Trellis Controller includes a terminal-based simulation that emulates the NeoTrellis M4 hardware step sequencer.
This simulation provides a visual 4×8 grid interface with full parameter lock functionality, allowing you to develop and
test sequencer patterns without physical hardware.

## Running the Simulation

### Quick Start

```bash
# From project root
make simulation

# Or build and run separately
make simulation-build
./build-simulation/trellis_simulation
```

### Exit the Simulation

Press **ESC** at any time to quit the simulation.

## Key Controls

### Grid Layout

The simulation maps keyboard keys to a 4×8 button grid representing 4 tracks with 8 steps each:

| Track       | Color  | Row    | Keys                            |
|-------------|--------|--------|---------------------------------|
| **Track 0** | RED    | Top    | `1` `2` `3` `4` `5` `6` `7` `8` |
| **Track 1** | GREEN  | Second | `q` `w` `e` `r` `t` `y` `u` `i` |
| **Track 2** | BLUE   | Third  | `a` `s` `d` `f` `g` `h` `j` `k` |
| **Track 3** | YELLOW | Bottom | `z` `x` `c` `v` `b` `n` `m` `,` |

### Basic Operations

#### Toggle Steps

**Quick press** any key to toggle the corresponding step on/off.

- Press `q` → Toggle Track 1, Step 0
- Press `5` → Toggle Track 0, Step 4
- Press `m` → Toggle Track 3, Step 6

Active steps appear as bright colored squares in the grid display.

### Parameter Locks

Parameter locks allow you to adjust per-step parameters like pitch, velocity, and timing while the sequencer is running.

#### How Parameter Locks Work

1. **Enter Parameter Lock Mode**
    - **Hold** any grid key for **≥500ms** (half a second)
    - The key must remain held for the entire parameter lock session
    - Visual feedback shows parameter lock mode is active

2. **Adjust Parameters**
    - While **still holding** the trigger key, press control keys to adjust parameters
    - Each control key press modifies different parameter types
    - The specific parameter affected depends on the control key clicked

3. **Exit Parameter Lock Mode**
    - **Release** the held key to immediately exit parameter lock mode
    - All parameter adjustments are saved to that step

#### Parameter Lock Keys and Control Keys

There are two banks of keys in the sequencer grid:

* The left bank, comprising the left-most 16 keys: <1-4 qwer asdf zxcv>
* The right bank, comprising the right-most 16 keys: <5678 tyui ghjk bnm,>

When entering parameter lock mode by pressing and holding a step key, the following happen:

* All other keys in that bank become inactive
* All keys in the opposite bank become control keys
* For the duration of parameter lock mode, all sequencer mode functions are disabled, e.g., toggling active state on or
  off

#### Control Key Layout

While in parameter lock mode, the following describes the control key layout

When the held key (parameter lock key) is in the left bank, the control keys are in the right bank:

| Control Key | Action                | Function                                 |
|-------------|-----------------------|------------------------------------------|
| b           | Press and release key | Decrement MIDI note value                |
| g           | Press and release key | Increment MIDI note value                |
| n           | Press and release key | Decrement MIDI velocity value            |
| h           | Press and release key | Increment MIDI note value                |
| all others  | Any                   | Inactive (reserved for future functions) |

When the held key (parameter lock key) is in the right bank, the control keys are in the left bank:

| Control Key | Action                | Function                                 |
|-------------|-----------------------|------------------------------------------|
| z           | Press and release key | Decrement MIDI note value                |
| a           | Press and release key | Increment MIDI note value                |
| x           | Press and release key | Decrement MIDI velocity value            |
| s           | Press and release key | Increment MIDI note value                |
| all others  | Any                   | Inactive (reserved for future functions) |


#### Parameter Lock Examples

```
Example 1: Adjust Track 1, Step 0 parameters
1. Hold 'q' for ≥500ms (keep holding!)
2. While holding 'q', press 'w' to increase pitch
3. While holding 'q', press 'a' to decrease velocity
4. Release 'q' to exit parameter lock mode

Example 2: Adjust Track 0, Step 4 parameters
1. Hold '5' for ≥500ms (keep holding!)
2. While holding '5', press other number keys to adjust
3. Release '5' to exit parameter lock mode
```

#### Important Notes

- Parameter lock mode is **only active while the trigger key is held**
- Releasing the trigger key **immediately exits** parameter lock mode
- You must hold the key for at least 500ms to enter parameter lock mode
- Visual feedback in the console shows when parameter lock is active

## Visual Interface

### Grid Display

The simulation shows a 4×8 grid where:

- **Dark squares** (`##`) = Step is OFF
- **Bright colored squares** = Step is ON
- **Animated colors** = Current playback position
- **Special indicators** = Parameter lock mode active

### Color Coding

- **Red** = Track 0 (Kick/Bass)
- **Green** = Track 1 (Snare/Clap)
- **Blue** = Track 2 (Hi-hat/Percussion)
- **Yellow** = Track 3 (Melody/Effects)

### Console Output

The debug console at the bottom shows:

- Button press/release events
- Parameter lock entry/exit
- State transitions
- System messages

## Advanced Features

### Timing and Synchronization

- The sequencer runs at a configurable BPM (default 120)
- Each track can have independent pattern lengths
- Parameter locks are sample-accurate

### Parameter Types

When in parameter lock mode, different keys adjust different parameters:

| Parameter       | Effect               | Typical Range        |
|-----------------|----------------------|----------------------|
| **Pitch**       | Note frequency       | -24 to +24 semitones |
| **Velocity**    | Volume/intensity     | 0-127                |
| **Gate**        | Note duration        | 10-100% of step      |
| **Probability** | Chance of triggering | 0-100%               |

### State Management

The simulation uses a bitwise state encoding system that provides:

- Deterministic input processing
- Clean state transitions
- Robust parameter lock handling
- Real-time safe operation

## Troubleshooting

### Terminal Scrolling Issues

If terminal scrolling is captured by the application:

- Use **Shift + Page Up/Down** to scroll
- Or press **ESC** to exit, view output, then restart
- Consider using `screen` or `tmux` for better control

### Parameter Lock Not Working

Ensure you:

1. Hold the key for at least 500ms (half a second)
2. Keep the key held while adjusting parameters
3. Use lowercase keys or numbers (not uppercase)

### Display Issues

If colors don't appear correctly:

- Ensure your terminal supports 256 colors
- Try a different terminal emulator
- Check your TERM environment variable

## Keyboard Reference

### Quick Reference Card

```
┌─────────────────────────────────────────────┐
│          TRELLIS SIMULATOR CONTROLS          │
├─────────────────────────────────────────────┤
│  Track 0 (RED):    1  2  3  4  5  6  7  8   │
│  Track 1 (GREEN):  q  w  e  r  t  y  u  i   │
│  Track 2 (BLUE):   a  s  d  f  g  h  j  k   │
│  Track 3 (YELLOW): z  x  c  v  b  n  m  ,   │
├─────────────────────────────────────────────┤
│  Quick Press = Toggle Step ON/OFF            │
│  Hold ≥500ms = Enter Parameter Lock Mode     │
│  While Holding = Adjust Parameters           │
│  Release Hold = Exit Parameter Lock Mode     │
│  ESC = Quit Simulation                       │
└─────────────────────────────────────────────┘
```

## Development Notes

### Architecture

The simulation uses:

- **ncurses** for terminal UI
- **Input abstraction layers** for hardware independence
- **Bitwise state encoding** for deterministic processing
- **Real-time safe** data structures

### Building from Source

```bash
# Configure for simulation
cmake -B build-simulation -DBUILD_SIMULATION=ON

# Build
cmake --build build-simulation

# Run
./build-simulation/trellis_simulation
```

### Debug Output

Enable verbose debug output by setting environment variables:

```bash
DEBUG=1 ./build-simulation/trellis_simulation
```

## See Also

- [Project README](../README.md) - Overall project documentation
- [CLAUDE.md](../CLAUDE.md) - Development guidelines and architecture
- [Hardware Guide](https://learn.adafruit.com/adafruit-neotrellis-m4) - NeoTrellis M4 hardware reference