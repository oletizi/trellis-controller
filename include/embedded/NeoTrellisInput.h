#ifndef NEOTRELLIS_INPUT_H
#define NEOTRELLIS_INPUT_H

#include "IInput.h"
#include "IClock.h"

class NeoTrellisInput : public IInput {
public:
    static constexpr uint8_t ROWS = 4;
    static constexpr uint8_t COLS = 8;
    static constexpr uint8_t MAX_EVENTS = 16;
    
    explicit NeoTrellisInput(IClock* clock);
    ~NeoTrellisInput() override = default;
    
    void init() override;
    void shutdown() override;
    
    bool pollEvents() override;
    bool getNextEvent(ButtonEvent& event) override;
    bool isButtonPressed(uint8_t row, uint8_t col) const override;
    
    uint8_t getRows() const override { return ROWS; }
    uint8_t getCols() const override { return COLS; }

private:
    IClock* clock_;
    bool initialized_;
    bool buttonStates_[ROWS][COLS];
    ButtonEvent eventQueue_[MAX_EVENTS];
    uint8_t eventQueueHead_;
    uint8_t eventQueueTail_;
    uint8_t eventQueueCount_;
    
    void addEvent(uint8_t row, uint8_t col, bool pressed);
    bool isEventQueueFull() const;
};

#endif