/*
 * Teensy 4.1 Controller for NeoTrellis M4
 * 
 * Test sketch demonstrating communication with NeoTrellis M4
 * configured as an I2C peripheral device.
 * 
 * Wiring:
 * - Teensy Pin 18 (SDA) -> NeoTrellis M4 SDA
 * - Teensy Pin 19 (SCL) -> NeoTrellis M4 SCL
 * - Teensy 3.3V -> NeoTrellis M4 VCC
 * - Teensy GND -> NeoTrellis M4 GND
 * - Add 4.7kΩ pullup resistors from SDA and SCL to 3.3V
 */

#include "NeoTrellisM4.h"

// Create NeoTrellis M4 instance
NeoTrellisM4 trellis(Wire);

// Track button states for demo
bool buttonStates[32] = {false};

// Color palette for demo
uint32_t colors[] = {
    NeoTrellisM4::Color(255, 0, 0),    // Red
    NeoTrellisM4::Color(0, 255, 0),    // Green  
    NeoTrellisM4::Color(0, 0, 255),    // Blue
    NeoTrellisM4::Color(255, 255, 0),  // Yellow
    NeoTrellisM4::Color(255, 0, 255),  // Magenta
    NeoTrellisM4::Color(0, 255, 255),  // Cyan
    NeoTrellisM4::Color(255, 128, 0),  // Orange
    NeoTrellisM4::Color(128, 0, 255)   // Purple
};

// Timing
unsigned long lastUpdate = 0;
unsigned long lastHeartbeat = 0;

// Modes
enum Mode {
    MODE_BUTTON_TEST,
    MODE_SEQUENCER,
    MODE_RAINBOW
};

Mode currentMode = MODE_BUTTON_TEST;

// Sequencer variables
uint8_t sequencerStep = 0;
unsigned long lastStepTime = 0;
uint16_t stepInterval = 125; // 120 BPM

// Button callback
void onButtonEvent(uint8_t key, uint8_t event) {
    if (event == NeoTrellisM4::BUTTON_PRESSED) {
        Serial.print("Button ");
        Serial.print(key);
        Serial.println(" pressed");
        
        // In button test mode, light up pressed buttons
        if (currentMode == MODE_BUTTON_TEST) {
            uint32_t color = colors[key % 8];
            trellis.setPixel(key, color);
            trellis.show();
        }
        
        // Mode switching with button 31 (bottom-right)
        if (key == 31) {
            currentMode = (Mode)((currentMode + 1) % 3);
            Serial.print("Switched to mode: ");
            Serial.println(currentMode);
            trellis.clear();
            trellis.show();
        }
        
    } else if (event == NeoTrellisM4::BUTTON_RELEASED) {
        Serial.print("Button ");
        Serial.print(key);
        Serial.println(" released");
        
        // In button test mode, turn off released buttons
        if (currentMode == MODE_BUTTON_TEST) {
            trellis.setPixel(key, 0);
            trellis.show();
        }
    }
}

void setup() {
    Serial.begin(115200);
    
    // Wait for serial connection (optional, remove for standalone operation)
    while (!Serial && millis() < 3000);
    
    Serial.println("Teensy 4.1 NeoTrellis M4 Controller");
    Serial.println("Initializing...");
    
    // Initialize I2C
    Wire.begin();
    Wire.setClock(400000); // 400kHz I2C
    
    // Initialize NeoTrellis M4
    if (!trellis.begin(0x60)) {
        Serial.println("ERROR: Could not communicate with NeoTrellis M4!");
        Serial.println("Check wiring and ensure M4 is running peripheral firmware");
        while (1) {
            delay(1000);
            Serial.println("Retrying...");
            if (trellis.begin(0x60)) {
                Serial.println("Connected!");
                break;
            }
        }
    }
    
    Serial.println("NeoTrellis M4 connected!");
    
    // Get firmware version
    uint8_t major, minor;
    trellis.getVersion(major, minor);
    Serial.print("M4 Firmware version: ");
    Serial.print(major);
    Serial.print(".");
    Serial.println(minor);
    
    // Set button callback
    trellis.setButtonCallback(onButtonEvent);
    
    // Set brightness
    trellis.setBrightness(30);
    
    // Startup animation
    startupAnimation();
    
    Serial.println("Ready! Press buttons to test.");
    Serial.println("Button 31 (bottom-right) switches modes");
}

void loop() {
    // Update button states
    if (millis() - lastUpdate >= 10) {
        lastUpdate = millis();
        trellis.update();
    }
    
    // Mode-specific behavior
    switch (currentMode) {
        case MODE_BUTTON_TEST:
            // Buttons light up when pressed (handled in callback)
            break;
            
        case MODE_SEQUENCER:
            runSequencer();
            break;
            
        case MODE_RAINBOW:
            runRainbow();
            break;
    }
    
    // Heartbeat LED (optional visual indicator)
    if (millis() - lastHeartbeat >= 1000) {
        lastHeartbeat = millis();
        // Could pulse an LED here if available
    }
}

void runSequencer() {
    if (millis() - lastStepTime >= stepInterval) {
        lastStepTime = millis();
        
        // Clear previous step
        for (int i = 0; i < 4; i++) {
            trellis.setPixel(i * 8 + (sequencerStep % 8), 0);
        }
        
        // Advance step
        sequencerStep = (sequencerStep + 1) % 8;
        
        // Light current step column
        for (int i = 0; i < 4; i++) {
            uint32_t color = colors[i * 2];
            trellis.setPixel(i * 8 + sequencerStep, color);
        }
        
        trellis.show();
    }
}

void runRainbow() {
    static uint8_t offset = 0;
    static unsigned long lastRainbowUpdate = 0;
    
    if (millis() - lastRainbowUpdate >= 50) {
        lastRainbowUpdate = millis();
        
        for (int i = 0; i < 32; i++) {
            uint32_t color = NeoTrellisM4::Wheel(((i * 256 / 32) + offset) & 255);
            trellis.setPixel(i, color);
        }
        
        trellis.show();
        offset = (offset + 1) & 255;
    }
}

void startupAnimation() {
    Serial.println("Running startup animation...");
    
    // Light up each button in sequence
    for (int i = 0; i < 32; i++) {
        trellis.setPixel(i, NeoTrellisM4::Wheel((i * 256 / 32) & 255));
        trellis.show();
        delay(30);
    }
    
    delay(500);
    
    // Fade out
    for (int brightness = 30; brightness >= 0; brightness -= 5) {
        trellis.setBrightness(brightness);
        trellis.show();
        delay(50);
    }
    
    // Clear and restore brightness
    trellis.clear();
    trellis.setBrightness(30);
    trellis.show();
}