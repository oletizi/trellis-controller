/*
 * NeoTrellis M4 I2C Peripheral Firmware
 * 
 * Configures the NeoTrellis M4 as an I2C peripheral device that can be
 * controlled by a Teensy 4.1 or other I2C host controller.
 * 
 * Features:
 * - 4x8 button matrix scanning with debouncing
 * - 32 RGB NeoPixel control
 * - I2C command interface at address 0x60
 */

#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_Keypad.h>

// I2C Configuration
#define I2C_ADDRESS 0x60

// NeoPixel Configuration
#define NEOPIXEL_PIN 10
#define NUM_PIXELS 32
Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Keypad Configuration
#define ROWS 4
#define COLS 8
char keys[ROWS][COLS] = {
  {0,  1,  2,  3,  4,  5,  6,  7},
  {8,  9,  10, 11, 12, 13, 14, 15},
  {16, 17, 18, 19, 20, 21, 22, 23},
  {24, 25, 26, 27, 28, 29, 30, 31}
};

// Pin mappings for NeoTrellis M4
byte rowPins[ROWS] = {14, 15, 16, 17}; // Connect to row pins
byte colPins[COLS] = {2, 3, 4, 5, 6, 7, 8, 9}; // Connect to column pins

Adafruit_Keypad keypad = Adafruit_Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// I2C Command Protocol
enum Commands {
  CMD_SET_PIXEL = 0x01,      // Set single LED: [cmd, index, r, g, b]
  CMD_SET_ALL_PIXELS = 0x02, // Set all LEDs: [cmd, r0, g0, b0, ...]
  CMD_SET_BRIGHTNESS = 0x03, // Set brightness: [cmd, brightness]
  CMD_SHOW_PIXELS = 0x04,    // Update LED display: [cmd]
  CMD_GET_BUTTONS = 0x10,    // Get button states: returns 4 bytes
  CMD_GET_BUTTON = 0x11,     // Get single button: [cmd, index] returns state
  CMD_GET_EVENTS = 0x12,     // Get button events since last read
  CMD_PING = 0x20,           // Ping test: [cmd] returns 0xAA
  CMD_GET_VERSION = 0x21     // Get firmware version
};

// Button state tracking
uint32_t currentButtonState = 0;  // Bit field for 32 buttons
uint32_t previousButtonState = 0;
uint32_t buttonPressEvents = 0;   // Buttons pressed since last read
uint32_t buttonReleaseEvents = 0; // Buttons released since last read

// I2C communication buffers
#define I2C_BUFFER_SIZE 64
uint8_t i2cCommand[I2C_BUFFER_SIZE];
uint8_t i2cResponse[I2C_BUFFER_SIZE];
uint8_t i2cCommandLength = 0;
uint8_t i2cResponseLength = 0;

// Timing
unsigned long lastKeypadUpdate = 0;
const unsigned long KEYPAD_UPDATE_INTERVAL = 10; // 10ms = 100Hz scan rate

void setup() {
  Serial.begin(115200);
  
  // Initialize NeoPixels
  pixels.begin();
  pixels.setBrightness(50); // Default brightness
  pixels.clear();
  pixels.show();
  
  // Initialize Keypad
  keypad.begin();
  
  // Initialize I2C as peripheral
  Wire.begin(I2C_ADDRESS);
  Wire.onReceive(i2cReceiveEvent);
  Wire.onRequest(i2cRequestEvent);
  
  // Visual startup indication
  startupAnimation();
  
  Serial.println("NeoTrellis M4 I2C Peripheral Ready");
  Serial.print("I2C Address: 0x");
  Serial.println(I2C_ADDRESS, HEX);
}

void loop() {
  // Update keypad at regular intervals
  if (millis() - lastKeypadUpdate >= KEYPAD_UPDATE_INTERVAL) {
    lastKeypadUpdate = millis();
    updateKeypad();
  }
  
  // Process any pending I2C commands
  processI2CCommand();
}

void updateKeypad() {
  keypad.tick();
  
  // Store previous state
  previousButtonState = currentButtonState;
  
  // Check each key
  while (keypad.available()) {
    keypadEvent e = keypad.read();
    uint8_t key = (uint8_t)e.bit.KEY;
    
    if (key < 32) {
      if (e.bit.EVENT == KEY_JUST_PRESSED) {
        currentButtonState |= (1UL << key);
        buttonPressEvents |= (1UL << key);
      } else if (e.bit.EVENT == KEY_JUST_RELEASED) {
        currentButtonState &= ~(1UL << key);
        buttonReleaseEvents |= (1UL << key);
      }
    }
  }
}

void i2cReceiveEvent(int numBytes) {
  i2cCommandLength = 0;
  
  while (Wire.available() && i2cCommandLength < I2C_BUFFER_SIZE) {
    i2cCommand[i2cCommandLength++] = Wire.read();
  }
  
  // Clear any remaining bytes to avoid buffer issues
  while (Wire.available()) {
    Wire.read();
  }
}

void i2cRequestEvent() {
  // Send prepared response
  if (i2cResponseLength > 0) {
    Wire.write(i2cResponse, i2cResponseLength);
    i2cResponseLength = 0;
  } else {
    // Default response if nothing prepared
    Wire.write(0xFF);
  }
}

void processI2CCommand() {
  if (i2cCommandLength == 0) return;
  
  uint8_t cmd = i2cCommand[0];
  i2cResponseLength = 0;
  
  switch (cmd) {
    case CMD_SET_PIXEL:
      if (i2cCommandLength >= 5) {
        uint8_t index = i2cCommand[1];
        uint8_t r = i2cCommand[2];
        uint8_t g = i2cCommand[3];
        uint8_t b = i2cCommand[4];
        if (index < NUM_PIXELS) {
          pixels.setPixelColor(index, pixels.Color(r, g, b));
        }
      }
      break;
      
    case CMD_SET_ALL_PIXELS:
      if (i2cCommandLength >= 97) { // 1 + (32 * 3)
        for (uint8_t i = 0; i < NUM_PIXELS; i++) {
          uint8_t r = i2cCommand[1 + (i * 3)];
          uint8_t g = i2cCommand[2 + (i * 3)];
          uint8_t b = i2cCommand[3 + (i * 3)];
          pixels.setPixelColor(i, pixels.Color(r, g, b));
        }
      }
      break;
      
    case CMD_SET_BRIGHTNESS:
      if (i2cCommandLength >= 2) {
        pixels.setBrightness(i2cCommand[1]);
      }
      break;
      
    case CMD_SHOW_PIXELS:
      pixels.show();
      break;
      
    case CMD_GET_BUTTONS:
      // Return 4 bytes of button state (32 bits)
      i2cResponse[0] = (currentButtonState >> 0) & 0xFF;
      i2cResponse[1] = (currentButtonState >> 8) & 0xFF;
      i2cResponse[2] = (currentButtonState >> 16) & 0xFF;
      i2cResponse[3] = (currentButtonState >> 24) & 0xFF;
      i2cResponseLength = 4;
      break;
      
    case CMD_GET_BUTTON:
      if (i2cCommandLength >= 2) {
        uint8_t buttonIndex = i2cCommand[1];
        if (buttonIndex < 32) {
          i2cResponse[0] = (currentButtonState & (1UL << buttonIndex)) ? 1 : 0;
          i2cResponseLength = 1;
        }
      }
      break;
      
    case CMD_GET_EVENTS:
      // Return press and release events (8 bytes total)
      i2cResponse[0] = (buttonPressEvents >> 0) & 0xFF;
      i2cResponse[1] = (buttonPressEvents >> 8) & 0xFF;
      i2cResponse[2] = (buttonPressEvents >> 16) & 0xFF;
      i2cResponse[3] = (buttonPressEvents >> 24) & 0xFF;
      i2cResponse[4] = (buttonReleaseEvents >> 0) & 0xFF;
      i2cResponse[5] = (buttonReleaseEvents >> 8) & 0xFF;
      i2cResponse[6] = (buttonReleaseEvents >> 16) & 0xFF;
      i2cResponse[7] = (buttonReleaseEvents >> 24) & 0xFF;
      i2cResponseLength = 8;
      // Clear events after reading
      buttonPressEvents = 0;
      buttonReleaseEvents = 0;
      break;
      
    case CMD_PING:
      i2cResponse[0] = 0xAA; // Ping response
      i2cResponseLength = 1;
      break;
      
    case CMD_GET_VERSION:
      i2cResponse[0] = 1; // Major version
      i2cResponse[1] = 0; // Minor version
      i2cResponseLength = 2;
      break;
  }
  
  // Clear command after processing
  i2cCommandLength = 0;
}

void startupAnimation() {
  // Simple rainbow sweep animation
  for (int i = 0; i < NUM_PIXELS; i++) {
    pixels.setPixelColor(i, Wheel((i * 256 / NUM_PIXELS) & 255));
    pixels.show();
    delay(20);
  }
  delay(500);
  pixels.clear();
  pixels.show();
}

// Color wheel function for rainbow effects
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}