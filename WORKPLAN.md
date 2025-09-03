# NeoTrellis M4 Simulation Environment - Work Plan

## Project Overview
Create a laptop-based simulation environment for the NeoTrellis M4 (4x8 grid) where LED states are displayed as colored blocks in a terminal interface and button presses are simulated via keyboard input.

## Hardware Configuration
- **NeoTrellis M4**: 4 rows × 8 columns = 32 buttons/LEDs
- **Display**: RGB LEDs represented as colored terminal blocks
- **Input**: Keyboard keys mapped to button positions

## Implementation Steps

### Phase 1: Architecture & Interfaces ✅
- [x] Create WORKPLAN.md documentation
- [x] Update StepSequencer constants for 4x8 grid (changed from 4x16)
- [x] Create IDisplay interface for hardware abstraction
  - Methods: setLED(row, col, r, g, b), clear(), refresh()
- [x] Create IInput interface for button/keyboard abstraction
  - Methods: pollEvents(), getNextEvent(), isButtonPressed()
  - Events: ButtonEvent structure with row/col/pressed/timestamp

### Phase 2: Curses Implementation ✅
- [x] Implement CursesDisplay class
  - RGB color support (256-color with 6x6x6 cube mapping)
  - 4x8 grid layout with colored blocks (██)
  - Real-time LED state updates with dirty tracking
- [x] Implement CursesInput class
  - Keyboard mapping (32 keys for 4x8 grid)
  - Non-blocking input handling with event queue
  - Key press/release toggle simulation

### Phase 3: Integration ✅
- [x] Create simulation main executable
  - Initialize curses environment with error handling
  - Wire up StepSequencer with simulation interfaces
  - Main event loop for input/display updates (60 FPS)
- [x] Update CMake build system
  - Add ncurses dependency via pkg-config
  - Create simulation build target (CMakeLists-simulation.txt)
  - Makefile targets: simulation, simulation-build, simulation-run

### Phase 4: Testing & Validation ⏳
- [x] Build system validation (successful compilation)
- [x] CMake configuration with ncurses linking
- [ ] Test RGB color display accuracy
- [ ] Validate keyboard input mapping
- [ ] Test step sequencer functionality in simulation
- [ ] Performance testing (real-time responsiveness)

## Keyboard Layout Plan
```
Row 0: 1 2 3 4 5 6 7 8
Row 1: Q W E R T Y U I  
Row 2: A S D F G H J K
Row 3: Z X C V B N M ,
```

## Technical Requirements
- **Dependencies**: ncurses library
- **Color Support**: 256-color or true color terminals
- **Platform**: macOS/Linux compatible
- **Integration**: Use existing dependency injection pattern

## Status Legend
- ⏳ Planning/In Progress
- ✅ Complete
- 🔄 Not Started
- ❌ Blocked

---
*Last Updated: 2025-09-03*