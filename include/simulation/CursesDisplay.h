#ifndef CURSES_DISPLAY_H
#define CURSES_DISPLAY_H

#include "IDisplay.h"
#include <ncurses.h>
#include <stdint.h>
#include <string>
#include <deque>

class CursesDisplay : public IDisplay {
public:
    static constexpr uint8_t ROWS = 4;
    static constexpr uint8_t COLS = 8;
    static constexpr int MAX_CONSOLE_LINES = 8;
    
    CursesDisplay();
    ~CursesDisplay() override;
    
    void init() override;
    void shutdown() override;
    
    void setLED(uint8_t row, uint8_t col, uint8_t r, uint8_t g, uint8_t b) override;
    void clear() override;
    void refresh() override;
    
    uint8_t getRows() const override { return ROWS; }
    uint8_t getCols() const override { return COLS; }
    
    // Console output for debug messages
    void addConsoleMessage(const std::string& message);
    void clearConsole();

private:
    struct LED {
        uint8_t r, g, b;
        bool dirty;
    };
    
    LED leds_[ROWS][COLS];
    bool initialized_;
    WINDOW* ledWindow_;
    WINDOW* infoWindow_;
    WINDOW* consoleWindow_;
    std::deque<std::string> consoleMessages_;
    
    void initColors();
    int getColorPair(uint8_t r, uint8_t g, uint8_t b);
    void drawGrid();
    void drawInfo();
    void drawConsole();
};

#endif