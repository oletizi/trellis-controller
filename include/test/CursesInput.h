#ifndef CURSES_INPUT_LEGACY_H
#define CURSES_INPUT_LEGACY_H

/**
 * @file CursesInput.h 
 * @brief Legacy compatibility wrapper for old CursesInput tests
 * 
 * This header provides backward compatibility for tests that were written
 * for the old CursesInput class. It wraps the new CursesInputLayer functionality
 * to provide the expected interface.
 * 
 * IMPORTANT: This is only for legacy test compatibility. New code should
 * use CursesInputLayer directly with the new InputEvent/InputController architecture.
 */

#include "IClock.h"
#include <stdint.h>

/**
 * @brief Legacy compatibility wrapper for CursesInput tests
 * 
 * This class provides the interface that old tests expect from CursesInput,
 * but implements it using static methods that don't depend on the new
 * input system architecture (which requires initialization, etc).
 * 
 * This allows existing unit tests to run without major refactoring.
 */
class CursesInput {
public:
    static constexpr uint8_t ROWS = 4;
    static constexpr uint8_t COLS = 8;
    
    explicit CursesInput(IClock* clock) : clock_(clock) {
        // Initialize button states to false
        for (int i = 0; i < ROWS * COLS; ++i) {
            buttonStates_[i] = false;
        }
    }
    
    /**
     * @brief Map character to grid row/column position
     * 
     * This implements the same key mapping as the real CursesInputLayer:
     * Row 0: 1 2 3 4 5 6 7 8  (numbers)
     * Row 1: q w e r t y u i  (QWERTY row)  
     * Row 2: a s d f g h j k  (home row)
     * Row 3: z x c v b n m ,  (bottom row)
     * 
     * Plus shifted versions: ! @ # $ % ^ & * and Q W E R... etc
     */
    bool getKeyMapping(char key, uint8_t& row, uint8_t& col) const {
        // Row 0: Numbers and shifted symbols
        switch (key) {
            case '1': case '!': row = 0; col = 0; return true;
            case '2': case '@': row = 0; col = 1; return true;
            case '3': case '#': row = 0; col = 2; return true;
            case '4': case '$': row = 0; col = 3; return true;
            case '5': case '%': row = 0; col = 4; return true;
            case '6': case '^': row = 0; col = 5; return true;
            case '7': case '&': row = 0; col = 6; return true;
            case '8': case '*': row = 0; col = 7; return true;
        }
        
        // Row 1: QWERTY row
        switch (key) {
            case 'q': case 'Q': row = 1; col = 0; return true;
            case 'w': case 'W': row = 1; col = 1; return true;
            case 'e': case 'E': row = 1; col = 2; return true;
            case 'r': case 'R': row = 1; col = 3; return true;
            case 't': case 'T': row = 1; col = 4; return true;
            case 'y': case 'Y': row = 1; col = 5; return true;
            case 'u': case 'U': row = 1; col = 6; return true;
            case 'i': case 'I': row = 1; col = 7; return true;
        }
        
        // Row 2: Home row
        switch (key) {
            case 'a': case 'A': row = 2; col = 0; return true;
            case 's': case 'S': row = 2; col = 1; return true;
            case 'd': case 'D': row = 2; col = 2; return true;
            case 'f': case 'F': row = 2; col = 3; return true;
            case 'g': case 'G': row = 2; col = 4; return true;
            case 'h': case 'H': row = 2; col = 5; return true;
            case 'j': case 'J': row = 2; col = 6; return true;
            case 'k': case 'K': row = 2; col = 7; return true;
        }
        
        // Row 3: Bottom row
        switch (key) {
            case 'z': case 'Z': row = 3; col = 0; return true;
            case 'x': case 'X': row = 3; col = 1; return true;
            case 'c': case 'C': row = 3; col = 2; return true;
            case 'v': case 'V': row = 3; col = 3; return true;
            case 'b': case 'B': row = 3; col = 4; return true;
            case 'n': case 'N': row = 3; col = 5; return true;
            case 'm': case 'M': row = 3; col = 6; return true;
            case ',': case '<': row = 3; col = 7; return true;
        }
        
        return false; // Key not mapped
    }
    
    /**
     * @brief Determine if key represents a PRESS (true) or RELEASE (false) action
     * 
     * Convention:
     * - Numbers (1-8): PRESS
     * - Shifted numbers (!@#$%^&*): RELEASE  
     * - Uppercase letters: PRESS
     * - Lowercase letters: RELEASE
     * - Special case: '<' (shift+comma) is PRESS, ',' is RELEASE
     */
    bool determineKeyAction(char key) const {
        // Numbers are press
        if (key >= '1' && key <= '8') {
            return true;
        }
        
        // Shifted numbers are release
        const char shiftedNumbers[] = "!@#$%^&*";
        for (char c : shiftedNumbers) {
            if (key == c) return false;
        }
        
        // Uppercase letters are press
        if (key >= 'A' && key <= 'Z') {
            return true;
        }
        
        // Lowercase letters are release
        if (key >= 'a' && key <= 'z') {
            return false;
        }
        
        // Special cases
        if (key == '<') return true;  // shift+comma is press
        if (key == ',') return false; // comma is release
        
        return false; // Default to release for unknown keys
    }
    
    /**
     * @brief Check if character is considered uppercase in our mapping
     */
    bool isUpperCase(char key) const {
        if (key >= 'A' && key <= 'Z') return true;
        if (key == '<') return true; // Special case for shift+comma
        return false;
    }
    
    /**
     * @brief Check if button at row,col is currently pressed
     * For this compatibility layer, just return false since we don't track real state
     */
    bool isButtonPressed(uint8_t row, uint8_t col) const {
        if (row >= ROWS || col >= COLS) return false;
        return buttonStates_[row * COLS + col];
    }
    
private:
    IClock* clock_;
    bool buttonStates_[ROWS * COLS]; // Simple state array for compatibility
};

#endif // CURSES_INPUT_LEGACY_H