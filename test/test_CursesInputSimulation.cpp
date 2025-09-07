#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "CursesInput.h"

using namespace Catch::Matchers;

// Mock clock for deterministic testing
class MockClock : public IClock {
private:
    uint32_t currentTime_;
    
public:
    MockClock(uint32_t startTime = 1000) : currentTime_(startTime) {}
    
    uint32_t getCurrentTime() const override { return currentTime_; }
    void delay(uint32_t milliseconds) override { currentTime_ += milliseconds; }
    void reset() override { currentTime_ = 1000; }
    
    void advance(uint32_t ms) { currentTime_ += ms; }
    void setTime(uint32_t time) { currentTime_ = time; }
};

// Mock ncurses functions for testing
// Note: In a real test environment, you'd want to mock these properly
// For now, we'll test the logic without actual ncurses dependency

TEST_CASE("CursesInput - Key mapping functionality", "[curses_input][key_mapping]") {
    MockClock mockClock;
    CursesInput input(&mockClock);
    
    SECTION("Key to row/column mapping") {
        uint8_t row, col;
        
        // Test row 0 (numbers 1-8)
        REQUIRE(input.getKeyMapping('1', row, col));
        REQUIRE(row == 0);
        REQUIRE(col == 0);
        
        REQUIRE(input.getKeyMapping('8', row, col));
        REQUIRE(row == 0);
        REQUIRE(col == 7);
        
        // Test row 1 (QWER... letters)
        REQUIRE(input.getKeyMapping('q', row, col));
        REQUIRE(row == 1);
        REQUIRE(col == 0);
        
        REQUIRE(input.getKeyMapping('Q', row, col)); // Uppercase
        REQUIRE(row == 1);
        REQUIRE(col == 0);
        
        REQUIRE(input.getKeyMapping('i', row, col));
        REQUIRE(row == 1);
        REQUIRE(col == 7);
        
        // Test row 2 (ASDF... letters)
        REQUIRE(input.getKeyMapping('a', row, col));
        REQUIRE(row == 2);
        REQUIRE(col == 0);
        
        REQUIRE(input.getKeyMapping('k', row, col));
        REQUIRE(row == 2);
        REQUIRE(col == 7);
        
        // Test row 3 (ZXCV... letters)
        REQUIRE(input.getKeyMapping('z', row, col));
        REQUIRE(row == 3);
        REQUIRE(col == 0);
        
        REQUIRE(input.getKeyMapping(',', row, col));
        REQUIRE(row == 3);
        REQUIRE(col == 7);
    }
    
    SECTION("Invalid key mappings") {
        uint8_t row, col;
        
        // Test unmapped keys
        REQUIRE_FALSE(input.getKeyMapping('9', row, col));
        REQUIRE_FALSE(input.getKeyMapping('0', row, col));
        REQUIRE_FALSE(input.getKeyMapping('p', row, col));
        REQUIRE_FALSE(input.getKeyMapping('l', row, col));
        REQUIRE_FALSE(input.getKeyMapping('.', row, col));
        REQUIRE_FALSE(input.getKeyMapping('/', row, col));
    }
    
    SECTION("Shifted key mappings") {
        uint8_t row, col;
        
        // Test shifted number keys (symbols)
        REQUIRE(input.getKeyMapping('!', row, col)); // Shift+1
        REQUIRE(row == 0);
        REQUIRE(col == 0);
        
        REQUIRE(input.getKeyMapping('@', row, col)); // Shift+2
        REQUIRE(row == 0);
        REQUIRE(col == 1);
        
        REQUIRE(input.getKeyMapping('*', row, col)); // Shift+8
        REQUIRE(row == 0);
        REQUIRE(col == 7);
        
        // Test shift+comma
        REQUIRE(input.getKeyMapping('<', row, col)); // Shift+comma
        REQUIRE(row == 3);
        REQUIRE(col == 7);
    }
}

TEST_CASE("CursesInput - Key action determination", "[curses_input][key_actions]") {
    MockClock mockClock;
    CursesInput input(&mockClock);
    
    SECTION("Number key actions") {
        // Numbers (1-8) should be PRESS
        REQUIRE(input.determineKeyAction('1') == true);
        REQUIRE(input.determineKeyAction('2') == true);
        REQUIRE(input.determineKeyAction('8') == true);
        
        // Shifted numbers (!@#$%^&*) should be RELEASE
        REQUIRE(input.determineKeyAction('!') == false);
        REQUIRE(input.determineKeyAction('@') == false);
        REQUIRE(input.determineKeyAction('#') == false);
        REQUIRE(input.determineKeyAction('$') == false);
        REQUIRE(input.determineKeyAction('%') == false);
        REQUIRE(input.determineKeyAction('^') == false);
        REQUIRE(input.determineKeyAction('&') == false);
        REQUIRE(input.determineKeyAction('*') == false);
    }
    
    SECTION("Letter key actions") {
        // Uppercase letters should be PRESS
        REQUIRE(input.determineKeyAction('Q') == true);
        REQUIRE(input.determineKeyAction('W') == true);
        REQUIRE(input.determineKeyAction('A') == true);
        REQUIRE(input.determineKeyAction('S') == true);
        REQUIRE(input.determineKeyAction('Z') == true);
        REQUIRE(input.determineKeyAction('X') == true);
        
        // Lowercase letters should be RELEASE
        REQUIRE(input.determineKeyAction('q') == false);
        REQUIRE(input.determineKeyAction('w') == false);
        REQUIRE(input.determineKeyAction('a') == false);
        REQUIRE(input.determineKeyAction('s') == false);
        REQUIRE(input.determineKeyAction('z') == false);
        REQUIRE(input.determineKeyAction('x') == false);
    }
    
    SECTION("Special case - comma and shift+comma") {
        // Regular comma should be RELEASE (lowercase equivalent)
        REQUIRE(input.determineKeyAction(',') == false);
        
        // Shift+comma (<) should be PRESS (uppercase equivalent)
        REQUIRE(input.determineKeyAction('<') == true);
    }
    
    SECTION("Uppercase detection") {
        // Test uppercase detection function
        REQUIRE(input.isUpperCase('A') == true);
        REQUIRE(input.isUpperCase('Z') == true);
        REQUIRE(input.isUpperCase('a') == false);
        REQUIRE(input.isUpperCase('z') == false);
        REQUIRE(input.isUpperCase('<') == true); // Special case for shift+comma
        REQUIRE(input.isUpperCase(',') == false);
        REQUIRE(input.isUpperCase('1') == false);
        REQUIRE(input.isUpperCase('!') == false);
    }
}

TEST_CASE("CursesInput - Button state management", "[curses_input][button_states]") {
    MockClock mockClock;
    CursesInput input(&mockClock);
    
    SECTION("Initial button states") {
        // All buttons should be unpressed initially
        for (uint8_t row = 0; row < CursesInput::ROWS; ++row) {
            for (uint8_t col = 0; col < CursesInput::COLS; ++col) {
                REQUIRE_FALSE(input.isButtonPressed(row, col));
            }
        }
    }
    
    SECTION("Button state bounds checking") {
        // Test invalid row/column values
        REQUIRE_FALSE(input.isButtonPressed(CursesInput::ROWS, 0));
        REQUIRE_FALSE(input.isButtonPressed(0, CursesInput::COLS));
        REQUIRE_FALSE(input.isButtonPressed(255, 255));
    }
}

TEST_CASE("CursesInput - Hold simulation patterns", "[curses_input][hold_simulation]") {
    MockClock mockClock;
    CursesInput input(&mockClock);
    
    // Note: These tests simulate the intended usage pattern for parameter lock testing
    // In practice, the test framework would need to inject key events or mock ncurses
    
    SECTION("Hold simulation concept") {
        // This documents the intended behavior for parameter lock testing:
        
        // To simulate holding button at row 1, col 3 (which maps to 'r'/'R'):
        // 1. Send 'R' (uppercase) to simulate PRESS
        // 2. Wait for hold threshold (500ms)
        // 3. Send 'r' (lowercase) to simulate RELEASE
        
        uint8_t testRow = 1, testCol = 3;
        char pressKey = 'R';
        char releaseKey = 'r';
        
        // Verify the mapping
        uint8_t mappedRow, mappedCol;
        REQUIRE(input.getKeyMapping(pressKey, mappedRow, mappedCol));
        REQUIRE(mappedRow == testRow);
        REQUIRE(mappedCol == testCol);
        
        REQUIRE(input.getKeyMapping(releaseKey, mappedRow, mappedCol));
        REQUIRE(mappedRow == testRow);
        REQUIRE(mappedCol == testCol);
        
        // Verify the actions
        REQUIRE(input.determineKeyAction(pressKey) == true);  // PRESS
        REQUIRE(input.determineKeyAction(releaseKey) == false); // RELEASE
    }
    
    SECTION("All button hold patterns") {
        // Document hold patterns for all testable buttons
        struct HoldPattern {
            uint8_t row, col;
            char pressKey, releaseKey;
            const char* description;
        };
        
        HoldPattern patterns[] = {
            // Row 0 - Numbers and shifted symbols
            {0, 0, '1', '!', "Button 0 (1/!)"},
            {0, 1, '2', '@', "Button 1 (2/@)"},
            {0, 2, '3', '#', "Button 2 (3/#)"},
            {0, 3, '4', '$', "Button 3 (4/$)"},
            {0, 4, '5', '%', "Button 4 (5/%)"},
            {0, 5, '6', '^', "Button 5 (6/^)"},
            {0, 6, '7', '&', "Button 6 (7/&)"},
            {0, 7, '8', '*', "Button 7 (8/*)"},
            
            // Row 1 - QWERTY row
            {1, 0, 'Q', 'q', "Button 8 (Q/q)"},
            {1, 1, 'W', 'w', "Button 9 (W/w)"},
            {1, 2, 'E', 'e', "Button 10 (E/e)"},
            {1, 3, 'R', 'r', "Button 11 (R/r)"},
            {1, 4, 'T', 't', "Button 12 (T/t)"},
            {1, 5, 'Y', 'y', "Button 13 (Y/y)"},
            {1, 6, 'U', 'u', "Button 14 (U/u)"},
            {1, 7, 'I', 'i', "Button 15 (I/i)"},
            
            // Row 2 - ASDF row
            {2, 0, 'A', 'a', "Button 16 (A/a)"},
            {2, 1, 'S', 's', "Button 17 (S/s)"},
            {2, 2, 'D', 'd', "Button 18 (D/d)"},
            {2, 3, 'F', 'f', "Button 19 (F/f)"},
            {2, 4, 'G', 'g', "Button 20 (G/g)"},
            {2, 5, 'H', 'h', "Button 21 (H/h)"},
            {2, 6, 'J', 'j', "Button 22 (J/j)"},
            {2, 7, 'K', 'k', "Button 23 (K/k)"},
            
            // Row 3 - ZXCV row
            {3, 0, 'Z', 'z', "Button 24 (Z/z)"},
            {3, 1, 'X', 'x', "Button 25 (X/x)"},
            {3, 2, 'C', 'c', "Button 26 (C/c)"},
            {3, 3, 'V', 'v', "Button 27 (V/v)"},
            {3, 4, 'B', 'b', "Button 28 (B/b)"},
            {3, 5, 'N', 'n', "Button 29 (N/n)"},
            {3, 6, 'M', 'm', "Button 30 (M/m)"},
            {3, 7, '<', ',', "Button 31 (</>)"},
        };
        
        for (const auto& pattern : patterns) {
            // Verify mapping consistency
            uint8_t pressRow, pressCol, releaseRow, releaseCol;
            
            REQUIRE(input.getKeyMapping(pattern.pressKey, pressRow, pressCol));
            REQUIRE(input.getKeyMapping(pattern.releaseKey, releaseRow, releaseCol));
            
            REQUIRE(pressRow == pattern.row);
            REQUIRE(pressCol == pattern.col);
            REQUIRE(releaseRow == pattern.row);
            REQUIRE(releaseCol == pattern.col);
            
            // Verify action determination
            REQUIRE(input.determineKeyAction(pattern.pressKey) == true);
            REQUIRE(input.determineKeyAction(pattern.releaseKey) == false);
        }
    }
}

TEST_CASE("CursesInput - Parameter lock testing scenarios", "[curses_input][parameter_lock_scenarios]") {
    MockClock mockClock;
    CursesInput input(&mockClock);
    
    SECTION("Step button hold scenario - Button 11 (R/r)") {
        // Simulate holding step button 11 (track 1, step 3)
        // This corresponds to 'R' for press and 'r' for release
        
        const uint8_t targetRow = 1, targetCol = 3;
        const char pressKey = 'R', releaseKey = 'r';
        
        // Verify this maps to the intended button
        uint8_t row, col;
        REQUIRE(input.getKeyMapping(pressKey, row, col));
        REQUIRE(row == targetRow);
        REQUIRE(col == targetCol);
        
        // Calculate button index (row * 8 + col)
        uint8_t buttonIndex = targetRow * 8 + targetCol; // Should be 11
        REQUIRE(buttonIndex == 11);
        
        // This button corresponds to track 1, step 3 in the sequencer layout
        uint8_t track = buttonIndex / 8; // Should be 1
        uint8_t step = buttonIndex % 8;  // Should be 3
        REQUIRE(track == 1);
        REQUIRE(step == 3);
        
        INFO("Button " << (int)buttonIndex << " (R/r) maps to Track " << (int)track << ", Step " << (int)step);
    }
    
    SECTION("Control grid button scenarios") {
        // When holding step 3 (left side), control grid should be on right side (columns 4-7)
        // Test some control buttons that would be used for parameter adjustment
        
        struct ControlButton {
            uint8_t row, col, buttonIndex;
            char pressKey, releaseKey;
            const char* expectedFunction;
        };
        
        ControlButton controlButtons[] = {
            // Right side buttons (columns 4-7) - these would be control grid for left-side held steps
            {0, 4, 4, '5', '%', "Note up/down or velocity control"},
            {0, 5, 5, '6', '^', "Note up/down or velocity control"},
            {0, 6, 6, '7', '&', "Note up/down or velocity control"},
            {0, 7, 7, '8', '*', "Note up/down or velocity control"},
            
            {1, 4, 12, 'T', 't', "Parameter control"},
            {1, 5, 13, 'Y', 'y', "Parameter control"},
            {1, 6, 14, 'U', 'u', "Parameter control"},
            {1, 7, 15, 'I', 'i', "Parameter control"},
            
            // More control buttons...
            {2, 4, 20, 'G', 'g', "Parameter control"},
            {2, 5, 21, 'H', 'h', "Parameter control"},
            {3, 4, 28, 'B', 'b', "Parameter control"},
            {3, 5, 29, 'N', 'n', "Parameter control"},
        };
        
        for (const auto& button : controlButtons) {
            // Verify the mapping
            uint8_t row, col;
            REQUIRE(input.getKeyMapping(button.pressKey, row, col));
            REQUIRE(row == button.row);
            REQUIRE(col == button.col);
            
            uint8_t calculatedIndex = row * 8 + col;
            REQUIRE(calculatedIndex == button.buttonIndex);
            
            // Verify press/release actions
            REQUIRE(input.determineKeyAction(button.pressKey) == true);
            REQUIRE(input.determineKeyAction(button.releaseKey) == false);
            
            INFO("Control Button " << (int)button.buttonIndex << " (" << button.pressKey << "/" << button.releaseKey << ") - " << button.expectedFunction);
        }
    }
    
    SECTION("Complete parameter lock test sequence documentation") {
        // Document a complete test sequence for automated testing
        
        INFO("Parameter Lock Test Sequence:");
        INFO("1. Press 'R' to simulate holding step button 11 (Track 1, Step 3)");
        INFO("2. Wait 500ms for hold threshold");
        INFO("3. AdaptiveButtonTracker should detect hold");
        INFO("4. SequencerStateManager should enter parameter lock mode");
        INFO("5. ControlGrid should map right-side buttons (4-7, 12-15, 20-23, 28-31) as controls");
        INFO("6. Press control buttons like 'T', 'Y', 'U', 'I' to adjust parameters");
        INFO("7. Press corresponding lowercase letters to release control buttons");
        INFO("8. Press 'r' to release the held step button");
        INFO("9. SequencerStateManager should exit parameter lock mode");
        INFO("10. Parameter locks should persist in ParameterLockPool");
        
        // This documentation helps create the automated test scripts
    }
}