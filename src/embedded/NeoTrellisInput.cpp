#include "NeoTrellisInput.h"
#include "SeesawI2C.h"
#include "SeesawProtocol.h"

#ifdef __SAMD51J19A__
    #include "string.h"
    extern "C" {
        void* memset(void* dest, int val, uint32_t len);
    }
#else
    #include <cstring>
#endif

// External Seesaw I2C interface (shared with display)
extern SeesawI2C g_seesaw;

NeoTrellisInput::NeoTrellisInput(IClock* clock) 
    : clock_(clock), initialized_(false), eventQueueHead_(0), eventQueueTail_(0), eventQueueCount_(0) {
    memset(buttonStates_, false, sizeof(buttonStates_));
}

void NeoTrellisInput::init() {
    if (initialized_) return;
    
    // NeoTrellis hardware should already be initialized by display
    initialized_ = true;
}

void NeoTrellisInput::shutdown() {
    if (!initialized_) return;
    
    // Clear event queue
    eventQueueHead_ = eventQueueTail_ = eventQueueCount_ = 0;
    memset(buttonStates_, false, sizeof(buttonStates_));
    
    initialized_ = false;
}

bool NeoTrellisInput::pollEvents() {
    if (!initialized_) return false;
    
    // Read keypad events from Seesaw FIFO
    uint8_t eventCount;
    if (g_seesaw.readKeypadCount(&eventCount) != SeesawI2C::SUCCESS) {
        return eventQueueCount_ > 0; // Return existing events on error
    }
    
    if (eventCount > 0) {
        // Read up to available events (max 8 at a time to avoid overflow)
        uint8_t readCount = (eventCount > 8) ? 8 : eventCount;
        Seesaw::KeyEvent events[8];
        
        if (g_seesaw.readKeypadFIFO((uint8_t*)events, readCount) == SeesawI2C::SUCCESS) {
            // Process each event
            for (uint8_t i = 0; i < readCount; i++) {
                uint8_t keyNum = events[i].bit.NUM;
                bool pressed = (events[i].bit.EDGE == Seesaw::KEYPAD_EDGE_RISING);
                
                // Convert linear key number to row/col (4x8 layout)
                uint8_t row = keyNum / COLS;
                uint8_t col = keyNum % COLS;
                
                if (row < ROWS && col < COLS) {
                    // Update button state and add event
                    bool wasPressed = buttonStates_[row][col];
                    if (pressed != wasPressed) {
                        buttonStates_[row][col] = pressed;
                        addEvent(row, col, pressed);
                    }
                }
            }
        }
    }
    
    return eventQueueCount_ > 0;
}

bool NeoTrellisInput::getNextEvent(ButtonEvent& event) {
    if (eventQueueCount_ == 0) {
        return false;
    }
    
    event = eventQueue_[eventQueueTail_];
    eventQueueTail_ = (eventQueueTail_ + 1) % MAX_EVENTS;
    eventQueueCount_--;
    
    return true;
}

bool NeoTrellisInput::isButtonPressed(uint8_t row, uint8_t col) const {
    if (row >= ROWS || col >= COLS) {
        return false;
    }
    return buttonStates_[row][col];
}

void NeoTrellisInput::addEvent(uint8_t row, uint8_t col, bool pressed) {
    if (isEventQueueFull()) {
        // Drop oldest event if queue is full
        eventQueueTail_ = (eventQueueTail_ + 1) % MAX_EVENTS;
        eventQueueCount_--;
    }
    
    ButtonEvent& event = eventQueue_[eventQueueHead_];
    event.row = row;
    event.col = col;
    event.pressed = pressed;
    event.timestamp = clock_ ? clock_->getCurrentTime() : 0;
    
    eventQueueHead_ = (eventQueueHead_ + 1) % MAX_EVENTS;
    eventQueueCount_++;
}

bool NeoTrellisInput::isEventQueueFull() const {
    return eventQueueCount_ >= MAX_EVENTS;
}