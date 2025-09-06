# NeoTrellis M4 to Teensy 4.1 Integration Work Plan

## Overview
Convert the NeoTrellis M4 from a standalone device to an I2C peripheral controlled by a Teensy 4.1 for audio sampling applications.

## Phase 1: Hardware Analysis & Preparation

### 1.1 NeoTrellis M4 Hardware Study
- [ ] Identify GPIO pins used for keypad matrix (4 rows + 4 columns)
- [ ] Locate NeoPixel data pin connection
- [ ] Find I2C pins (SDA, SCL) on the M4
- [ ] Document power supply requirements (3.3V/5V)
- [ ] Take photos of board layout for reference

### 1.2 Teensy 4.1 Pin Planning
- [ ] Assign I2C pins (Wire: SDA=18, SCL=19 or Wire1: SDA=17, SCL=16)
- [ ] Reserve pins for audio shield if using
- [ ] Plan power distribution (3.3V from Teensy to M4)

## Phase 2: M4 Firmware Development

### 2.1 Basic I2C Slave Setup
- [ ] Create new Arduino project for M4
- [ ] Configure M4 as I2C slave (address 0x60)
- [ ] Implement basic I2C receive/transmit handlers
- [ ] Test I2C communication with simple ping/pong

### 2.2 Keypad Matrix Scanning
- [ ] Set up 4x4 matrix scanning routine
- [ ] Implement debouncing (20ms typical)
- [ ] Create button state tracking (pressed, released, held)
- [ ] Map physical buttons to logical indices (0-15)

### 2.3 NeoPixel Control
- [ ] Initialize NeoPixel strip (16 pixels)
- [ ] Create LED buffer for RGB values
- [ ] Implement brightness control
- [ ] Add smooth color transitions (optional)

### 2.4 I2C Protocol Design
```
Command Format:
Byte 0: Command Type
  0x01 = Set LED (Byte 1: LED index, Bytes 2-4: R,G,B)
  0x02 = Set All LEDs (Bytes 1-48: RGB data for all 16 LEDs)
  0x03 = Set Brightness (Byte 1: 0-255)
  0x10 = Get Button States (Returns 2 bytes: current + changed)
  0x11 = Get Single Button (Byte 1: button index, Returns: state)
```

- [ ] Implement command parser
- [ ] Add error handling for invalid commands
- [ ] Test each command type individually

## Phase 3: Teensy 4.1 Driver Development

### 3.1 Basic Communication Library
- [ ] Create NeoTrellisM4 class for Teensy
- [ ] Implement I2C communication methods
- [ ] Add connection testing/validation
- [ ] Handle I2C timeouts and errors

### 3.2 High-Level Interface
```cpp
class NeoTrellisM4 {
public:
  void begin(uint8_t addr = 0x60);
  void setPixel(uint8_t key, uint32_t color);
  void setPixelColor(uint8_t key, uint8_t r, uint8_t g, uint8_t b);
  void setBrightness(uint8_t brightness);
  void show();
  bool isPressed(uint8_t key);
  bool wasPressed(uint8_t key);
  bool wasReleased(uint8_t key);
  void update(); // Call frequently to poll buttons
};
```

- [ ] Implement all public methods
- [ ] Add button state caching and change detection
- [ ] Create example sketches for testing

### 3.3 Integration Testing
- [ ] Test LED control (individual pixels)
- [ ] Test button reading (press/release detection)
- [ ] Verify performance (update rate, latency)
- [ ] Stress test with rapid button presses

## Phase 4: Hardware Integration

### 4.1 Power Supply Design
- [ ] Connect Teensy 3.3V to M4 VCC
- [ ] Add common ground connection
- [ ] Consider adding decoupling capacitors (0.1μF near each IC)
- [ ] Test power draw under full LED load

### 4.2 I2C Wiring
- [ ] Connect SDA pins (Teensy pin 18 to M4 SDA)
- [ ] Connect SCL pins (Teensy pin 19 to M4 SCL)
- [ ] Add 4.7kΩ pullup resistors to 3.3V (SDA and SCL)
- [ ] Keep wire lengths short (<6 inches)

### 4.3 Physical Mounting
- [ ] Design 3D printed bracket to hold both boards
- [ ] Plan cable routing between boards
- [ ] Ensure access to programming ports on both devices
- [ ] Consider future expansion for additional modules

## Phase 5: Software Integration & Testing

### 5.1 Combined System Testing
- [ ] Test basic functionality (LEDs + buttons)
- [ ] Verify I2C stability under audio processing load
- [ ] Check for timing conflicts with audio interrupts
- [ ] Measure button response latency

### 5.2 Audio Integration
- [ ] Integrate with Teensy Audio Library
- [ ] Map buttons to sample triggers
- [ ] Implement LED feedback for playing samples
- [ ] Add visual indicators for recording, playback modes

### 5.3 Performance Optimization
- [ ] Optimize I2C update frequency (avoid audio dropouts)
- [ ] Implement efficient LED update batching
- [ ] Add button debouncing refinements
- [ ] Profile CPU usage on both processors

## Phase 6: Documentation & Expansion

### 6.1 Code Documentation
- [ ] Document I2C protocol specification
- [ ] Create wiring diagrams
- [ ] Write usage examples and tutorials
- [ ] Add troubleshooting guide

### 6.2 Future Expansion Planning
- [ ] Design for chaining multiple M4 units
- [ ] Plan encoder/display integration points
- [ ] Consider PCB design for cleaner connections
- [ ] Document lessons learned for next iteration

## Tools Needed
- Multimeter for continuity/voltage checking
- Oscilloscope or logic analyzer (optional, for I2C debugging)
- Breadboard and jumper wires
- 4.7kΩ resistors (2x for I2C pullups)
- 0.1μF capacitors (optional, for decoupling)

## Estimated Timeline
- Phase 1: 2-3 hours
- Phase 2: 8-12 hours
- Phase 3: 6-8 hours
- Phase 4: 2-4 hours
- Phase 5: 4-6 hours
- Phase 6: 2-3 hours

**Total: 24-36 hours** (spread over 2-3 weeks for testing/iteration)

## Success Criteria
- [ ] M4 responds reliably to I2C commands
- [ ] All 16 buttons register press/release events
- [ ] All 16 LEDs can be controlled individually
- [ ] No interference with Teensy audio processing
- [ ] System stable for extended use (>1 hour continuous operation)

## Architectural Review & Recommendations

### Critical Concerns Identified

#### 1. **Architecture Principle Violation**
The distributed approach breaks the project's core platform abstraction principles by splitting business logic across two embedded devices. This violates the established pattern where all business logic resides in `/src/core/` with clean hardware abstraction interfaces.

#### 2. **Distributed System Complexity**
- **State synchronization challenges** between M4 and Teensy
- **Error recovery complexity** for I2C communication failures
- **Debugging difficulty** with logic split across two processors
- **Testing complexity** requiring hardware-in-loop for validation

#### 3. **Performance Analysis**
- **I2C Overhead**: ~15% bandwidth utilization (acceptable)
- **Button-to-audio latency**: 4-8ms typical (acceptable for audio)
- **Risk**: I2C blocking operations could cause audio dropouts
- **Mitigation needed**: Separate I2C bus or DMA transfers

#### 4. **Protocol Weaknesses**
Current protocol design lacks:
- **Acknowledgment mechanism** for command validation
- **Error recovery protocol** for I2C failures
- **Flow control** for high-frequency operations
- **Version negotiation** for protocol evolution

#### 5. **Timeline Reality Check**
Based on distributed embedded system complexity:
- Phase 2 (M4 Firmware): 16-24 hours (vs. 8-12 estimated)
- Phase 3 (Teensy Driver): 12-16 hours (vs. 6-8 estimated)
- Phase 5 (Integration): 12-20 hours (vs. 4-6 estimated)
- **Realistic Total: 40-60 hours** (vs. 24-36 estimated)

### Recommended Alternative Architectures

#### Option 1: M4 as Primary Controller (Recommended)
Keep NeoTrellis M4 as the main controller, add Teensy as audio coprocessor:

```cpp
// Extend current architecture with audio interface
class IAudioOutput {
    virtual void playNote(uint8_t note, uint8_t velocity) = 0;
    virtual void stopNote(uint8_t note) = 0;
};

class TeensyAudioCoprocessor : public IAudioOutput {
    // Simple serial or I2C commands for audio only
};

// Maintain clean dependency injection
StepSequencer::Dependencies deps{
    .display = &neoTrellisDisplay,
    .input = &neoTrellisInput,
    .audio = &teensyAudio  // New addition
};
```

**Benefits**:
- Preserves platform abstraction principles
- Maintains single point of control
- Simpler debugging and testing
- Lower UI latency

#### Option 2: Hybrid Architecture
M4 handles all UI logic, Teensy receives audio commands only:

```cpp
// Unidirectional command flow (M4 → Teensy)
class AudioCommandProtocol {
    CMD_TRIGGER_SAMPLE = 0x01,  // [sample_id, velocity]
    CMD_STOP_SAMPLE = 0x02,     // [sample_id]
    CMD_SET_PARAMETER = 0x03,   // [param_id, value]
};
```

**Benefits**:
- Eliminates complex bidirectional I2C
- M4 maintains all application state
- Teensy focuses solely on audio processing
- No state synchronization issues

#### Option 3: Full Teensy Migration
If Teensy must be primary controller:

```cpp
// Clean migration with M4 as pure I/O peripheral
class NeoTrellisPeripheral : public IDisplay, public IInput {
    // Minimal firmware on M4
    // All business logic on Teensy
};
```

**Considerations**:
- Requires complete codebase migration to Teensy
- M4 becomes "dumb" I/O device
- Maintains architectural cleanliness
- Higher development effort

### Risk Mitigation Strategies

#### High-Priority Risks

1. **I2C Communication Failures**
   - Implement timeout and retry mechanisms
   - Add watchdog timers on both devices
   - Design bus reset and recovery procedures
   - Consider redundant communication channel

2. **Audio Performance Impact**
   - Use separate I2C bus (Wire1) for UI
   - Implement DMA transfers where possible
   - Profile interrupt latency carefully
   - Design graceful degradation for UI updates

3. **Development Complexity**
   - Start with minimal viable protocol
   - Use logic analyzer for I2C debugging (essential)
   - Implement comprehensive logging
   - Create simulation/mock environment first

### Enhanced Protocol Design

```cpp
// Improved protocol with error handling
struct CommandPacket {
    uint8_t cmd;
    uint8_t seq_num;  // For tracking
    uint8_t data_len;
    uint8_t data[29];
    uint8_t checksum;
};

struct ResponsePacket {
    uint8_t status;    // SUCCESS, ERROR, BUSY
    uint8_t seq_num;   // Echo from command
    uint8_t data[30];
};
```

### Missing Components to Address

1. **Power Management**
   - Power sequencing between devices
   - Brown-out detection
   - Safe shutdown procedures

2. **EMI/Noise Isolation**
   - Separate ground planes for audio/digital
   - Ferrite beads on I2C lines
   - Shielded cables for audio signals

3. **State Management**
   - Explicit state machine definition
   - State synchronization protocol
   - Recovery from desync conditions

4. **Development Infrastructure**
   - Hardware debugging setup (SWD/JTAG)
   - Automated testing framework
   - Performance profiling tools

### Recommended Path Forward

1. **Evaluate Necessity**: Determine if M4's 120MHz Cortex-M4F can handle audio directly
2. **Prototype Simple First**: Start with Option 1 (M4 primary + Teensy audio)
3. **Measure Performance**: Profile actual I2C overhead and audio latency
4. **Iterate Based on Data**: Only increase complexity if measurements require it
5. **Maintain Architecture**: Preserve clean abstractions wherever possible

### Conclusion

While technically feasible, the distributed architecture introduces significant complexity that may not be justified. The project's existing clean architecture with platform abstraction and dependency injection should be preserved. Consider simpler alternatives before committing to the distributed approach.

If proceeding with the distributed design:
- Budget 40-60 hours minimum
- Invest in proper I2C debugging tools
- Implement robust error handling from the start
- Design for testability with hardware mocks
- Document state machines and protocols thoroughly