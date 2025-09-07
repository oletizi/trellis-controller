#include "ControlGrid.h"
#include <cmath>
#include <algorithm>

ControlGrid::ControlGrid()
    : handPreference_(HandPreference::AUTO)
    , usageStats_() {
    
    // Initialize usage history
    for (uint8_t i = 0; i < 32; ++i) {
        usageHistory_[i] = 0;
    }
}

ControlGrid::ControlMapping ControlGrid::getMapping(uint8_t heldStep, uint8_t heldTrack) const {
    // Validate inputs
    if (heldStep >= 8 || heldTrack >= 4) {
        return ControlMapping();  // Invalid mapping
    }
    
    // Calculate control grid start position
    uint8_t controlStart = calculateControlGridStart(heldStep);
    
    // Generate mapping based on hand preference
    ControlMapping mapping;
    
    switch (handPreference_) {
        case HandPreference::RIGHT:
            mapping = calculateRightHandedMapping(controlStart);
            break;
        case HandPreference::LEFT:
            mapping = calculateLeftHandedMapping(controlStart);
            break;
        case HandPreference::AUTO:
            mapping = calculateAutoMapping(controlStart);
            break;
    }
    
    return mapping;
}

bool ControlGrid::isInControlGrid(uint8_t button, uint8_t heldStep, uint8_t heldTrack) const {
    if (!isValidButton(button) || heldStep >= 8 || heldTrack >= 4) {
        return false;
    }
    
    ControlMapping mapping = getMapping(heldStep, heldTrack);
    return mapping.isInControlArea(button);
}

ParameterLockPool::ParameterType ControlGrid::getParameterType(uint8_t button, const ControlMapping& mapping) const {
    if (!mapping.isValid || !isValidButton(button)) {
        return ParameterLockPool::ParameterType::NONE;
    }
    
    // Check which parameter this button controls
    if (button == mapping.noteUpButton || button == mapping.noteDownButton) {
        return ParameterLockPool::ParameterType::NOTE;
    }
    if (button == mapping.velocityUpButton || button == mapping.velocityDownButton) {
        return ParameterLockPool::ParameterType::VELOCITY;
    }
    if (button == mapping.lengthUpButton || button == mapping.lengthDownButton) {
        return ParameterLockPool::ParameterType::LENGTH;
    }
    if (button == mapping.probabilityUpButton || button == mapping.probabilityDownButton) {
        return ParameterLockPool::ParameterType::PROBABILITY;
    }
    
    return ParameterLockPool::ParameterType::NONE;
}

int8_t ControlGrid::getParameterAdjustment(uint8_t button, const ControlMapping& mapping) const {
    if (!mapping.isValid || !isValidButton(button)) {
        return 0;
    }
    
    if (mapping.isIncrementButton(button)) {
        return 1;
    }
    if (mapping.isDecrementButton(button)) {
        return -1;
    }
    
    return 0;
}

ControlGrid::ErgonomicValidation ControlGrid::validateErgonomics(const ControlMapping& mapping) const {
    ErgonomicValidation validation;
    
    if (!mapping.isValid) {
        validation.recommendation = "Invalid mapping";
        return validation;
    }
    
    // Check hand span requirements
    float maxSpan = 0.0f;
    
    // Calculate span between note controls
    if (mapping.noteUpButton != 0xFF && mapping.noteDownButton != 0xFF) {
        float span = calculateHandSpan(mapping.noteUpButton, mapping.noteDownButton);
        maxSpan = std::max(maxSpan, span);
    }
    
    // Check if span is comfortable (max 4 button units for comfort)
    validation.handSpanOk = maxSpan <= 4.0f;
    
    // Check reachability - all controls should be within the 4x4 grid
    validation.reachabilityOk = true;  // By design, all controls are in 4x4 grid
    
    // Check logical layout
    validation.layoutLogical = isLayoutLogical(mapping);
    
    // Calculate comfort score
    float spanScore = (maxSpan <= 3.0f) ? 1.0f : (maxSpan <= 4.0f) ? 0.7f : 0.3f;
    float layoutScore = validation.layoutLogical ? 1.0f : 0.5f;
    validation.comfortScore = (spanScore + layoutScore) / 2.0f;
    
    // Overall validity
    validation.isValid = validation.handSpanOk && validation.reachabilityOk && validation.layoutLogical;
    
    // Recommendations
    if (!validation.handSpanOk) {
        validation.recommendation = "Controls too far apart - consider different hand preference";
    } else if (!validation.layoutLogical) {
        validation.recommendation = "Control layout could be more intuitive";
    } else {
        validation.recommendation = "Control layout is ergonomic";
    }
    
    return validation;
}

void ControlGrid::recordButtonUsage(uint8_t button) {
    if (!isValidButton(button)) return;
    
    // Update usage history
    if (usageHistory_[button] < 255) {
        ++usageHistory_[button];
    }
    
    // Update statistics
    uint8_t row, col;
    buttonToRowCol(button, row, col);
    
    if (col < 4) {
        ++usageStats_.leftSideUsage;
    } else {
        ++usageStats_.rightSideUsage;
    }
    
    ++usageStats_.totalUsage;
    
    // Update detected preference
    if (usageStats_.totalUsage >= 20) {  // Need minimum samples
        usageStats_.detectedPreference = detectDominantHand();
        
        // Calculate confidence
        float leftRatio = static_cast<float>(usageStats_.leftSideUsage) / usageStats_.totalUsage;
        float rightRatio = static_cast<float>(usageStats_.rightSideUsage) / usageStats_.totalUsage;
        usageStats_.confidence = std::abs(leftRatio - rightRatio);
    }
}

ControlGrid::HandPreference ControlGrid::detectDominantHand() const {
    if (usageStats_.totalUsage < 20) {
        return HandPreference::AUTO;
    }
    
    float leftRatio = static_cast<float>(usageStats_.leftSideUsage) / usageStats_.totalUsage;
    
    if (leftRatio > 0.6f) {
        return HandPreference::LEFT;
    } else if (leftRatio < 0.4f) {
        return HandPreference::RIGHT;
    }
    
    return HandPreference::AUTO;
}

void ControlGrid::resetUsageStats() {
    usageStats_ = UsageStats();
    
    for (uint8_t i = 0; i < 32; ++i) {
        usageHistory_[i] = 0;
    }
}

void ControlGrid::getControlColors(const ControlMapping& mapping, uint32_t* colors) const {
    if (!colors || !mapping.isValid) return;
    
    // Initialize all to black
    for (uint8_t i = 0; i < 32; ++i) {
        colors[i] = 0;
    }
    
    // Set colors for control buttons
    if (mapping.noteUpButton < 32) colors[mapping.noteUpButton] = COLOR_NOTE_UP;
    if (mapping.noteDownButton < 32) colors[mapping.noteDownButton] = COLOR_NOTE_DOWN;
    if (mapping.velocityUpButton < 32) colors[mapping.velocityUpButton] = COLOR_VELOCITY;
    if (mapping.velocityDownButton < 32) colors[mapping.velocityDownButton] = COLOR_VELOCITY;
    if (mapping.lengthUpButton < 32) colors[mapping.lengthUpButton] = COLOR_LENGTH;
    if (mapping.lengthDownButton < 32) colors[mapping.lengthDownButton] = COLOR_LENGTH;
    if (mapping.probabilityUpButton < 32) colors[mapping.probabilityUpButton] = COLOR_PROBABILITY;
    if (mapping.probabilityDownButton < 32) colors[mapping.probabilityDownButton] = COLOR_PROBABILITY;
    if (mapping.pageUpButton < 32) colors[mapping.pageUpButton] = COLOR_PAGE;
    if (mapping.pageDownButton < 32) colors[mapping.pageDownButton] = COLOR_PAGE;
    if (mapping.clearButton < 32) colors[mapping.clearButton] = COLOR_CLEAR;
    if (mapping.copyButton < 32) colors[mapping.copyButton] = COLOR_COPY;
}

const char* ControlGrid::getButtonDescription(uint8_t button, const ControlMapping& mapping) const {
    if (!mapping.isValid || !isValidButton(button)) {
        return "Invalid";
    }
    
    if (button == mapping.noteUpButton) return "Note Up";
    if (button == mapping.noteDownButton) return "Note Down";
    if (button == mapping.velocityUpButton) return "Velocity Up";
    if (button == mapping.velocityDownButton) return "Velocity Down";
    if (button == mapping.lengthUpButton) return "Length Up";
    if (button == mapping.lengthDownButton) return "Length Down";
    if (button == mapping.probabilityUpButton) return "Probability Up";
    if (button == mapping.probabilityDownButton) return "Probability Down";
    if (button == mapping.pageUpButton) return "Next Page";
    if (button == mapping.pageDownButton) return "Previous Page";
    if (button == mapping.clearButton) return "Clear Locks";
    if (button == mapping.copyButton) return "Copy Parameters";
    
    return "Unused";
}

uint8_t ControlGrid::calculateControlGridStart(uint8_t heldStep) {
    // If held step is in columns 0-3, control grid is columns 4-7
    // If held step is in columns 4-7, control grid is columns 0-3
    return (heldStep < 4) ? 4 : 0;
}

void ControlGrid::buttonToRowCol(uint8_t button, uint8_t& row, uint8_t& col) {
    if (button < 32) {
        row = button / 8;
        col = button % 8;
    } else {
        row = 0xFF;
        col = 0xFF;
    }
}

uint8_t ControlGrid::rowColToButton(uint8_t row, uint8_t col) {
    if (row < 4 && col < 8) {
        return row * 8 + col;
    }
    return 0xFF;
}

ControlGrid::ControlMapping ControlGrid::calculateRightHandedMapping(uint8_t controlStart) const {
    ControlMapping mapping;
    
    // Set control area boundaries
    mapping.controlAreaStart = controlStart;
    mapping.controlAreaEnd = controlStart + 3 + 24;  // 4x4 grid
    
    // Right-handed layout - primary controls on right side of control grid
    // Bottom row (row 3) for note controls - most important
    mapping.noteDownButton = rowColToButton(3, controlStart + 0);  // D1 or D5
    mapping.noteUpButton = rowColToButton(2, controlStart + 0);     // C1 or C5
    
    // Velocity on right side
    mapping.velocityDownButton = rowColToButton(3, controlStart + 3);  // D4 or D8
    mapping.velocityUpButton = rowColToButton(2, controlStart + 3);     // C4 or C8
    
    // Length in middle
    mapping.lengthDownButton = rowColToButton(3, controlStart + 1);    // D2 or D6
    mapping.lengthUpButton = rowColToButton(2, controlStart + 1);       // C2 or C6
    
    // Probability
    mapping.probabilityDownButton = rowColToButton(3, controlStart + 2); // D3 or D7
    mapping.probabilityUpButton = rowColToButton(2, controlStart + 2);   // C3 or C7
    
    // Top row for utility controls
    mapping.pageDownButton = rowColToButton(0, controlStart + 0);       // A1 or A5
    mapping.pageUpButton = rowColToButton(0, controlStart + 1);         // A2 or A6
    mapping.clearButton = rowColToButton(0, controlStart + 2);          // A3 or A7
    mapping.copyButton = rowColToButton(0, controlStart + 3);           // A4 or A8
    
    mapping.isValid = true;
    return mapping;
}

ControlGrid::ControlMapping ControlGrid::calculateLeftHandedMapping(uint8_t controlStart) const {
    ControlMapping mapping;
    
    // Set control area boundaries
    mapping.controlAreaStart = controlStart;
    mapping.controlAreaEnd = controlStart + 3 + 24;  // 4x4 grid
    
    // Left-handed layout - primary controls on left side of control grid
    // Bottom row (row 3) for note controls - most important
    mapping.noteDownButton = rowColToButton(3, controlStart + 3);   // D4 or D8
    mapping.noteUpButton = rowColToButton(2, controlStart + 3);      // C4 or C8
    
    // Velocity on left side
    mapping.velocityDownButton = rowColToButton(3, controlStart + 0); // D1 or D5
    mapping.velocityUpButton = rowColToButton(2, controlStart + 0);   // C1 or C5
    
    // Length in middle
    mapping.lengthDownButton = rowColToButton(3, controlStart + 2);   // D3 or D7
    mapping.lengthUpButton = rowColToButton(2, controlStart + 2);      // C3 or C7
    
    // Probability
    mapping.probabilityDownButton = rowColToButton(3, controlStart + 1); // D2 or D6
    mapping.probabilityUpButton = rowColToButton(2, controlStart + 1);   // C2 or C6
    
    // Top row for utility controls (mirrored)
    mapping.pageDownButton = rowColToButton(0, controlStart + 3);      // A4 or A8
    mapping.pageUpButton = rowColToButton(0, controlStart + 2);        // A3 or A7
    mapping.clearButton = rowColToButton(0, controlStart + 1);         // A2 or A6
    mapping.copyButton = rowColToButton(0, controlStart + 0);          // A1 or A5
    
    mapping.isValid = true;
    return mapping;
}

ControlGrid::ControlMapping ControlGrid::calculateAutoMapping(uint8_t controlStart) const {
    // Use detected preference or default to right-handed
    HandPreference detectedPref = detectDominantHand();
    
    if (detectedPref == HandPreference::LEFT) {
        return calculateLeftHandedMapping(controlStart);
    }
    
    // Default to right-handed
    return calculateRightHandedMapping(controlStart);
}

float ControlGrid::calculateHandSpan(uint8_t button1, uint8_t button2) const {
    if (!isValidButton(button1) || !isValidButton(button2)) {
        return 999.0f;  // Invalid span
    }
    
    uint8_t row1, col1, row2, col2;
    buttonToRowCol(button1, row1, col1);
    buttonToRowCol(button2, row2, col2);
    
    // Calculate Euclidean distance
    float rowDiff = static_cast<float>(static_cast<int>(row2) - static_cast<int>(row1));
    float colDiff = static_cast<float>(static_cast<int>(col2) - static_cast<int>(col1));
    
    return std::sqrt(rowDiff * rowDiff + colDiff * colDiff);
}

bool ControlGrid::isLayoutLogical(const ControlMapping& mapping) const {
    if (!mapping.isValid) return false;
    
    // Check that up/down pairs are vertically aligned
    uint8_t noteUpRow, noteUpCol, noteDownRow, noteDownCol;
    buttonToRowCol(mapping.noteUpButton, noteUpRow, noteUpCol);
    buttonToRowCol(mapping.noteDownButton, noteDownRow, noteDownCol);
    
    // Up should be above down (lower row number)
    if (noteUpRow >= noteDownRow) return false;
    
    // Should be in same column
    if (noteUpCol != noteDownCol) return false;
    
    // Similar checks for velocity
    uint8_t velUpRow, velUpCol, velDownRow, velDownCol;
    buttonToRowCol(mapping.velocityUpButton, velUpRow, velUpCol);
    buttonToRowCol(mapping.velocityDownButton, velDownRow, velDownCol);
    
    if (velUpRow >= velDownRow) return false;
    if (velUpCol != velDownCol) return false;
    
    return true;
}

ParameterLockPool::ParameterType ControlGrid::ControlMapping::getParameterForButton(uint8_t button) const {
    if (!isValid) {
        return ParameterLockPool::ParameterType::NONE;
    }
    
    if (button == noteUpButton || button == noteDownButton) {
        return ParameterLockPool::ParameterType::NOTE;
    }
    if (button == velocityUpButton || button == velocityDownButton) {
        return ParameterLockPool::ParameterType::VELOCITY;
    }
    if (button == lengthUpButton || button == lengthDownButton) {
        return ParameterLockPool::ParameterType::LENGTH;
    }
    if (button == probabilityUpButton || button == probabilityDownButton) {
        return ParameterLockPool::ParameterType::PROBABILITY;
    }
    
    return ParameterLockPool::ParameterType::NONE;
}