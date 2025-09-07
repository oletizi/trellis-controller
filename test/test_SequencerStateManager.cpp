#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "SequencerStateManager.h"

using namespace Catch::Matchers;

TEST_CASE("SequencerStateManager - Initialization and basic state", "[sequencer_state_manager]") {
    SequencerStateManager stateManager;
    
    SECTION("Initial state should be NORMAL") {
        REQUIRE(stateManager.getCurrentMode() == SequencerStateManager::Mode::NORMAL);
        REQUIRE(stateManager.getPreviousMode() == SequencerStateManager::Mode::NORMAL);
        REQUIRE_FALSE(stateManager.isInParameterLockMode());
    }
    
    SECTION("Parameter lock context should be inactive initially") {
        const auto& context = stateManager.getParameterLockContext();
        REQUIRE_FALSE(context.active);
        REQUIRE(context.heldStep == 0xFF);
        REQUIRE(context.heldTrack == 0xFF);
        REQUIRE(context.holdStartTime == 0);
        REQUIRE(context.previousMode == SequencerStateManager::Mode::NORMAL);
    }
    
    SECTION("Validate initial state") {
        REQUIRE(stateManager.validateState());
    }
}

TEST_CASE("SequencerStateManager - Parameter lock mode transitions", "[sequencer_state_manager][transitions]") {
    SequencerStateManager stateManager;
    
    SECTION("Enter parameter lock mode successfully") {
        uint8_t track = 1;
        uint8_t step = 3;
        
        auto result = stateManager.enterParameterLockMode(track, step);
        REQUIRE(result == SequencerStateManager::TransitionResult::SUCCESS);
        REQUIRE(stateManager.getCurrentMode() == SequencerStateManager::Mode::PARAMETER_LOCK);
        REQUIRE(stateManager.getPreviousMode() == SequencerStateManager::Mode::NORMAL);
        REQUIRE(stateManager.isInParameterLockMode());
        
        const auto& context = stateManager.getParameterLockContext();
        REQUIRE(context.active);
        REQUIRE(context.heldStep == step);
        REQUIRE(context.heldTrack == track);
        REQUIRE(context.getHeldButtonIndex() == track * 8 + step);
        REQUIRE(context.isValid());
    }
    
    SECTION("Exit parameter lock mode successfully") {
        // First enter parameter lock mode
        stateManager.enterParameterLockMode(2, 5);
        REQUIRE(stateManager.isInParameterLockMode());
        
        // Then exit
        auto result = stateManager.exitParameterLockMode();
        REQUIRE(result == SequencerStateManager::TransitionResult::SUCCESS);
        REQUIRE(stateManager.getCurrentMode() == SequencerStateManager::Mode::NORMAL);
        REQUIRE_FALSE(stateManager.isInParameterLockMode());
        
        const auto& context = stateManager.getParameterLockContext();
        REQUIRE_FALSE(context.active);
    }
    
    SECTION("Force exit to normal mode") {
        // Enter parameter lock mode
        stateManager.enterParameterLockMode(0, 7);
        REQUIRE(stateManager.isInParameterLockMode());
        
        // Force exit
        stateManager.forceExitToNormal();
        REQUIRE(stateManager.getCurrentMode() == SequencerStateManager::Mode::NORMAL);
        REQUIRE_FALSE(stateManager.isInParameterLockMode());
    }
    
    SECTION("Invalid parameter lock transitions") {
        // Test invalid track/step combinations
        auto result1 = stateManager.enterParameterLockMode(4, 0); // Invalid track
        REQUIRE(result1 != SequencerStateManager::TransitionResult::SUCCESS);
        
        auto result2 = stateManager.enterParameterLockMode(0, 8); // Invalid step
        REQUIRE(result2 != SequencerStateManager::TransitionResult::SUCCESS);
        
        auto result3 = stateManager.enterParameterLockMode(255, 255); // Both invalid
        REQUIRE(result3 != SequencerStateManager::TransitionResult::SUCCESS);
        
        // Should remain in normal mode
        REQUIRE(stateManager.getCurrentMode() == SequencerStateManager::Mode::NORMAL);
    }
}

TEST_CASE("SequencerStateManager - Control grid calculations", "[sequencer_state_manager][control_grid]") {
    SequencerStateManager stateManager;
    
    SECTION("Control grid mapping for left side steps (0-3)") {
        // Steps 0-3 should use columns 4-7 as control grid
        for (uint8_t step = 0; step < 4; ++step) {
            stateManager.enterParameterLockMode(0, step); // Track 0
            
            const auto& context = stateManager.getParameterLockContext();
            REQUIRE(context.controlGridStart == 4);
            
            // Test control grid button detection
            REQUIRE(stateManager.isInControlGrid(4));  // First control button
            REQUIRE(stateManager.isInControlGrid(7));  // Last control button in top row
            REQUIRE(stateManager.isInControlGrid(28)); // Bottom-right control button
            REQUIRE(stateManager.isInControlGrid(31)); // Last control button
            
            // These should NOT be in control grid
            REQUIRE_FALSE(stateManager.isInControlGrid(0)); // Held button area
            REQUIRE_FALSE(stateManager.isInControlGrid(3)); // Held button area
            
            stateManager.forceExitToNormal();
        }
    }
    
    SECTION("Control grid mapping for right side steps (4-7)") {
        // Steps 4-7 should use columns 0-3 as control grid
        for (uint8_t step = 4; step < 8; ++step) {
            stateManager.enterParameterLockMode(0, step); // Track 0
            
            const auto& context = stateManager.getParameterLockContext();
            REQUIRE(context.controlGridStart == 0);
            
            // Test control grid button detection
            REQUIRE(stateManager.isInControlGrid(0));  // First control button
            REQUIRE(stateManager.isInControlGrid(3));  // Last control button in top row
            REQUIRE(stateManager.isInControlGrid(24)); // Bottom-left control button
            REQUIRE(stateManager.isInControlGrid(27)); // Last control button
            
            // These should NOT be in control grid
            REQUIRE_FALSE(stateManager.isInControlGrid(4)); // Held button area
            REQUIRE_FALSE(stateManager.isInControlGrid(7)); // Held button area
            
            stateManager.forceExitToNormal();
        }
    }
    
    SECTION("Get control grid buttons") {
        stateManager.enterParameterLockMode(1, 2); // Track 1, Step 2 (column 2)
        
        uint8_t controlButtons[16];
        uint8_t count = stateManager.getControlGridButtons(controlButtons, 16);
        REQUIRE(count == 16); // Should return 16 buttons (4x4 grid)
        
        // Verify all returned buttons are in the control grid
        for (uint8_t i = 0; i < count; ++i) {
            REQUIRE(stateManager.isInControlGrid(controlButtons[i]));
        }
        
        // Test with limited buffer
        uint8_t limitedBuffer[8];
        count = stateManager.getControlGridButtons(limitedBuffer, 8);
        REQUIRE(count == 8); // Should return only what fits
    }
    
    SECTION("Control grid for all tracks and steps") {
        for (uint8_t track = 0; track < 4; ++track) {
            for (uint8_t step = 0; step < 8; ++step) {
                stateManager.enterParameterLockMode(track, step);
                
                const auto& context = stateManager.getParameterLockContext();
                REQUIRE(context.isValid());
                
                uint8_t expectedStart = (step < 4) ? 4 : 0;
                REQUIRE(context.controlGridStart == expectedStart);
                
                // Held button should NOT be in control grid
                uint8_t heldButton = context.getHeldButtonIndex();
                REQUIRE_FALSE(stateManager.isInControlGrid(heldButton));
                
                stateManager.forceExitToNormal();
            }
        }
    }
}

TEST_CASE("SequencerStateManager - Mode transition validation", "[sequencer_state_manager][validation]") {
    SequencerStateManager stateManager;
    
    SECTION("Mode transition permissions") {
        // From NORMAL mode
        REQUIRE(stateManager.canTransitionTo(SequencerStateManager::Mode::PARAMETER_LOCK));
        REQUIRE(stateManager.canTransitionTo(SequencerStateManager::Mode::PATTERN_SELECT));
        REQUIRE(stateManager.canTransitionTo(SequencerStateManager::Mode::SHIFT_CONTROL));
        REQUIRE(stateManager.canTransitionTo(SequencerStateManager::Mode::SETTINGS));
        
        // Enter parameter lock mode
        stateManager.enterParameterLockMode(0, 0);
        
        // From PARAMETER_LOCK mode - should only allow exit to NORMAL
        REQUIRE(stateManager.canTransitionTo(SequencerStateManager::Mode::NORMAL));
        // Other transitions might be blocked depending on implementation
    }
    
    SECTION("Generic mode transitions") {
        auto result = stateManager.transitionToMode(SequencerStateManager::Mode::PATTERN_SELECT);
        REQUIRE(result == SequencerStateManager::TransitionResult::SUCCESS);
        REQUIRE(stateManager.getCurrentMode() == SequencerStateManager::Mode::PATTERN_SELECT);
        
        // Return to normal
        result = stateManager.transitionToMode(SequencerStateManager::Mode::NORMAL);
        REQUIRE(result == SequencerStateManager::TransitionResult::SUCCESS);
        REQUIRE(stateManager.getCurrentMode() == SequencerStateManager::Mode::NORMAL);
    }
    
    SECTION("Invalid mode transitions") {
        // Try to transition to invalid mode
        auto result = stateManager.transitionToMode(SequencerStateManager::Mode::MODE_COUNT);
        REQUIRE(result == SequencerStateManager::TransitionResult::INVALID_MODE);
        REQUIRE(stateManager.getCurrentMode() == SequencerStateManager::Mode::NORMAL);
    }
}

TEST_CASE("SequencerStateManager - Timeout functionality", "[sequencer_state_manager][timeout]") {
    SequencerStateManager stateManager;
    
    SECTION("Set and check timeout") {
        uint32_t customTimeout = 5000; // 5 seconds
        stateManager.setParameterLockTimeout(customTimeout);
        
        stateManager.enterParameterLockMode(0, 0);
        const auto& context = stateManager.getParameterLockContext();
        REQUIRE(context.timeoutMs == customTimeout);
    }
    
    SECTION("Timeout detection") {
        uint32_t startTime = 1000;
        uint32_t shortTimeout = 2000; // 2 seconds
        
        stateManager.setParameterLockTimeout(shortTimeout);
        stateManager.enterParameterLockMode(0, 0);
        
        // Should not timeout immediately
        REQUIRE_FALSE(stateManager.hasTimedOut(startTime));
        REQUIRE_FALSE(stateManager.hasTimedOut(startTime + shortTimeout - 1));
        
        // Should timeout after the timeout period
        REQUIRE(stateManager.hasTimedOut(startTime + shortTimeout));
        REQUIRE(stateManager.hasTimedOut(startTime + shortTimeout + 1000));
    }
    
    SECTION("Update with timeout handling") {
        uint32_t currentTime = 1000;
        stateManager.setParameterLockTimeout(1000); // 1 second timeout
        
        stateManager.enterParameterLockMode(0, 0);
        REQUIRE(stateManager.isInParameterLockMode());
        
        // Update before timeout
        stateManager.update(currentTime + 500);
        REQUIRE(stateManager.isInParameterLockMode());
        
        // Update after timeout should exit mode
        stateManager.update(currentTime + 1500);
        REQUIRE_FALSE(stateManager.isInParameterLockMode());
        REQUIRE(stateManager.getCurrentMode() == SequencerStateManager::Mode::NORMAL);
    }
}

TEST_CASE("SequencerStateManager - Parameter lock context validation", "[sequencer_state_manager][context]") {
    SequencerStateManager stateManager;
    
    SECTION("Context validation for valid combinations") {
        for (uint8_t track = 0; track < 4; ++track) {
            for (uint8_t step = 0; step < 8; ++step) {
                stateManager.enterParameterLockMode(track, step);
                const auto& context = stateManager.getParameterLockContext();
                
                REQUIRE(context.isValid());
                REQUIRE(context.heldStep == step);
                REQUIRE(context.heldTrack == track);
                REQUIRE(context.getHeldButtonIndex() == track * 8 + step);
                
                stateManager.forceExitToNormal();
            }
        }
    }
    
    SECTION("Button index calculation") {
        struct TestCase {
            uint8_t track, step, expectedButton;
        };
        
        TestCase testCases[] = {
            {0, 0, 0},   // Track 0, Step 0 -> Button 0
            {0, 7, 7},   // Track 0, Step 7 -> Button 7
            {1, 0, 8},   // Track 1, Step 0 -> Button 8
            {1, 7, 15},  // Track 1, Step 7 -> Button 15
            {2, 0, 16},  // Track 2, Step 0 -> Button 16
            {2, 7, 23},  // Track 2, Step 7 -> Button 23
            {3, 0, 24},  // Track 3, Step 0 -> Button 24
            {3, 7, 31},  // Track 3, Step 7 -> Button 31
        };
        
        for (const auto& testCase : testCases) {
            stateManager.enterParameterLockMode(testCase.track, testCase.step);
            const auto& context = stateManager.getParameterLockContext();
            
            REQUIRE(context.getHeldButtonIndex() == testCase.expectedButton);
            
            stateManager.forceExitToNormal();
        }
    }
    
    SECTION("Control grid calculation") {
        stateManager.enterParameterLockMode(1, 2); // Track 1, Step 2
        const auto& context = stateManager.getParameterLockContext();
        
        // Step 2 is in left half (0-3), so control grid should start at column 4
        REQUIRE(context.controlGridStart == 4);
        
        stateManager.forceExitToNormal();
        stateManager.enterParameterLockMode(2, 6); // Track 2, Step 6
        const auto& context2 = stateManager.getParameterLockContext();
        
        // Step 6 is in right half (4-7), so control grid should start at column 0
        REQUIRE(context2.controlGridStart == 0);
    }
}

TEST_CASE("SequencerStateManager - Mode name utilities", "[sequencer_state_manager][utilities]") {
    SECTION("Mode name strings") {
        REQUIRE_THAT(SequencerStateManager::getModeName(SequencerStateManager::Mode::NORMAL),
                     ContainsSubstring("NORMAL"));
        REQUIRE_THAT(SequencerStateManager::getModeName(SequencerStateManager::Mode::PARAMETER_LOCK),
                     ContainsSubstring("PARAMETER"));
        REQUIRE_THAT(SequencerStateManager::getModeName(SequencerStateManager::Mode::PATTERN_SELECT),
                     ContainsSubstring("PATTERN"));
        REQUIRE_THAT(SequencerStateManager::getModeName(SequencerStateManager::Mode::SHIFT_CONTROL),
                     ContainsSubstring("SHIFT"));
        REQUIRE_THAT(SequencerStateManager::getModeName(SequencerStateManager::Mode::SETTINGS),
                     ContainsSubstring("SETTINGS"));
    }
}

TEST_CASE("SequencerStateManager - Edge cases and error handling", "[sequencer_state_manager][edge_cases]") {
    SequencerStateManager stateManager;
    
    SECTION("Multiple parameter lock mode entries") {
        // Enter parameter lock mode
        auto result1 = stateManager.enterParameterLockMode(0, 0);
        REQUIRE(result1 == SequencerStateManager::TransitionResult::SUCCESS);
        
        // Try to enter again - should either succeed (overwrite) or fail gracefully
        auto result2 = stateManager.enterParameterLockMode(1, 5);
        // Behavior depends on implementation - either way, state should be valid
        REQUIRE(stateManager.validateState());
    }
    
    SECTION("Exit without entry") {
        // Try to exit parameter lock mode without entering it
        auto result = stateManager.exitParameterLockMode();
        
        // Should handle gracefully and remain in normal mode
        REQUIRE(stateManager.getCurrentMode() == SequencerStateManager::Mode::NORMAL);
        REQUIRE(stateManager.validateState());
    }
    
    SECTION("State validation after various operations") {
        // Perform various state transitions
        stateManager.enterParameterLockMode(2, 3);
        REQUIRE(stateManager.validateState());
        
        stateManager.setParameterLockTimeout(3000);
        REQUIRE(stateManager.validateState());
        
        stateManager.exitParameterLockMode();
        REQUIRE(stateManager.validateState());
        
        stateManager.transitionToMode(SequencerStateManager::Mode::PATTERN_SELECT);
        REQUIRE(stateManager.validateState());
        
        stateManager.forceExitToNormal();
        REQUIRE(stateManager.validateState());
    }
    
    SECTION("Boundary value testing") {
        // Test boundary values for track and step
        struct BoundaryTest {
            uint8_t track, step;
            bool shouldSucceed;
        };
        
        BoundaryTest tests[] = {
            {0, 0, true},    // Minimum valid
            {3, 7, true},    // Maximum valid
            {4, 0, false},   // Invalid track
            {0, 8, false},   // Invalid step
            {4, 8, false},   // Both invalid
            {255, 255, false} // Extreme invalid
        };
        
        for (const auto& test : tests) {
            auto result = stateManager.enterParameterLockMode(test.track, test.step);
            
            if (test.shouldSucceed) {
                REQUIRE(result == SequencerStateManager::TransitionResult::SUCCESS);
                REQUIRE(stateManager.isInParameterLockMode());
                stateManager.forceExitToNormal();
            } else {
                REQUIRE(result != SequencerStateManager::TransitionResult::SUCCESS);
                REQUIRE(stateManager.getCurrentMode() == SequencerStateManager::Mode::NORMAL);
            }
            
            REQUIRE(stateManager.validateState());
        }
    }
}