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