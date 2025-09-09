#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/generators/catch_generators.hpp>
#include "ControlGrid.h"

using namespace Catch::Matchers;

TEST_CASE("ControlGrid - Initialization and basic functionality", "[control_grid]") {
    ControlGrid grid;
    
    SECTION("Initial state") {
        REQUIRE(grid.getHandPreference() == ControlGrid::HandPreference::RIGHT);
        
        const auto& stats = grid.getUsageStats();
        REQUIRE(stats.leftSideUsage == 0);
        REQUIRE(stats.rightSideUsage == 0);
        REQUIRE(stats.totalUsage == 0);
        REQUIRE(stats.detectedPreference == ControlGrid::HandPreference::AUTO);
        REQUIRE_THAT(stats.confidence, WithinAbs(0.0f, 0.001f));
    }
    
    SECTION("Hand preference settings") {
        grid.setHandPreference(ControlGrid::HandPreference::LEFT);
        REQUIRE(grid.getHandPreference() == ControlGrid::HandPreference::LEFT);
        
        grid.setHandPreference(ControlGrid::HandPreference::AUTO);
        REQUIRE(grid.getHandPreference() == ControlGrid::HandPreference::AUTO);
    }
}

TEST_CASE("ControlGrid - Basic coordinate conversions", "[control_grid][coordinates]") {
    SECTION("Button to row/column conversion") {
        struct TestCase {
            uint8_t button, expectedRow, expectedCol;
        };
        
        TestCase testCases[] = {
            {0, 0, 0},   // Top-left
            {7, 0, 7},   // Top-right
            {8, 1, 0},   // Second row, first column
            {15, 1, 7},  // Second row, last column
            {16, 2, 0},  // Third row, first column
            {23, 2, 7},  // Third row, last column
            {24, 3, 0},  // Bottom-left
            {31, 3, 7}   // Bottom-right
        };
        
        for (const auto& test : testCases) {
            uint8_t row, col;
            ControlGrid::buttonToRowCol(test.button, row, col);
            REQUIRE(row == test.expectedRow);
            REQUIRE(col == test.expectedCol);
        }
    }
    
    SECTION("Row/column to button conversion") {
        struct TestCase {
            uint8_t row, col, expectedButton;
        };
        
        TestCase testCases[] = {
            {0, 0, 0},   // Top-left
            {0, 7, 7},   // Top-right
            {1, 0, 8},   // Second row, first column
            {1, 7, 15},  // Second row, last column
            {2, 0, 16},  // Third row, first column
            {2, 7, 23},  // Third row, last column
            {3, 0, 24},  // Bottom-left
            {3, 7, 31}   // Bottom-right
        };
        
        for (const auto& test : testCases) {
            uint8_t button = ControlGrid::rowColToButton(test.row, test.col);
            REQUIRE(button == test.expectedButton);
        }
    }
    
    SECTION("Coordinate conversion round-trip") {
        for (uint8_t button = 0; button < 32; ++button) {
            uint8_t row, col;
            ControlGrid::buttonToRowCol(button, row, col);
            uint8_t reconstructed = ControlGrid::rowColToButton(row, col);
            REQUIRE(reconstructed == button);
        }
    }
}

TEST_CASE("ControlGrid - Control grid calculation", "[control_grid][mapping]") {
    ControlGrid grid;
    
    SECTION("Control grid start calculation") {
        // Left side steps (0-3) should use right side (columns 4-7) for control
        for (uint8_t step = 0; step < 4; ++step) {
            uint8_t start = ControlGrid::calculateControlGridStart(step);
            REQUIRE(start == 4);
        }
        
        // Right side steps (4-7) should use left side (columns 0-3) for control
        for (uint8_t step = 4; step < 8; ++step) {
            uint8_t start = ControlGrid::calculateControlGridStart(step);
            REQUIRE(start == 0);
        }
    }
    
    SECTION("Control mapping for left side held steps") {
        // Test step in left half (step 2)
        auto mapping = grid.getMapping(2, 1); // Step 2, Track 1
        
        REQUIRE(mapping.isValid);
        REQUIRE(mapping.controlAreaStart == 4);  // Should use right side
        REQUIRE(mapping.controlAreaEnd >= mapping.controlAreaStart);
        
        // Verify control buttons are in expected area
        REQUIRE(mapping.isInControlArea(4));   // First control column
        REQUIRE(mapping.isInControlArea(7));   // Last control column (top row)
        REQUIRE(mapping.isInControlArea(28));  // Bottom row, first control
        REQUIRE(mapping.isInControlArea(31));  // Bottom-right
        
        // These should NOT be in control area
        REQUIRE_FALSE(mapping.isInControlArea(0));  // Held step area
        REQUIRE_FALSE(mapping.isInControlArea(3));  // Held step area
    }
    
    SECTION("Control mapping for right side held steps") {
        // Test step in right half (step 5)
        auto mapping = grid.getMapping(5, 2); // Step 5, Track 2
        
        REQUIRE(mapping.isValid);
        REQUIRE(mapping.controlAreaStart == 0);  // Should use left side
        REQUIRE(mapping.controlAreaEnd >= mapping.controlAreaStart);
        
        // Verify control buttons are in expected area
        REQUIRE(mapping.isInControlArea(0));   // First control column
        REQUIRE(mapping.isInControlArea(3));   // Last control column (top row)
        REQUIRE(mapping.isInControlArea(24));  // Bottom row, first control
        REQUIRE(mapping.isInControlArea(27));  // Bottom-left control area
        
        // These should NOT be in control area
        REQUIRE_FALSE(mapping.isInControlArea(4));  // Held step area
        REQUIRE_FALSE(mapping.isInControlArea(7));  // Held step area
    }
}

TEST_CASE("ControlGrid - Control button detection", "[control_grid][detection]") {
    ControlGrid grid;
    
    SECTION("isInControlGrid method") {
        // Test with step 1 (left side) - control grid should be on right (columns 4-7)
        for (uint8_t button = 0; button < 32; ++button) {
            bool expected = (button % 8) >= 4;  // Columns 4-7
            bool actual = grid.isInControlGrid(button, 1, 0);
            REQUIRE(actual == expected);
        }
        
        // Test with step 6 (right side) - control grid should be on left (columns 0-3)
        for (uint8_t button = 0; button < 32; ++button) {
            bool expected = (button % 8) < 4;   // Columns 0-3
            bool actual = grid.isInControlGrid(button, 6, 0);
            REQUIRE(actual == expected);
        }
    }
    
    SECTION("All track/step combinations") {
        for (uint8_t track = 0; track < 4; ++track) {
            for (uint8_t step = 0; step < 8; ++step) {
                auto mapping = grid.getMapping(step, track);
                REQUIRE(mapping.isValid);
                
                // Held button should NOT be in control grid
                uint8_t heldButton = track * 8 + step;
                REQUIRE_FALSE(mapping.isInControlArea(heldButton));
                REQUIRE_FALSE(grid.isInControlGrid(heldButton, step, track));
                
                // At least some buttons should be in control grid
                bool foundControlButton = false;
                for (uint8_t button = 0; button < 32; ++button) {
                    if (grid.isInControlGrid(button, step, track)) {
                        foundControlButton = true;
                        REQUIRE(mapping.isInControlArea(button));
                    }
                }
                REQUIRE(foundControlButton);
            }
        }
    }
}

TEST_CASE("ControlGrid - Parameter mapping", "[control_grid][parameters]") {
    ControlGrid grid;
    
    SECTION("Parameter type detection") {
        auto mapping = grid.getMapping(0, 0); // Step 0, Track 0
        
        // Test that buttons have parameter types assigned
        bool foundNoteUp = false, foundNoteDown = false;
        bool foundVelocityUp = false, foundVelocityDown = false;
        
        for (uint8_t button = 0; button < 32; ++button) {
            if (mapping.isInControlArea(button)) {
                auto paramType = grid.getParameterType(button, mapping);
                
                if (button == mapping.noteUpButton) {
                    REQUIRE(paramType == ParameterLockPool::ParameterType::NOTE);
                    foundNoteUp = true;
                }
                if (button == mapping.noteDownButton) {
                    REQUIRE(paramType == ParameterLockPool::ParameterType::NOTE);
                    foundNoteDown = true;
                }
                if (button == mapping.velocityUpButton) {
                    REQUIRE(paramType == ParameterLockPool::ParameterType::VELOCITY);
                    foundVelocityUp = true;
                }
                if (button == mapping.velocityDownButton) {
                    REQUIRE(paramType == ParameterLockPool::ParameterType::VELOCITY);
                    foundVelocityDown = true;
                }
            }
        }
        
        // Should have found primary controls
        REQUIRE(foundNoteUp);
        REQUIRE(foundNoteDown);
        REQUIRE(foundVelocityUp);
        REQUIRE(foundVelocityDown);
    }
    
    SECTION("Parameter adjustment values") {
        auto mapping = grid.getMapping(3, 1); // Step 3, Track 1
        
        // Test increment buttons
        if (mapping.noteUpButton != 0xFF) {
            int8_t adjustment = grid.getParameterAdjustment(mapping.noteUpButton, mapping);
            REQUIRE(adjustment > 0); // Should be positive increment
        }
        
        if (mapping.velocityUpButton != 0xFF) {
            int8_t adjustment = grid.getParameterAdjustment(mapping.velocityUpButton, mapping);
            REQUIRE(adjustment > 0); // Should be positive increment
        }
        
        // Test decrement buttons
        if (mapping.noteDownButton != 0xFF) {
            int8_t adjustment = grid.getParameterAdjustment(mapping.noteDownButton, mapping);
            REQUIRE(adjustment < 0); // Should be negative increment
        }
        
        if (mapping.velocityDownButton != 0xFF) {
            int8_t adjustment = grid.getParameterAdjustment(mapping.velocityDownButton, mapping);
            REQUIRE(adjustment < 0); // Should be negative increment
        }
        
        // Test non-control buttons
        for (uint8_t button = 0; button < 32; ++button) {
            if (!mapping.isInControlArea(button)) {
                int8_t adjustment = grid.getParameterAdjustment(button, mapping);
                REQUIRE(adjustment == 0); // Non-control buttons should have no adjustment
            }
        }
    }
    
    SECTION("Increment/decrement button classification") {
        auto mapping = grid.getMapping(7, 3); // Step 7, Track 3
        
        // Check increment/decrement classification
        if (mapping.noteUpButton != 0xFF) {
            REQUIRE(mapping.isIncrementButton(mapping.noteUpButton));
            REQUIRE_FALSE(mapping.isDecrementButton(mapping.noteUpButton));
        }
        
        if (mapping.noteDownButton != 0xFF) {
            REQUIRE_FALSE(mapping.isIncrementButton(mapping.noteDownButton));
            REQUIRE(mapping.isDecrementButton(mapping.noteDownButton));
        }
        
        if (mapping.velocityUpButton != 0xFF) {
            REQUIRE(mapping.isIncrementButton(mapping.velocityUpButton));
            REQUIRE_FALSE(mapping.isDecrementButton(mapping.velocityUpButton));
        }
        
        if (mapping.velocityDownButton != 0xFF) {
            REQUIRE_FALSE(mapping.isIncrementButton(mapping.velocityDownButton));
            REQUIRE(mapping.isDecrementButton(mapping.velocityDownButton));
        }
    }
}

TEST_CASE("ControlGrid - Ergonomic validation", "[control_grid][ergonomics]") {
    ControlGrid grid;
    
    SECTION("Valid mapping ergonomics") {
        auto mapping = grid.getMapping(1, 0); // Step 1, Track 0
        auto validation = grid.validateErgonomics(mapping);
        
        // Basic validation should pass for generated mappings
        REQUIRE(validation.isValid);
        REQUIRE(validation.comfortScore >= 0.0f);
        REQUIRE(validation.comfortScore <= 1.0f);
    }
    
    SECTION("All mappings should be ergonomically valid") {
        for (uint8_t track = 0; track < 4; ++track) {
            for (uint8_t step = 0; step < 8; ++step) {
                auto mapping = grid.getMapping(step, track);
                auto validation = grid.validateErgonomics(mapping);
                
                // Generated mappings should be ergonomically sound
                REQUIRE(validation.isValid);
                REQUIRE(validation.comfortScore > 0.0f);
                
                if (!validation.isValid) {
                    // If validation fails, there should be a recommendation
                    REQUIRE(validation.recommendation != nullptr);
                }
            }
        }
    }
}

TEST_CASE("ControlGrid - Usage tracking and hand preference detection", "[control_grid][usage]") {
    ControlGrid grid;
    
    SECTION("Button usage recording") {
        auto initialStats = grid.getUsageStats();
        REQUIRE(initialStats.totalUsage == 0);
        
        // Record some button usage
        grid.recordButtonUsage(0);  // Left side
        grid.recordButtonUsage(1);  // Left side
        grid.recordButtonUsage(4);  // Right side
        
        auto updatedStats = grid.getUsageStats();
        REQUIRE(updatedStats.totalUsage == 3);
        REQUIRE(updatedStats.leftSideUsage == 2);
        REQUIRE(updatedStats.rightSideUsage == 1);
    }
    
    SECTION("Hand preference detection") {
        // Record predominantly right-side usage
        for (int i = 0; i < 20; ++i) {
            grid.recordButtonUsage(4 + (i % 4)); // Right side buttons (4-7)
        }
        
        // Record some left-side usage
        for (int i = 0; i < 5; ++i) {
            grid.recordButtonUsage(i % 4); // Left side buttons (0-3)
        }
        
        auto detectedPreference = grid.detectDominantHand();
        auto stats = grid.getUsageStats();
        
        // Should detect right-hand preference
        REQUIRE(stats.rightSideUsage > stats.leftSideUsage);
        // Detection algorithm should reflect this
    }
    
    SECTION("Reset usage statistics") {
        // Record some usage
        grid.recordButtonUsage(0);
        grid.recordButtonUsage(4);
        
        REQUIRE(grid.getUsageStats().totalUsage == 2);
        
        // Reset
        grid.resetUsageStats();
        
        auto stats = grid.getUsageStats();
        REQUIRE(stats.totalUsage == 0);
        REQUIRE(stats.leftSideUsage == 0);
        REQUIRE(stats.rightSideUsage == 0);
    }
}

TEST_CASE("ControlGrid - Hand preference influence on mapping", "[control_grid][hand_preference]") {
    ControlGrid grid;
    
    SECTION("Right-handed mapping") {
        grid.setHandPreference(ControlGrid::HandPreference::RIGHT);
        
        auto mapping = grid.getMapping(0, 0); // Step 0, Track 0
        REQUIRE(mapping.isValid);
        
        // Right-handed users might prefer certain button arrangements
        // Specific requirements depend on implementation
    }
    
    SECTION("Left-handed mapping") {
        grid.setHandPreference(ControlGrid::HandPreference::LEFT);
        
        auto mapping = grid.getMapping(0, 0); // Step 0, Track 0
        REQUIRE(mapping.isValid);
        
        // Left-handed mapping might differ from right-handed
        // Implementation-specific behavior
    }
    
    SECTION("Auto preference mapping") {
        grid.setHandPreference(ControlGrid::HandPreference::AUTO);
        
        // Record usage pattern for right-handed behavior
        for (int i = 0; i < 50; ++i) {
            grid.recordButtonUsage(4 + (i % 28)); // Favor right side
        }
        
        auto mapping = grid.getMapping(0, 0);
        REQUIRE(mapping.isValid);
        
        // Auto mapping should adapt to usage patterns
    }
}

TEST_CASE("ControlGrid - Visual feedback", "[control_grid][visual]") {
    ControlGrid grid;
    
    SECTION("Control colors generation") {
        auto mapping = grid.getMapping(2, 1); // Step 2, Track 1
        uint32_t colors[32] = {0};
        
        grid.getControlColors(mapping, colors);
        
        // Control buttons should have non-zero colors
        bool foundColoredButton = false;
        for (uint8_t button = 0; button < 32; ++button) {
            if (mapping.isInControlArea(button)) {
                REQUIRE(colors[button] != 0); // Should have a color assigned
                foundColoredButton = true;
            } else {
                // Non-control buttons might have default colors or be uncolored
                // Implementation-specific
            }
        }
        
        REQUIRE(foundColoredButton); // Should have found at least one colored control
    }
    
    SECTION("Button descriptions") {
        auto mapping = grid.getMapping(5, 2); // Step 5, Track 2
        
        // Test descriptions for control buttons
        for (uint8_t button = 0; button < 32; ++button) {
            const char* description = grid.getButtonDescription(button, mapping);
            REQUIRE(description != nullptr); // Should always return a description
            
            if (mapping.isInControlArea(button)) {
                // Control buttons should have meaningful descriptions
                // (specific strings depend on implementation)
                REQUIRE(strlen(description) > 0);
            }
        }
    }
}

TEST_CASE("ControlGrid - Edge cases and error handling", "[control_grid][edge_cases]") {
    ControlGrid grid;
    
    SECTION("Invalid button indices") {
        auto mapping = grid.getMapping(0, 0);
        
        // Test operations with invalid buttons
        REQUIRE_FALSE(mapping.isInControlArea(32));
        REQUIRE_FALSE(mapping.isInControlArea(255));
        
        auto paramType = grid.getParameterType(255, mapping);
        REQUIRE(paramType == ParameterLockPool::ParameterType::NONE);
        
        int8_t adjustment = grid.getParameterAdjustment(255, mapping);
        REQUIRE(adjustment == 0);
    }
    
    SECTION("Invalid step/track combinations") {
        // Test with invalid parameters - should handle gracefully
        auto mapping1 = grid.getMapping(8, 0);   // Invalid step
        auto mapping2 = grid.getMapping(0, 4);   // Invalid track
        auto mapping3 = grid.getMapping(255, 255); // Both invalid
        
        // Mappings might be invalid or have default behavior
        // Should not crash or cause undefined behavior
    }
    
    SECTION("Empty/default mapping validation") {
        ControlGrid::ControlMapping emptyMapping; // Default constructor
        REQUIRE_FALSE(emptyMapping.isValid);
        
        auto validation = grid.validateErgonomics(emptyMapping);
        REQUIRE_FALSE(validation.isValid);
        REQUIRE(validation.comfortScore == 0.0f);
    }
}

// Parametric tests for all valid positions
TEST_CASE("ControlGrid - All positions mapping validation", "[control_grid][parametric]") {
    ControlGrid grid;
    
    auto track = GENERATE(values({0, 1, 2, 3}));
    auto step = GENERATE(values({0, 1, 2, 3, 4, 5, 6, 7}));
    
    SECTION("Track " + std::to_string(track) + ", Step " + std::to_string(step)) {
        auto mapping = grid.getMapping(step, track);
        
        REQUIRE(mapping.isValid);
        REQUIRE(mapping.controlAreaStart < 32);
        REQUIRE(mapping.controlAreaEnd < 32);
        REQUIRE(mapping.controlAreaStart <= mapping.controlAreaEnd);
        
        // Held button should not be in control area
        uint8_t heldButton = track * 8 + step;
        REQUIRE_FALSE(mapping.isInControlArea(heldButton));
        
        // Should have some control buttons defined
        REQUIRE((mapping.noteUpButton != 0xFF || mapping.noteDownButton != 0xFF));
        
        // Ergonomics should be valid
        auto validation = grid.validateErgonomics(mapping);
        REQUIRE(validation.isValid);
    }
}

TEST_CASE("ControlGrid - Comprehensive workflow test", "[control_grid][workflow]") {
    ControlGrid grid;
    
    SECTION("Complete parameter lock control workflow") {
        // Simulate parameter lock mode for step 3, track 1
        uint8_t heldStep = 3, heldTrack = 1;
        auto mapping = grid.getMapping(heldStep, heldTrack);
        
        REQUIRE(mapping.isValid);
        
        // Get visual colors for display
        uint32_t colors[32];
        grid.getControlColors(mapping, colors);
        
        // Simulate some control button presses
        std::vector<uint8_t> controlButtons;
        for (uint8_t button = 0; button < 32; ++button) {
            if (mapping.isInControlArea(button)) {
                controlButtons.push_back(button);
            }
        }
        
        REQUIRE(!controlButtons.empty());
        
        // Test parameter adjustments for each control button
        for (uint8_t button : controlButtons) {
            auto paramType = grid.getParameterType(button, mapping);
            int8_t adjustment = grid.getParameterAdjustment(button, mapping);
            
            // Record usage for hand preference learning
            grid.recordButtonUsage(button);
            
            // Button should have a meaningful function
            REQUIRE((paramType != ParameterLockPool::ParameterType::NONE || 
                    button == mapping.clearButton || 
                    button == mapping.copyButton ||
                    button == mapping.pageUpButton ||
                    button == mapping.pageDownButton));
        }
        
        // Check that usage was recorded
        REQUIRE(grid.getUsageStats().totalUsage > 0);
        
        // Validate final state
        auto finalValidation = grid.validateErgonomics(mapping);
        REQUIRE(finalValidation.isValid);
    }
}