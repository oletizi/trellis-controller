#ifndef CONTROL_GRID_H
#define CONTROL_GRID_H

#include <stdint.h>
#include "ParameterLockPool.h"

/**
 * Control grid management for parameter lock editing.
 * Maps the "opposite end" 4x4 grid to parameter controls when a step is held.
 * Supports ergonomic validation and hand preference adaptation.
 */
class ControlGrid {
public:
    /**
     * Hand preference for control placement
     */
    enum class HandPreference : uint8_t {
        RIGHT = 0,     // Right-handed user (default)
        LEFT = 1,      // Left-handed user
        AUTO = 2       // Auto-detect based on usage patterns
    };
    
    /**
     * Control button assignments within the 4x4 grid
     */
    struct ControlMapping {
        // Primary controls (always available)
        uint8_t noteUpButton;         // Increase note by semitone
        uint8_t noteDownButton;       // Decrease note by semitone
        uint8_t velocityUpButton;     // Increase velocity
        uint8_t velocityDownButton;   // Decrease velocity
        
        // Secondary controls (page-dependent)
        uint8_t lengthUpButton;       // Increase gate length
        uint8_t lengthDownButton;     // Decrease gate length
        uint8_t probabilityUpButton;  // Increase probability
        uint8_t probabilityDownButton;// Decrease probability
        
        // Navigation controls
        uint8_t pageUpButton;         // Next parameter page
        uint8_t pageDownButton;       // Previous parameter page
        uint8_t clearButton;          // Clear all locks for this step
        uint8_t copyButton;           // Copy parameters
        
        // Grid boundaries
        uint8_t controlAreaStart;     // First button in 4x4 control area (0-31)
        uint8_t controlAreaEnd;       // Last button in control area
        bool isValid;                 // Mapping is valid
        
        // Default constructor
        ControlMapping() 
            : noteUpButton(0xFF), noteDownButton(0xFF)
            , velocityUpButton(0xFF), velocityDownButton(0xFF)
            , lengthUpButton(0xFF), lengthDownButton(0xFF)
            , probabilityUpButton(0xFF), probabilityDownButton(0xFF)
            , pageUpButton(0xFF), pageDownButton(0xFF)
            , clearButton(0xFF), copyButton(0xFF)
            , controlAreaStart(0xFF), controlAreaEnd(0xFF)
            , isValid(false) {}
        
        // Check if button is within control area
        bool isInControlArea(uint8_t button) const {
            return isValid && button >= controlAreaStart && button <= controlAreaEnd;
        }
        
        // Get parameter type for button
        ParameterLockPool::ParameterType getParameterForButton(uint8_t button) const;
        
        // Check if button is an increment control
        bool isIncrementButton(uint8_t button) const {
            return button == noteUpButton || 
                   button == velocityUpButton || 
                   button == lengthUpButton || 
                   button == probabilityUpButton;
        }
        
        // Check if button is a decrement control
        bool isDecrementButton(uint8_t button) const {
            return button == noteDownButton || 
                   button == velocityDownButton || 
                   button == lengthDownButton || 
                   button == probabilityDownButton;
        }
    };
    
    /**
     * Ergonomic validation results
     */
    struct ErgonomicValidation {
        bool isValid;                 // Overall ergonomic validity
        bool handSpanOk;             // Hand span requirement met
        bool reachabilityOk;         // All controls reachable
        bool layoutLogical;          // Control layout makes sense
        float comfortScore;          // Comfort rating (0.0-1.0)
        const char* recommendation;  // Improvement recommendation
        
        ErgonomicValidation()
            : isValid(false), handSpanOk(false), reachabilityOk(false)
            , layoutLogical(false), comfortScore(0.0f)
            , recommendation(nullptr) {}
    };
    
    /**
     * Usage statistics for auto hand preference detection
     */
    struct UsageStats {
        uint32_t leftSideUsage;      // Usage count for left side buttons
        uint32_t rightSideUsage;     // Usage count for right side buttons
        uint32_t totalUsage;         // Total button usage
        HandPreference detectedPreference;
        float confidence;            // Detection confidence (0.0-1.0)
        
        UsageStats()
            : leftSideUsage(0), rightSideUsage(0), totalUsage(0)
            , detectedPreference(HandPreference::AUTO), confidence(0.0f) {}
    };

public:
    /**
     * Constructor
     */
    ControlGrid();
    
    /**
     * Get control mapping for a held step
     * @param heldStep Step index being held (0-7)
     * @param heldTrack Track index of held step (0-3)
     * @return Control mapping for the opposite grid
     */
    ControlMapping getMapping(uint8_t heldStep, uint8_t heldTrack) const;
    
    /**
     * Check if button is in control grid for held step
     * @param button Button index (0-31)
     * @param heldStep Step being held (0-7)
     * @param heldTrack Track of held step (0-3)
     * @return true if button is in control grid
     */
    bool isInControlGrid(uint8_t button, uint8_t heldStep, uint8_t heldTrack) const;
    
    /**
     * Get parameter type for control button
     * @param button Button index (0-31)
     * @param mapping Current control mapping
     * @return Parameter type or NONE if not a control button
     */
    ParameterLockPool::ParameterType getParameterType(uint8_t button, const ControlMapping& mapping) const;
    
    /**
     * Get parameter adjustment for button press
     * @param button Button index (0-31)
     * @param mapping Current control mapping
     * @return Adjustment value (-1, 0, or +1)
     */
    int8_t getParameterAdjustment(uint8_t button, const ControlMapping& mapping) const;
    
    /**
     * Validate ergonomics of control placement
     * @param mapping Control mapping to validate
     * @return Ergonomic validation results
     */
    ErgonomicValidation validateErgonomics(const ControlMapping& mapping) const;
    
    /**
     * Set hand preference
     * @param preference Hand preference setting
     */
    void setHandPreference(HandPreference preference) { handPreference_ = preference; }
    
    /**
     * Get current hand preference
     * @return Current hand preference
     */
    HandPreference getHandPreference() const { return handPreference_; }
    
    /**
     * Record button usage for auto-detection
     * @param button Button that was used
     */
    void recordButtonUsage(uint8_t button);
    
    /**
     * Detect dominant hand from usage patterns
     * @return Detected hand preference
     */
    HandPreference detectDominantHand() const;
    
    /**
     * Get usage statistics
     * @return Usage statistics structure
     */
    const UsageStats& getUsageStats() const { return usageStats_; }
    
    /**
     * Reset usage statistics
     */
    void resetUsageStats();
    
    /**
     * Get visual indicator colors for control buttons
     * @param mapping Control mapping
     * @param colors Array to fill with colors (32 elements)
     */
    void getControlColors(const ControlMapping& mapping, uint32_t* colors) const;
    
    /**
     * Get button description for debugging
     * @param button Button index
     * @param mapping Control mapping
     * @return Description string
     */
    const char* getButtonDescription(uint8_t button, const ControlMapping& mapping) const;
    
    /**
     * Calculate control grid position
     * @param heldStep Step being held (0-7)
     * @return Starting column for control grid (0 or 4)
     */
    static uint8_t calculateControlGridStart(uint8_t heldStep);
    
    /**
     * Convert button index to row/column
     * @param button Button index (0-31)
     * @param row Output row (0-3)
     * @param col Output column (0-7)
     */
    static void buttonToRowCol(uint8_t button, uint8_t& row, uint8_t& col);
    
    /**
     * Convert row/column to button index
     * @param row Row (0-3)
     * @param col Column (0-7)
     * @return Button index (0-31)
     */
    static uint8_t rowColToButton(uint8_t row, uint8_t col);

private:
    HandPreference handPreference_;      // User's hand preference
    UsageStats usageStats_;              // Button usage statistics
    mutable uint8_t usageHistory_[32];   // Recent usage per button
    
    // Color constants for visual feedback
    static constexpr uint32_t COLOR_NOTE_UP = 0x00FF00;      // Green
    static constexpr uint32_t COLOR_NOTE_DOWN = 0xFF0000;    // Red
    static constexpr uint32_t COLOR_VELOCITY = 0x0080FF;     // Cyan
    static constexpr uint32_t COLOR_LENGTH = 0xFFFF00;       // Yellow
    static constexpr uint32_t COLOR_PROBABILITY = 0xFF00FF;  // Magenta
    static constexpr uint32_t COLOR_PAGE = 0x808080;         // Gray
    static constexpr uint32_t COLOR_CLEAR = 0xFF8000;        // Orange
    static constexpr uint32_t COLOR_COPY = 0x00FFFF;         // Light cyan
    
    /**
     * Calculate control mapping for right-handed user
     * @param controlStart Starting column (0 or 4)
     * @return Control mapping
     */
    ControlMapping calculateRightHandedMapping(uint8_t controlStart) const;
    
    /**
     * Calculate control mapping for left-handed user
     * @param controlStart Starting column (0 or 4)
     * @return Control mapping
     */
    ControlMapping calculateLeftHandedMapping(uint8_t controlStart) const;
    
    /**
     * Calculate automatic mapping based on usage
     * @param controlStart Starting column (0 or 4)
     * @return Control mapping
     */
    ControlMapping calculateAutoMapping(uint8_t controlStart) const;
    
    /**
     * Validate button index
     * @param button Button index
     * @return true if valid (0-31)
     */
    bool isValidButton(uint8_t button) const {
        return button < 32;
    }
    
    /**
     * Calculate hand span distance between buttons
     * @param button1 First button
     * @param button2 Second button
     * @return Distance in button units
     */
    float calculateHandSpan(uint8_t button1, uint8_t button2) const;
    
    /**
     * Check if button layout is logical
     * @param mapping Control mapping to check
     * @return true if layout makes sense
     */
    bool isLayoutLogical(const ControlMapping& mapping) const;
};

#endif // CONTROL_GRID_H