// NeoTrellis M4 Step Sequencer - Thin Hardware Interface
// This file contains ONLY device-specific hardware translation
// All business logic is in src/core/

#include <Adafruit_NeoTrellisM4.h>
#include <MIDIUSB.h>

// Include core interfaces
#include "core/StepSequencer.h"
#include "core/IClock.h"
#include "core/IMidiOutput.h"
#include "core/IDisplay.h"
#include "core/IInput.h"

// Hardware objects
Adafruit_NeoTrellisM4 trellis;

// Hardware abstraction implementations
class ArduinoClock : public IClock {
public:
    uint32_t millis() const override {
        return ::millis();
    }
    
    void delay(uint32_t milliseconds) override {
        ::delay(milliseconds);
    }
    
    void reset() override {
        // No-op for Arduino millis()
    }
};

class ArduinoMidiOutput : public IMidiOutput {
public:
    ArduinoMidiOutput() : connected_(true) {}
    
    void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) override {
        if (!connected_) return;
        
        midiEventPacket_t noteOn = {0x09, static_cast<uint8_t>(0x90 | channel), note, velocity};
        MidiUSB.sendMIDI(noteOn);
        MidiUSB.flush();
        
        Serial.print("MIDI Note On: CH");
        Serial.print(channel + 1);
        Serial.print(" Note:");
        Serial.print(note);
        Serial.print(" Vel:");
        Serial.println(velocity);
    }
    
    void sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) override {
        if (!connected_) return;
        
        midiEventPacket_t noteOff = {0x08, static_cast<uint8_t>(0x80 | channel), note, velocity};
        MidiUSB.sendMIDI(noteOff);
        MidiUSB.flush();
    }
    
    void sendStart() override {
        if (!connected_) return;
        
        midiEventPacket_t startMsg = {0x0F, 0xFA, 0, 0};
        MidiUSB.sendMIDI(startMsg);
        MidiUSB.flush();
        Serial.println("MIDI: Start sent");
    }
    
    void sendStop() override {
        if (!connected_) return;
        
        midiEventPacket_t stopMsg = {0x0F, 0xFC, 0, 0};
        MidiUSB.sendMIDI(stopMsg);
        MidiUSB.flush();
        Serial.println("MIDI: Stop sent");
    }
    
    bool isConnected() const override {
        return connected_;
    }
    
    void flush() override {
        MidiUSB.flush();
    }

private:
    bool connected_;
};

class ArduinoDisplay : public IDisplay {
public:
    explicit ArduinoDisplay(Adafruit_NeoTrellisM4* trellis) : trellis_(trellis) {}
    
    void setPixel(uint8_t index, uint32_t color) override {
        if (index < 32 && trellis_) {
            // Convert from RGB to trellis color format
            uint8_t r = (color >> 16) & 0xFF;
            uint8_t g = (color >> 8) & 0xFF;
            uint8_t b = color & 0xFF;
            trellis_->setPixelColor(index, r, g, b);
        }
    }
    
    void show() override {
        if (trellis_) {
            trellis_->show();
        }
    }
    
    void setBrightness(uint8_t brightness) override {
        if (trellis_) {
            trellis_->setBrightness(brightness);
        }
    }
    
    void clear() override {
        if (trellis_) {
            trellis_->fill(0);
        }
    }

private:
    Adafruit_NeoTrellisM4* trellis_;
};

class ArduinoInput : public IInput {
public:
    explicit ArduinoInput(Adafruit_NeoTrellisM4* trellis) : trellis_(trellis) {}
    
    uint32_t getButtonState() override {
        // This would need to be implemented based on how NeoTrellis M4 scans
        // For now, return 0 - actual implementation would scan the matrix
        return 0;
    }
    
    bool isButtonPressed(uint8_t button) override {
        // Individual button state - would need hardware implementation
        return false;
    }
    
    void update() override {
        // Update button scanning state
    }

private:
    Adafruit_NeoTrellisM4* trellis_;
};

// Hardware abstraction instances
ArduinoClock systemClock;
ArduinoMidiOutput midiOutput;
ArduinoDisplay display(&trellis);
ArduinoInput input(&trellis);

// Core sequencer instance
StepSequencer* sequencer = nullptr;

// Button state tracking for parameter locks
uint32_t lastButtonStates = 0;
uint32_t currentButtonStates = 0;

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000); // Wait for serial or timeout
    
    Serial.println("NeoTrellis M4 Step Sequencer with Parameter Locks");
    
    // Initialize hardware
    if (!trellis.begin()) {
        Serial.println("Error: Failed to initialize NeoTrellis M4!");
        while (1) delay(100);
    }
    
    // Set up dependencies
    StepSequencer::Dependencies deps;
    deps.clock = &systemClock;
    deps.midiOutput = &midiOutput;
    deps.display = &display;
    deps.input = &input;
    
    // Create core sequencer
    sequencer = new StepSequencer(deps);
    sequencer->init(120, 8); // 120 BPM, 8 steps
    
    // Set up NeoTrellis callbacks
    for (int i = 0; i < 32; i++) {
        trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
        trellis.activateKey(i, SEESAW_KEYPAD_EDGE_FALLING);
    }
    
    // Initial display setup
    display.setBrightness(50);
    display.clear();
    display.show();
    
    // Start the sequencer
    sequencer->start();
    
    Serial.println("Sequencer initialized and started");
    Serial.println("Hold a step button to enter parameter lock mode");
    Serial.println("In parameter lock mode:");
    Serial.println("- Bottom buttons adjust note (C1/D1 or C5/D5)");
    Serial.println("- Other controls adjust velocity, length, etc.");
}

void loop() {
    // Core sequencer tick (handles timing and parameter locks)
    sequencer->tick();
    
    // Handle hardware button input
    handleButtons();
    
    // Update display
    sequencer->updateDisplay();
    
    // Small delay to prevent overwhelming the system
    delay(1);
}

void handleButtons() {
    // Read button states from hardware
    currentButtonStates = 0;
    
    // NeoTrellis M4 button scanning
    trellis.tick();
    
    while (trellis.available()) {
        keyEvent evt = trellis.read();
        uint8_t key = evt.bit.NUM;
        bool pressed = evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING;
        
        // Update button state mask
        if (pressed) {
            currentButtonStates |= (1UL << key);
        }
        
        // Forward to core sequencer
        if (sequencer) {
            sequencer->handleButton(key, pressed, millis());
        }
        
        // Enhanced debug output
        Serial.print("Button ");
        Serial.print(key);
        Serial.print(pressed ? " pressed" : " released");
        Serial.print(" at time ");
        Serial.print(millis());
        if (sequencer->isInParameterLockMode()) {
            Serial.print(" (Parameter Lock Mode)");
        }
        Serial.println();
        
        // Additional diagnostics for parameter lock debugging
        if (pressed) {
            Serial.print("DEBUG: Button press detected, current mode: ");
            Serial.println(sequencer->isInParameterLockMode() ? "PARAM_LOCK" : "NORMAL");
        }
    }
    
    lastButtonStates = currentButtonStates;
}