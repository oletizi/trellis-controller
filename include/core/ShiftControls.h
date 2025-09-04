#ifndef SHIFTCONTROLS_H
#define SHIFTCONTROLS_H

#include "IShiftControls.h"
#include "IClock.h"

class ShiftControls : public IShiftControls {
public:
    static constexpr uint8_t SHIFT_ROW = 3;
    static constexpr uint8_t SHIFT_COL = 0;
    static constexpr uint8_t START_STOP_ROW = 3;
    static constexpr uint8_t START_STOP_COL = 7;
    
    enum class ControlAction {
        None,
        StartStop
    };
    
    struct Dependencies {
        IClock* clock = nullptr;
    };
    
    ShiftControls();
    explicit ShiftControls(Dependencies deps);
    ~ShiftControls();
    
    void handleShiftInput(const ButtonEvent& event) override;
    bool isShiftActive() const override;
    bool shouldHandleAsControl(uint8_t row, uint8_t col) const override;
    
    ControlAction getTriggeredAction();
    void clearTriggeredAction();

private:
    IClock* clock_;
    bool ownsClock_;
    bool shiftActive_;
    ControlAction triggeredAction_;
    uint32_t lastActionTime_;
    
    static IClock* createDefaultClock();
    
    bool isShiftKey(uint8_t row, uint8_t col) const;
    bool isControlKey(uint8_t row, uint8_t col) const;
    ControlAction getControlAction(uint8_t row, uint8_t col) const;
};

#endif