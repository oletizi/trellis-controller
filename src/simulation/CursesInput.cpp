#include "CursesInput.h"
#include <ncurses.h>
#include <cstring>

CursesInput::CursesInput(IClock* clock) 
    : clock_(clock), initialized_(false) {
    memset(buttonStates_, false, sizeof(buttonStates_));
}

CursesInput::~CursesInput() {
    shutdown();
}

void CursesInput::init() {
    if (initialized_) return;
    
    initKeyMapping();
    
    // Set non-blocking input mode
    nodelay(stdscr, TRUE);
    
    initialized_ = true;
}

void CursesInput::shutdown() {
    if (!initialized_) return;
    
    // Clear event queue
    while (!eventQueue_.empty()) {
        eventQueue_.pop();
    }
    
    initialized_ = false;
}

void CursesInput::initKeyMapping() {
    // Row 0: 1 2 3 4 5 6 7 8 (numbers and shifted symbols)
    keyMap_['1'] = {0, 0}; keyMap_['2'] = {0, 1}; keyMap_['3'] = {0, 2}; keyMap_['4'] = {0, 3};
    keyMap_['5'] = {0, 4}; keyMap_['6'] = {0, 5}; keyMap_['7'] = {0, 6}; keyMap_['8'] = {0, 7};
    keyMap_['!'] = {0, 0}; keyMap_['@'] = {0, 1}; keyMap_['#'] = {0, 2}; keyMap_['$'] = {0, 3};
    keyMap_['%'] = {0, 4}; keyMap_['^'] = {0, 5}; keyMap_['&'] = {0, 6}; keyMap_['*'] = {0, 7};
    
    // Row 1: Q W E R T Y U I (uppercase = PRESS, lowercase = RELEASE)
    keyMap_['q'] = {1, 0}; keyMap_['w'] = {1, 1}; keyMap_['e'] = {1, 2}; keyMap_['r'] = {1, 3};
    keyMap_['t'] = {1, 4}; keyMap_['y'] = {1, 5}; keyMap_['u'] = {1, 6}; keyMap_['i'] = {1, 7};
    keyMap_['Q'] = {1, 0}; keyMap_['W'] = {1, 1}; keyMap_['E'] = {1, 2}; keyMap_['R'] = {1, 3};
    keyMap_['T'] = {1, 4}; keyMap_['Y'] = {1, 5}; keyMap_['U'] = {1, 6}; keyMap_['I'] = {1, 7};
    
    // Row 2: A S D F G H J K (uppercase = PRESS, lowercase = RELEASE)
    keyMap_['a'] = {2, 0}; keyMap_['s'] = {2, 1}; keyMap_['d'] = {2, 2}; keyMap_['f'] = {2, 3};
    keyMap_['g'] = {2, 4}; keyMap_['h'] = {2, 5}; keyMap_['j'] = {2, 6}; keyMap_['k'] = {2, 7};
    keyMap_['A'] = {2, 0}; keyMap_['S'] = {2, 1}; keyMap_['D'] = {2, 2}; keyMap_['F'] = {2, 3};
    keyMap_['G'] = {2, 4}; keyMap_['H'] = {2, 5}; keyMap_['J'] = {2, 6}; keyMap_['K'] = {2, 7};
    
    // Row 3: Z X C V B N M , (uppercase = PRESS, lowercase = RELEASE)
    keyMap_['z'] = {3, 0}; keyMap_['x'] = {3, 1}; keyMap_['c'] = {3, 2}; keyMap_['v'] = {3, 3};
    keyMap_['b'] = {3, 4}; keyMap_['n'] = {3, 5}; keyMap_['m'] = {3, 6}; keyMap_[','] = {3, 7};
    keyMap_['Z'] = {3, 0}; keyMap_['X'] = {3, 1}; keyMap_['C'] = {3, 2}; keyMap_['V'] = {3, 3};
    keyMap_['B'] = {3, 4}; keyMap_['N'] = {3, 5}; keyMap_['M'] = {3, 6}; keyMap_['<'] = {3, 7};
}

bool CursesInput::getKeyMapping(int key, uint8_t& row, uint8_t& col) {
    auto it = keyMap_.find(key);
    if (it != keyMap_.end()) {
        row = it->second.row;
        col = it->second.col;
        return true;
    }
    return false;
}

bool CursesInput::pollEvents() {
    if (!initialized_) return false;
    
    int key;
    while ((key = getch()) != ERR) {
        uint8_t row, col;
        if (getKeyMapping(key, row, col)) {
            bool shouldPress = determineKeyAction(key);
            bool currentState = buttonStates_[row][col];
            
            // Only generate event if state actually changes
            if (shouldPress != currentState) {
                buttonStates_[row][col] = shouldPress;
                
                ButtonEvent event;
                event.row = row;
                event.col = col;
                event.pressed = shouldPress;
                event.timestamp = clock_ ? clock_->getCurrentTime() : 0;
                eventQueue_.push(event);
            }
        }
        
        // Handle special keys
        if (key == 27) { // ESC key - quit simulation
            ButtonEvent quitEvent;
            quitEvent.row = 255; // Special quit signal
            quitEvent.col = 255;
            quitEvent.pressed = true;
            quitEvent.timestamp = clock_ ? clock_->getCurrentTime() : 0;
            eventQueue_.push(quitEvent);
        }
    }
    
    return !eventQueue_.empty();
}

bool CursesInput::getNextEvent(ButtonEvent& event) {
    if (eventQueue_.empty()) {
        return false;
    }
    
    event = eventQueue_.front();
    eventQueue_.pop();
    return true;
}

bool CursesInput::isButtonPressed(uint8_t row, uint8_t col) const {
    if (row >= ROWS || col >= COLS) {
        return false;
    }
    return buttonStates_[row][col];
}

bool CursesInput::determineKeyAction(int key) {
    // For numbers, use shift state to determine press/release
    // Numbers (1-8) = PRESS, Shifted symbols (!@#$%^&*) = RELEASE
    if (key >= '1' && key <= '8') {
        return true; // PRESS
    }
    if (key == '!' || key == '@' || key == '#' || key == '$' || 
        key == '%' || key == '^' || key == '&' || key == '*') {
        return false; // RELEASE
    }
    
    // For letters, uppercase = PRESS, lowercase = RELEASE
    if (isUpperCase(key)) {
        return true; // PRESS
    } else {
        return false; // RELEASE  
    }
}

bool CursesInput::isUpperCase(int key) {
    return (key >= 'A' && key <= 'Z') || key == '<'; // '<' is shift+comma
}