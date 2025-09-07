#ifndef CURSES_INPUT_H
#define CURSES_INPUT_H

#include "IInput.h"
#include "IClock.h"
#include <queue>
#include <map>

class CursesInput : public IInput {
public:
    static constexpr uint8_t ROWS = 4;
    static constexpr uint8_t COLS = 8;
    
    explicit CursesInput(IClock* clock);
    ~CursesInput() override;
    
    void init() override;
    void shutdown() override;
    
    bool pollEvents() override;
    bool getNextEvent(ButtonEvent& event) override;
    bool isButtonPressed(uint8_t row, uint8_t col) const override;
    
    uint8_t getRows() const override { return ROWS; }
    uint8_t getCols() const override { return COLS; }

private:
    struct KeyMapping {
        uint8_t row;
        uint8_t col;
    };
    
    IClock* clock_;
    bool initialized_;
    std::map<int, KeyMapping> keyMap_;
    std::queue<ButtonEvent> eventQueue_;
    bool buttonStates_[ROWS][COLS];
    
    void initKeyMapping();
    bool getKeyMapping(int key, uint8_t& row, uint8_t& col);
    bool determineKeyAction(int key);
    bool isUpperCase(int key);
    void handleKeyPress(int key);
    void handleKeyRelease(int key);
};

#endif