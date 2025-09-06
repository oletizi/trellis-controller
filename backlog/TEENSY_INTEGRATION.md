# Teensy 4.1 + NeoTrellis M4 Integration

## Overview
This implementation transforms the NeoTrellis M4 from a standalone controller into an I2C peripheral device controlled by a Teensy 4.1. This architecture enables advanced audio sampling capabilities while maintaining the NeoTrellis' button and LED functionality.

## Architecture

```
[Teensy 4.1 Controller]
    |
    | I2C Bus (400kHz)
    |
[NeoTrellis M4 Peripheral]
    |
    +-- 32 RGB LEDs (NeoPixels)
    +-- 4x8 Button Matrix
```

## Hardware Setup

### Required Components
- Teensy 4.1
- NeoTrellis M4
- 2x 4.7kΩ resistors (I2C pullups)
- Jumper wires
- USB cables for programming both devices

### Wiring Connections
| Teensy 4.1 | NeoTrellis M4 | Description |
|------------|---------------|-------------|
| Pin 18     | SDA          | I2C Data    |
| Pin 19     | SCL          | I2C Clock   |
| 3.3V       | VCC          | Power       |
| GND        | GND          | Ground      |

**Note:** Add 4.7kΩ pullup resistors from SDA and SCL to 3.3V

## Software Components

### 1. NeoTrellis M4 Peripheral Firmware
Location: `neotrellis_m4_peripheral/`

The M4 runs custom firmware that:
- Configures the device as an I2C peripheral at address 0x60
- Scans the 4x8 button matrix at 100Hz
- Controls 32 RGB NeoPixels
- Responds to I2C commands from the Teensy

### 2. Teensy Controller Library
Location: `teensy_controller/`

Files:
- `NeoTrellisM4.h/cpp` - Driver library
- `teensy_controller.ino` - Example/test sketch

The Teensy library provides:
- High-level API for LED control
- Button state polling and event detection
- Mode switching capabilities
- Foundation for audio sampler integration

## I2C Protocol

### Commands

| Command | Value | Description | Data Format |
|---------|-------|-------------|-------------|
| SET_PIXEL | 0x01 | Set single LED | [cmd, index, r, g, b] |
| SET_ALL_PIXELS | 0x02 | Set all LEDs | [cmd, rgb_data[96]] |
| SET_BRIGHTNESS | 0x03 | Set global brightness | [cmd, brightness] |
| SHOW_PIXELS | 0x04 | Update LED display | [cmd] |
| GET_BUTTONS | 0x10 | Get button states | Returns 4 bytes |
| GET_BUTTON | 0x11 | Get single button | [cmd, index] → state |
| GET_EVENTS | 0x12 | Get button events | Returns 8 bytes |
| PING | 0x20 | Test connection | [cmd] → 0xAA |
| GET_VERSION | 0x21 | Get firmware version | [cmd] → [major, minor] |

## Programming Instructions

### 1. Flash the NeoTrellis M4
```bash
cd neotrellis_m4_peripheral
arduino-cli compile --fqbn adafruit:samd:adafruit_trellis_m4 .
arduino-cli upload --fqbn adafruit:samd:adafruit_trellis_m4 -p /dev/cu.usbmodem* .
```

### 2. Flash the Teensy 4.1
```bash
cd teensy_controller
# Use Arduino IDE or Teensyduino to compile and upload
# Select Board: Teensy 4.1
# Select Port: Your Teensy port
# Click Upload
```

## Testing

1. Connect hardware as described above
2. Flash both devices with their respective firmware
3. Open Serial Monitor for Teensy (115200 baud)
4. Press buttons to test - they should light up
5. Press button 31 (bottom-right) to switch between modes:
   - Button Test Mode: Buttons light when pressed
   - Sequencer Mode: Running light pattern
   - Rainbow Mode: Animated rainbow effect

## API Usage Example

```cpp
#include "NeoTrellisM4.h"

NeoTrellisM4 trellis(Wire);

void setup() {
    Wire.begin();
    trellis.begin(0x60);
    
    // Set an LED
    trellis.setPixel(0, 255, 0, 0); // Red
    trellis.show();
    
    // Set callback for button events
    trellis.setButtonCallback([](uint8_t key, uint8_t event) {
        if (event == NeoTrellisM4::BUTTON_PRESSED) {
            // Handle button press
        }
    });
}

void loop() {
    trellis.update(); // Poll buttons
}
```

## Next Steps

With this foundation in place, the next phases include:
1. Implementing audio output on Teensy 4.1
2. Adding sample playback engine
3. Creating SFZ/DecentSampler format support
4. Implementing USB file transfer
5. Building the complete sampler application

## Troubleshooting

### No Communication
- Verify wiring connections
- Check I2C pullup resistors are installed
- Ensure both devices are powered
- Verify correct I2C address (0x60)

### Buttons Not Responding
- Check M4 firmware is running (startup animation should play)
- Verify Teensy is calling `trellis.update()` regularly
- Check serial output for error messages

### LEDs Not Working
- Verify `trellis.show()` is called after setting pixels
- Check brightness setting (default is 30/255)
- Ensure power supply is adequate for all LEDs