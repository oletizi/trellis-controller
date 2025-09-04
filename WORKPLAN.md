# Shift-Key Control Mechanism - Arduino CLI Implementation

## 🔴 CRITICAL ISSUE RESOLVED (2025-09-04)

### ✅ Root Cause Identified
**The CMake/bossac build system doesn't work reliably with the NeoTrellis M4's UF2 bootloader.**

**Evidence:**
- Device immediately re-enters bootloader mode after CMake-built firmware is flashed
- Commit `5d5a883` on feat/midi branch works perfectly using **Arduino CLI**
- CMake builds produce incompatible firmware for UF2 bootloader
- Arduino CLI properly handles all UF2 bootloader requirements

### ✅ Solution: Arduino CLI Implementation
The shift-key functionality will be implemented using Arduino CLI, following the successful pattern from the MIDI branch.

---

## Project Overview
Implement a shift-key control system for the NeoTrellis M4 step sequencer where:
- **Shift Key**: Bottom-left key (position [3,0])  
- **Control Key**: Bottom-right key (position [3,7]) for start/stop functionality
- When shift is held + control key pressed: start/stop sequencer
- When shift is not held: normal step sequencer operation

## Implementation Strategy

### Phase 1: Arduino CLI Project Setup ✅
**Use the working Arduino project structure** from commit `5d5a883`:
- Base project: `arduino_trellis/arduino_trellis.ino`
- Working build/flash process with `arduino-cli`
- Proper UF2 bootloader compatibility

### Phase 2: Port Shift-Key Logic to Arduino
**Convert existing CMake implementation** to Arduino sketch:

#### 2.1 Core Shift Controls Class (Arduino Compatible)
```cpp
// arduino_trellis/ShiftControls.h
class ShiftControls {
private:
    bool shiftActive_ = false;
    const uint8_t SHIFT_ROW = 3;
    const uint8_t SHIFT_COL = 0;
    const uint8_t CONTROL_ROW = 3; 
    const uint8_t CONTROL_COL = 7;
    
public:
    void handleInput(uint8_t row, uint8_t col, bool pressed);
    bool isShiftActive() const { return shiftActive_; }
    bool isControlKey(uint8_t row, uint8_t col) const;
    bool shouldHandleAsShiftControl(uint8_t row, uint8_t col) const;
};
```

#### 2.2 Integration with Arduino Sequencer
**Main sketch modifications** (`arduino_trellis.ino`):
```cpp
#include "Adafruit_NeoTrellis.h"

Adafruit_NeoTrellis trellis;
ShiftControls shiftControls;
// ... existing sequencer variables ...

void setup() {
    // Existing trellis setup...
    trellis.begin();
    trellis.setBrightness(50);
    
    // Register button callback with shift control handling
    for(int i=0; i<NEO_TRELLIS_NUM_KEYS; i++){
        trellis.registerCallback(i, trellisCallback);
        trellis.setPixelColor(i, 0);
    }
    trellis.show();
}

TrellisCallback trellisCallback(keyEvent evt) {
    uint8_t row = evt.bit.NUM / 8;
    uint8_t col = evt.bit.NUM % 8;
    bool pressed = evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING;
    
    // Handle shift controls first
    if (shiftControls.shouldHandleAsShiftControl(row, col)) {
        shiftControls.handleInput(row, col, pressed);
        
        // Check for shift+control combination
        if (shiftControls.isShiftActive() && 
            shiftControls.isControlKey(row, col) && pressed) {
            togglePlayback();  // Start/stop sequencer
            return 0;
        }
        
        // Update visual feedback for shift mode
        updateShiftVisualFeedback();
        return 0;
    }
    
    // Normal step sequencer input
    if (pressed && row < 4 && col < 8) {
        toggleStep(row, col);
    }
    
    return 0;
}
```

### Phase 3: Visual Feedback Implementation
**LED color scheme for shift mode**:
```cpp
void updateShiftVisualFeedback() {
    if (shiftControls.isShiftActive()) {
        // Highlight shift key in white
        trellis.setPixelColor(24, 0x404040);  // Bottom-left (dim white)
        
        // Highlight control keys in yellow when shift active
        trellis.setPixelColor(31, 0x404000);  // Bottom-right (dim yellow)
    } else {
        // Normal step sequencer colors
        updateNormalDisplay();
    }
    trellis.show();
}
```

### Phase 4: Testing and Validation

#### 4.1 Arduino CLI Build Process
```bash
# Navigate to Arduino project
cd arduino_trellis

# Compile
arduino-cli compile --fqbn adafruit:samd:adafruit_trellis_m4 arduino_trellis

# Flash to device (device must be in bootloader mode)
arduino-cli upload --fqbn adafruit:samd:adafruit_trellis_m4 -p /dev/cu.usbmodem11101 arduino_trellis
```

#### 4.2 Hardware Testing
1. **Shift Key Detection**: Verify bottom-left key activates shift mode
2. **Start/Stop Control**: Test shift + bottom-right key toggles playback  
3. **Visual Feedback**: Confirm LED feedback for shift mode
4. **Normal Operation**: Ensure no interference with step sequencer
5. **Real-time Performance**: Verify responsive button handling

### Phase 5: Keep CMake for Simulation
**Maintain CMake build for development**:
- Host simulation with shift controls for testing
- Unit tests for shift control logic
- Cross-platform development without hardware

## File Structure
```
arduino_trellis/
├── arduino_trellis.ino       # Main Arduino sketch
├── ShiftControls.h           # Shift control logic (header-only)
└── build/                    # Arduino CLI build artifacts

src/core/                     # CMake simulation only
├── ShiftControls.cpp         # For unit tests and simulation
└── IShiftControls.h          # Interface for testing

src/simulation/
└── main.cpp                  # Simulation with shift controls
```

## Success Criteria
- [x] Arduino CLI project structure established ✅
- [x] **Reliable flash/upload process using Arduino CLI** ✅
- [x] **Makefile fixed to use Arduino CLI for hardware deployment** ✅
- [x] **CMake simulation environment preserved for development** ✅
- [x] **Shift-key controls implemented in Arduino sketch** ✅
- [x] **Start/stop functionality added (togglePlayback)** ✅
- [x] **Visual feedback implemented (shift highlights)** ✅
- [x] **Hardware testing**: Verify shift key detection works on device ✅
- [x] **Hardware testing**: Verify start/stop control with shift modifier functional ✅
- [x] **Hardware testing**: Verify no interference with normal step sequencer ✅

## Technical Advantages of Arduino CLI Approach

### Hardware Compatibility
- **UF2 Bootloader**: Fully compatible with NeoTrellis M4
- **Library Ecosystem**: Access to official Adafruit libraries
- **Proven Reliability**: Arduino framework handles hardware initialization
- **No Custom Firmware**: Uses established Arduino infrastructure

### Development Workflow  
- **Simple Build**: `arduino-cli compile` 
- **Reliable Flash**: `arduino-cli upload` with auto-reset
- **Cross-platform**: Works on macOS/Linux/Windows
- **Professional Tools**: Command-line automation capability

### Performance
- **Optimized**: Arduino compiler optimizations for SAMD51
- **Memory Efficient**: Proven memory layouts for this hardware
- **Real-time**: Stable timing for step sequencer applications
- **Battle-tested**: Used by thousands of NeoTrellis M4 projects

## Migration Benefits

### Preserved Architecture  
- **Core Logic**: Shift control algorithms remain unchanged
- **Testing**: Unit tests still valuable via CMake simulation
- **Interfaces**: Clean abstraction concepts maintained
- **Documentation**: All design decisions preserved

### Reduced Complexity
- **No Custom Linker Scripts**: Arduino handles memory layout
- **No Bootloader Fighting**: UF2 compatibility built-in  
- **No Register-level Code**: High-level Arduino APIs
- **No Debugging Infrastructure**: Serial debugging just works

## Current Status (2025-09-04)

### ✅ Infrastructure Complete
- **Hardware deployment resolved**: Arduino CLI working, device flashing reliably
- **Basic step sequencer running**: 4 tracks, 8 steps, working button/LED interaction
- **Build system fixed**: `make flash` uses Arduino CLI, documentation updated
- **Foundation solid**: Ready to implement shift-key functionality

### 🔄 Next: Shift-Key Implementation  
**Current Arduino sketch** (`arduino_trellis.ino`) contains:
- Working StepSequencer class
- Button callback system via `trellisCallback()`
- LED update system
- **Missing**: ShiftControls class and shift-key logic

**Implementation needed**:
1. Add ShiftControls class to handle shift-key state
2. Modify trellisCallback to detect shift+control combinations  
3. Add visual feedback for shift mode
4. Add start/stop functionality

## Implementation Timeline

### ✅ Completed (Day 1)
- [x] Identify Arduino CLI as solution
- [x] **Fix Makefile to use Arduino CLI for hardware deployment** ✅
- [x] **Verify reliable flash process works** ✅
- [x] **Arduino sketch confirmed working** (basic step sequencer functional) ✅

### ✅ Implementation Complete (Day 1 continued)
- [x] **Add ShiftControls class to Arduino sketch** ✅
- [x] **Modify input handling for shift-key detection** ✅
- [x] **Port shift control logic to Arduino-compatible format** ✅
- [x] **Add visual feedback for shift mode** ✅
- [x] **Add start/stop functionality** ✅

### ✅ Hardware Testing Complete
- [x] **Build and flash updated Arduino sketch** ✅
- [x] **Test shift-key controls on hardware** ✅
- [x] **Verified shift-key detection works** ✅ (bottom-left button activates shift mode)
- [x] **Verified start/stop control functional** ✅ (shift + bottom-right toggles playback)
- [x] **No interference with normal step sequencer** ✅

### ✅ Short-term Goals Complete
- [x] Implement shift key detection in Arduino sketch ✅
- [x] Add start/stop control with visual feedback ✅  
- [x] Test complete functionality on hardware ✅
- [x] Verify no regression in step sequencer operation ✅

### Maintenance
- [ ] Keep CMake simulation for development/testing
- [ ] Maintain unit test coverage via simulation build
- [ ] Document Arduino CLI workflow for future development

## Conclusion

The Arduino CLI approach leverages the existing working infrastructure while providing a reliable path to implement shift-key controls on the NeoTrellis M4 hardware. This solution prioritizes:

1. **Hardware Compatibility**: Proven Arduino ecosystem
2. **Development Velocity**: Fast iteration with reliable builds  
3. **Architecture Preservation**: Core logic and testing maintained
4. **Future Maintainability**: Standard Arduino development patterns

The CMake implementation provided valuable architecture and testing infrastructure that remains useful for simulation and development, while Arduino CLI enables reliable hardware deployment.