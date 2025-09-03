#include "NeoTrellisInput.h"
#include "NeoTrellisHardware.h"

#ifdef __SAMD51J19A__
    #include "string.h"
    extern "C" {
        void* memset(void* dest, int val, uint32_t len);
    }
#else
    #include <cstring>
#endif

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
    
    // Read hardware state via NeoTrellis API
    g_neoTrellis.read();
    
    // For now, since the hardware polling isn't fully implemented,
    // we'll simulate some button events for testing
    // TODO: Implement proper hardware button reading
    
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