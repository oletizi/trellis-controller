#include "NeoTrellisDisplay.h"
#include "NeoTrellisHardware.h"

// Global NeoTrellis instance (defined here, declared in header)
NeoTrellis g_neoTrellis;

NeoTrellisDisplay::NeoTrellisDisplay() : initialized_(false) {
    clear();
}

void NeoTrellisDisplay::init() {
    if (initialized_) return;
    
    // Initialize NeoTrellis hardware
    if (!g_neoTrellis.begin()) {
        // Hardware initialization failed - continue anyway for simulation
    }
    
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
    for (uint8_t row = 0; row < ROWS; row++) {
        for (uint8_t col = 0; col < COLS; col++) {
            LED& led = leds_[row][col];
            if (led.dirty) {
                // Convert row,col to linear index for NeoTrellis
                uint8_t index = row * COLS + col;
                // Pack RGB into 32-bit color (0x00RRGGBB format)
                uint32_t color = ((uint32_t)led.r << 16) | ((uint32_t)led.g << 8) | led.b;
                g_neoTrellis.pixels.setPixelColor(index, color);
                led.dirty = false;
            }
        }
    }
    g_neoTrellis.pixels.show();
}