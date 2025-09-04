#include "ShiftControls.h"

#ifdef __SAMD51J19A__
    #include "string.h"
    extern "C" {
        void* memset(void* dest, int val, uint32_t len);
    }
#else
    #include <cstring>
#endif

class DefaultClock : public IClock {
public:
    uint32_t getCurrentTime() const override {
        static uint32_t time = 0;
        return time++;
    }
    
    void delay(uint32_t milliseconds) override {
        volatile uint32_t count = milliseconds * 1000;
        while(count--);
    }
    
    void reset() override {
        // Reset implementation if needed
    }
};

ShiftControls::ShiftControls()
    : ownsClock_(false)
    , shiftActive_(false)
    , triggeredAction_(ControlAction::None)
    , lastActionTime_(0) {
    
    static DefaultClock defaultClock;
    clock_ = &defaultClock;
    ownsClock_ = false; // Static instance, don't delete
}

ShiftControls::ShiftControls(Dependencies deps)
    : ownsClock_(false)
    , shiftActive_(false)
    , triggeredAction_(ControlAction::None)
    , lastActionTime_(0) {
    
    if (deps.clock) {
        clock_ = deps.clock;
        ownsClock_ = false;
    } else {
        static DefaultClock defaultClock;
        clock_ = &defaultClock;
        ownsClock_ = false; // Static instance, don't delete
    }
}

ShiftControls::~ShiftControls() {
    if (ownsClock_ && clock_) {
        delete clock_;
    }
}

void ShiftControls::handleShiftInput(const ButtonEvent& event) {
    if (isShiftKey(event.row, event.col)) {
        shiftActive_ = event.pressed;
        return;
    }
    
    // Handle control keys only when shift is active and button is pressed
    if (shiftActive_ && event.pressed && isControlKey(event.row, event.col)) {
        ControlAction action = getControlAction(event.row, event.col);
        if (action != ControlAction::None) {
            triggeredAction_ = action;
            lastActionTime_ = clock_->getCurrentTime();
        }
    }
}

bool ShiftControls::isShiftActive() const {
    return shiftActive_;
}

bool ShiftControls::shouldHandleAsControl(uint8_t row, uint8_t col) const {
    return isShiftKey(row, col) || (shiftActive_ && isControlKey(row, col));
}

ShiftControls::ControlAction ShiftControls::getTriggeredAction() {
    return triggeredAction_;
}

void ShiftControls::clearTriggeredAction() {
    triggeredAction_ = ControlAction::None;
}

bool ShiftControls::isShiftKey(uint8_t row, uint8_t col) const {
    return (row == SHIFT_ROW && col == SHIFT_COL);
}

bool ShiftControls::isControlKey(uint8_t row, uint8_t col) const {
    return (row == START_STOP_ROW && col == START_STOP_COL);
}

ShiftControls::ControlAction ShiftControls::getControlAction(uint8_t row, uint8_t col) const {
    if (row == START_STOP_ROW && col == START_STOP_COL) {
        return ControlAction::StartStop;
    }
    return ControlAction::None;
}