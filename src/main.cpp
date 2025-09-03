#include <stdint.h>
#include "NeoTrellis.h"
#include "StepSequencer.h"

constexpr uint32_t SAMPLE_RATE = 48000;
constexpr uint32_t BUFFER_SIZE = 256;

NeoTrellis trellis;
StepSequencer sequencer;

void handleButtonEvent(keyEvent evt);
void updateLED(uint8_t index, bool active);
void updatePlayhead();
uint32_t millis();
void delay(uint32_t ms);

void setup() {
    trellis.begin();
    
    trellis.pixels.setBrightness(50);
    trellis.pixels.clear();
    trellis.pixels.show();
    
    for (uint8_t i = 0; i < 32; i++) {
        trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
        trellis.activateKey(i, SEESAW_KEYPAD_EDGE_FALLING);
        trellis.registerCallback(i, handleButtonEvent);
    }
    
    sequencer.init(120, 16);
}

void handleButtonEvent(keyEvent evt);
void updateLED(uint8_t index, bool active);
void updatePlayhead();

void handleButtonEvent(keyEvent evt) {
    uint8_t row = evt.bit.NUM / 8;
    uint8_t col = evt.bit.NUM % 8;
    
    if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
        sequencer.toggleStep(row, col);
        updateLED(evt.bit.NUM, sequencer.isStepActive(row, col));
    }
}

void updateLED(uint8_t index, bool active) {
    if (active) {
        trellis.pixels.setPixelColor(index, 0xFF0000);
    } else {
        trellis.pixels.setPixelColor(index, 0x000000);
    }
    trellis.pixels.show();
}

void updatePlayhead() {
    static uint8_t lastPosition = 0;
    uint8_t currentPosition = sequencer.getCurrentStep();
    
    if (currentPosition != lastPosition) {
        for (uint8_t row = 0; row < 4; row++) {
            uint8_t lastIndex = row * 8 + lastPosition;
            uint8_t currentIndex = row * 8 + currentPosition;
            
            if (sequencer.isStepActive(row, lastPosition)) {
                trellis.pixels.setPixelColor(lastIndex, 0xFF0000);
            } else {
                trellis.pixels.setPixelColor(lastIndex, 0x000000);
            }
            
            if (sequencer.isStepActive(row, currentPosition)) {
                trellis.pixels.setPixelColor(currentIndex, 0x00FF00);
            } else {
                trellis.pixels.setPixelColor(currentIndex, 0x0000FF);
            }
        }
        trellis.pixels.show();
        lastPosition = currentPosition;
    }
}

int main() {
    setup();
    
    uint32_t lastTime = 0;
    
    while (true) {
        trellis.read();
        
        uint32_t currentTime = millis();
        if (currentTime - lastTime >= 10) {
            sequencer.tick();
            updatePlayhead();
            lastTime = currentTime;
        }
        
        delay(5);
    }
    
    return 0;
}

uint32_t millis() {
    static uint32_t milliseconds = 0;
    return milliseconds++;
}

void delay(uint32_t ms) {
    volatile uint32_t count = ms * 1000;
    while(count--);
}