#ifndef SEESAW_PROTOCOL_H
#define SEESAW_PROTOCOL_H

#include <stdint.h>

// Seesaw Protocol Constants (extracted from Adafruit_Seesaw library)
// Reference: https://github.com/adafruit/Adafruit_Seesaw
// Commit: 985b41efae3d9a8cba12a7b4d9ff0d226f9e0759

namespace Seesaw {

// Default I2C addresses
constexpr uint8_t DEFAULT_ADDR = 0x49;
constexpr uint8_t NEOTRELLIS_ADDR = 0x2E;

// Module Base Addresses
enum ModuleBase : uint8_t {
    STATUS = 0x00,
    GPIO = 0x01,
    SERCOM0 = 0x02,
    TIMER = 0x08,
    ADC = 0x09,
    DAC = 0x0A,
    INTERRUPT = 0x0B,
    DAP = 0x0C,
    EEPROM = 0x0D,
    NEOPIXEL = 0x0E,
    TOUCH = 0x0F,
    KEYPAD = 0x10,
    ENCODER = 0x11,
    SPECTRUM = 0x12
};

// Status Module Registers
enum StatusRegister : uint8_t {
    STATUS_HW_ID = 0x01,
    STATUS_VERSION = 0x02,
    STATUS_OPTIONS = 0x03,
    STATUS_TEMP = 0x04,
    STATUS_SWRST = 0x7F
};

// NeoPixel Module Registers
enum NeoPixelRegister : uint8_t {
    NEOPIXEL_STATUS = 0x00,
    NEOPIXEL_PIN = 0x01,
    NEOPIXEL_SPEED = 0x02,
    NEOPIXEL_BUF_LENGTH = 0x03,
    NEOPIXEL_BUF = 0x04,
    NEOPIXEL_SHOW = 0x05
};

// Keypad Module Registers  
enum KeypadRegister : uint8_t {
    KEYPAD_STATUS = 0x00,
    KEYPAD_EVENT = 0x01,
    KEYPAD_INTENSET = 0x02,
    KEYPAD_INTENCLR = 0x03,
    KEYPAD_COUNT = 0x04,
    KEYPAD_FIFO = 0x10
};

// Keypad Edge Definitions
enum KeypadEdge : uint8_t {
    KEYPAD_EDGE_HIGH = 0,
    KEYPAD_EDGE_LOW = 1,
    KEYPAD_EDGE_FALLING = 2,
    KEYPAD_EDGE_RISING = 3
};

// NeoTrellis specific constants
constexpr uint8_t NEOTRELLIS_NEOPIX_PIN = 3;
constexpr uint8_t NEOTRELLIS_NUM_ROWS = 4;
constexpr uint8_t NEOTRELLIS_NUM_COLS = 4;
constexpr uint8_t NEOTRELLIS_NUM_KEYS = 16;

// Key event structure (matches Adafruit library)
union KeyEvent {
    struct {
        uint8_t EDGE : 2;
        uint8_t NUM : 6;
    } bit;
    uint8_t reg;
};

// Protocol helper functions
inline uint8_t makeAddress(ModuleBase module, uint8_t function) {
    return (static_cast<uint8_t>(module) << 8) | function;
}

} // namespace Seesaw

#endif