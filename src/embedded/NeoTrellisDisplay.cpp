#include "NeoTrellisDisplay.h"
#include "SeesawI2C.h"
#include "SeesawProtocol.h"

// Seesaw I2C interface for NeoTrellis (shared between display and input)
SeesawI2C g_seesaw;

NeoTrellisDisplay::NeoTrellisDisplay() : initialized_(false) {
    clear();
}

void NeoTrellisDisplay::init() {
    if (initialized_) return;
    
    // Initialize Seesaw I2C communication
    if (!g_seesaw.begin()) {
        // I2C initialization failed - continue anyway for testing
    }
    
    // Configure NeoPixel settings for NeoTrellis M4
    g_seesaw.setNeoPixelPin(Seesaw::NEOTRELLIS_NEOPIX_PIN);
    g_seesaw.setNeoPixelLength(ROWS * COLS); // 4x8 = 32 LEDs
    
    initialized_ = true;
    clear();
    refresh();
}

void NeoTrellisDisplay::shutdown() {
    if (!initialized_) return;
    
    clear();
    refresh();
    initialized_ = false;
}

void NeoTrellisDisplay::setLED(uint8_t row, uint8_t col, uint8_t r, uint8_t g, uint8_t b) {
    if (row >= ROWS || col >= COLS) return;
    
    LED& led = leds_[row][col];
    if (led.r != r || led.g != g || led.b != b) {
        led.r = r;
        led.g = g;
        led.b = b;
        led.dirty = true;
    }
}

void NeoTrellisDisplay::clear() {
    for (uint8_t row = 0; row < ROWS; row++) {
        for (uint8_t col = 0; col < COLS; col++) {
            leds_[row][col] = {0, 0, 0, true};
        }
    }
}

void NeoTrellisDisplay::refresh() {
    if (!initialized_) return;
    
    updateHardware();
}

void NeoTrellisDisplay::updateHardware() {
    // Build RGB data for all dirty LEDs
    uint8_t rgbData[ROWS * COLS * 3]; // 3 bytes per LED (R,G,B)
    bool anyDirty = false;
    
    for (uint8_t row = 0; row < ROWS; row++) {
        for (uint8_t col = 0; col < COLS; col++) {
            LED& led = leds_[row][col];
            if (led.dirty) {
                anyDirty = true;
                uint8_t index = row * COLS + col;
                
                // NeoPixel format: GRB order (not RGB)
                rgbData[index * 3 + 0] = led.g; // Green
                rgbData[index * 3 + 1] = led.r; // Red  
                rgbData[index * 3 + 2] = led.b; // Blue
                
                led.dirty = false;
            }
        }
    }
    
    if (anyDirty) {
        // Send RGB data to Seesaw NeoPixel buffer
        g_seesaw.setNeoPixelBuffer(0, rgbData, sizeof(rgbData));
        
        // Trigger LED update
        g_seesaw.showNeoPixels();
    }
}