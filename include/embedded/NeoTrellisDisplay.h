#ifndef NEOTRELLIS_DISPLAY_H
#define NEOTRELLIS_DISPLAY_H

#include "IDisplay.h"

class NeoTrellisDisplay : public IDisplay {
public:
    static constexpr uint8_t ROWS = 4;
    static constexpr uint8_t COLS = 8;
    
    NeoTrellisDisplay();
    ~NeoTrellisDisplay() override = default;
    
    void init() override;
    void shutdown() override;
    
    void setLED(uint8_t row, uint8_t col, uint8_t r, uint8_t g, uint8_t b) override;
    void clear() override;
    void refresh() override;
    
    uint8_t getRows() const override { return ROWS; }
    uint8_t getCols() const override { return COLS; }

private:
    struct LED {
        uint8_t r, g, b;
        bool dirty;
    };
    
    LED leds_[ROWS][COLS];
    bool initialized_;
    
    void updateHardware();
};

#endif