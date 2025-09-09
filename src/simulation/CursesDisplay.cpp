#include "CursesDisplay.h"
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <tuple>

CursesDisplay::CursesDisplay() 
    : initialized_(false), ledWindow_(nullptr), infoWindow_(nullptr), consoleWindow_(nullptr) {
    clear();
}

CursesDisplay::~CursesDisplay() {
    shutdown();
}

void CursesDisplay::init() {
    if (initialized_) return;
    
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0); // Hide cursor
    
    // Check for color support
    if (!has_colors()) {
        endwin();
        throw std::runtime_error("Terminal does not support colors");
    }
    
    start_color();
    initColors();
    
    // Create windows
    int termWidth;
    getmaxyx(stdscr, std::ignore, termWidth);
    
    // LED display window (main area) - start at row 5 to accommodate extra instructions
    ledWindow_ = newwin(ROWS * 2 + 2, COLS * 4 + 2, 5, 2);
    
    // Info window (middle) - instructions
    infoWindow_ = newwin(7, termWidth - 4, ROWS * 2 + 7, 2);
    
    // Console window (bottom) - debug output
    consoleWindow_ = newwin(MAX_CONSOLE_LINES + 2, termWidth - 4, ROWS * 2 + 8, 2);
    
    // Draw borders
    box(ledWindow_, 0, 0);
    box(infoWindow_, 0, 0);
    box(consoleWindow_, 0, 0);
    
    // Console window title
    mvwprintw(consoleWindow_, 0, 2, " Debug Console ");
    
    // Title and instructions
    mvprintw(0, 2, "NeoTrellis M4 Step Sequencer Simulator - 4x8 Grid with Parameter Locks");
    mvprintw(1, 2, "Step Sequencer: RED=Track0, GREEN=Track1, BLUE=Track2, YELLOW=Track3 | Press ESC to quit");
    
    // Updated instructions with correct parameter lock behavior
    mvprintw(2, 2, "CONTROLS: lowercase/numbers=toggle step, hold ≥500ms for parameter lock mode");
    mvprintw(3, 2, "Track 0 (RED): 1 2 3 4 5 6 7 8 | Track 1 (GREEN): q w e r t y u i");
    
    // Corrected track layout with lowercase keys
    mvprintw(4, 2, "Track 2 (BLUE): a s d f g h j k | Track 3 (YELLOW): z x c v b n m ,");
    
    initialized_ = true;
    drawGrid();
    drawInfo();
    drawConsole();
    refresh();
}

void CursesDisplay::shutdown() {
    if (!initialized_) return;
    
    if (ledWindow_) {
        delwin(ledWindow_);
        ledWindow_ = nullptr;
    }
    
    if (infoWindow_) {
        delwin(infoWindow_);
        infoWindow_ = nullptr;
    }
    
    if (consoleWindow_) {
        delwin(consoleWindow_);
        consoleWindow_ = nullptr;
    }
    
    endwin();
    initialized_ = false;
}

void CursesDisplay::initColors() {
    // Initialize color pairs for different LED colors
    // We'll use 256 color mode if available, otherwise basic colors
    
    if (COLORS >= 256) {
        // True color simulation using 256 color palette
        for (int i = 1; i < 64; i++) {
            init_pair(i, i, COLOR_BLACK);
        }
    } else {
        // Basic 8 colors
        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_BLUE, COLOR_BLACK);
        init_pair(4, COLOR_YELLOW, COLOR_BLACK);
        init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(6, COLOR_CYAN, COLOR_BLACK);
        init_pair(7, COLOR_WHITE, COLOR_BLACK);
    }
}

int CursesDisplay::getColorPair(uint8_t r, uint8_t g, uint8_t b) {
    // If all RGB values are low, treat as off (black)
    if (r < 30 && g < 30 && b < 30) {
        return 0; // Default black
    }
    
    if (COLORS >= 256) {
        // Map RGB to 6x6x6 color cube (216 colors) plus grayscale
        int r6 = (r * 5) / 255;
        int g6 = (g * 5) / 255;
        int b6 = (b * 5) / 255;
        int colorIndex = 16 + (r6 * 36) + (g6 * 6) + b6;
        
        // Limit to available pairs
        int pairIndex = std::min(colorIndex % 63 + 1, 63);
        return pairIndex;
    } else {
        // Map to basic 8 colors based on dominant component
        if (r > g && r > b) return 1; // Red
        if (g > r && g > b) return 2; // Green  
        if (b > r && b > g) return 3; // Blue
        if (r > 128 && g > 128) return 4; // Yellow
        if (r > 128 && b > 128) return 5; // Magenta
        if (g > 128 && b > 128) return 6; // Cyan
        return 7; // White
    }
}

void CursesDisplay::setLED(uint8_t row, uint8_t col, uint8_t r, uint8_t g, uint8_t b) {
    if (row >= ROWS || col >= COLS) return;
    
    LED& led = leds_[row][col];
    if (led.r != r || led.g != g || led.b != b) {
        led.r = r;
        led.g = g;
        led.b = b;
        led.dirty = true;
    }
}

void CursesDisplay::clear() {
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            leds_[row][col] = {0, 0, 0, true};
        }
    }
}

void CursesDisplay::refresh() {
    if (!initialized_) return;
    
    drawGrid();
    drawConsole();
    
    // Refresh all windows
    wrefresh(ledWindow_);
    wrefresh(infoWindow_);
    wrefresh(consoleWindow_);
    ::refresh();
}

void CursesDisplay::drawGrid() {
    if (!ledWindow_) return;
    
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            LED& led = leds_[row][col];
            if (led.dirty) {
                int colorPair = getColorPair(led.r, led.g, led.b);
                int winRow = row * 2 + 1;
                int winCol = col * 4 + 1;
                
                // Use colored blocks to represent LEDs
                wattron(ledWindow_, COLOR_PAIR(colorPair));
                mvwprintw(ledWindow_, winRow, winCol, "##");
                mvwprintw(ledWindow_, winRow + 1, winCol, "##");
                wattroff(ledWindow_, COLOR_PAIR(colorPair));
                
                led.dirty = false;
            }
        }
    }
}

void CursesDisplay::drawInfo() {
    if (!infoWindow_) return;
    
    // Clear info area
    werase(infoWindow_);
    box(infoWindow_, 0, 0);
    
    // Updated keyboard mapping with hold capability
    mvwprintw(infoWindow_, 1, 2, "CONTROLS: Press=toggle step, Hold ≥500ms=parameter lock mode (keep holding!)");
    mvwprintw(infoWindow_, 2, 2, "Track 0 (RED):    1 2 3 4 5 6 7 8    |  Track 1 (GREEN):  q w e r t y u i");  
    mvwprintw(infoWindow_, 3, 2, "Track 2 (BLUE):   a s d f g h j k    |  Track 3 (YELLOW): z x c v b n m ,");
    mvwprintw(infoWindow_, 4, 2, "PARAMETER LOCKS: Hold any key ≥500ms to enter param lock mode (keep holding!)");
    mvwprintw(infoWindow_, 5, 2, "Example: Hold 'q' → param lock active → press other keys to adjust → release 'q' to exit");
}

void CursesDisplay::addConsoleMessage(const std::string& message) {
    consoleMessages_.push_back(message);
    
    // Keep only the last MAX_CONSOLE_LINES messages
    while (consoleMessages_.size() > MAX_CONSOLE_LINES) {
        consoleMessages_.pop_front();
    }
    
    // Redraw console if initialized
    if (initialized_) {
        drawConsole();
        wrefresh(consoleWindow_);
    }
}

void CursesDisplay::clearConsole() {
    consoleMessages_.clear();
    if (initialized_) {
        drawConsole();
        wrefresh(consoleWindow_);
    }
}

void CursesDisplay::drawConsole() {
    if (!consoleWindow_) return;
    
    // Clear console area
    werase(consoleWindow_);
    box(consoleWindow_, 0, 0);
    mvwprintw(consoleWindow_, 0, 2, " Debug Console ");
    
    // Draw messages
    int row = 1;
    for (const auto& message : consoleMessages_) {
        if (row > MAX_CONSOLE_LINES) break;
        mvwprintw(consoleWindow_, row, 2, "%s", message.c_str());
        row++;
    }
}