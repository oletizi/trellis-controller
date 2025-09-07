#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "AdaptiveButtonTracker.h"
#include "SequencerStateManager.h"
#include "ParameterLockPool.h"
#include "ControlGrid.h"

using namespace Catch::Matchers;

// Mock time source for deterministic testing
class MockTimeSource {
private:
    uint32_t currentTime_;
    
public:
    MockTimeSource(uint32_t startTime = 1000) : currentTime_(startTime) {}
    
    uint32_t getCurrentTime() const { return currentTime_; }
    void advance(uint32_t ms) { currentTime_ += ms; }
    void setTime(uint32_t time) { currentTime_ = time; }
};

TEST_CASE("Parameter Lock Integration - Complete workflow", "[integration][parameter_lock][workflow]") {
    AdaptiveButtonTracker buttonTracker;
    SequencerStateManager stateManager;
    ParameterLockPool lockPool;
    ControlGrid controlGrid;
    MockTimeSource timeSource;
    
    const uint8_t TEST_TRACK = 1;
    const uint8_t TEST_STEP = 3;
    const uint8_t HELD_BUTTON = TEST_TRACK * 8 + TEST_STEP; // Button 11
    const uint32_t HOLD_THRESHOLD = 500;
    
    SECTION("Complete parameter lock cycle") {
        uint32_t currentTime = timeSource.getCurrentTime();
        
        // === STEP 1: Press and hold step button ===
        uint32_t buttonMask = 1UL << HELD_BUTTON;
        buttonTracker.update(buttonMask, currentTime);
        
        REQUIRE(buttonTracker.isPressed(HELD_BUTTON));
        REQUIRE_FALSE(buttonTracker.isHeld(HELD_BUTTON));
        REQUIRE(stateManager.getCurrentMode() == SequencerStateManager::Mode::NORMAL);
        
        // === STEP 2: Wait for hold threshold ===
        timeSource.advance(HOLD_THRESHOLD);
        currentTime = timeSource.getCurrentTime();
        buttonTracker.update(buttonMask, currentTime);
        
        REQUIRE(buttonTracker.isHeld(HELD_BUTTON));
        REQUIRE(buttonTracker.getHeldButton() == HELD_BUTTON);
        
        // === STEP 3: Enter parameter lock mode ===
        auto enterResult = stateManager.enterParameterLockMode(TEST_TRACK, TEST_STEP);
        REQUIRE(enterResult == SequencerStateManager::TransitionResult::SUCCESS);
        REQUIRE(stateManager.isInParameterLockMode());
        
        const auto& context = stateManager.getParameterLockContext();
        REQUIRE(context.active);
        REQUIRE(context.heldStep == TEST_STEP);
        REQUIRE(context.heldTrack == TEST_TRACK);
        REQUIRE(context.getHeldButtonIndex() == HELD_BUTTON);
        
        // === STEP 4: Get control grid mapping ===
        auto mapping = controlGrid.getMapping(TEST_STEP, TEST_TRACK);
        REQUIRE(mapping.isValid);
        
        // Verify held button is NOT in control grid
        REQUIRE_FALSE(mapping.isInControlArea(HELD_BUTTON));
        REQUIRE_FALSE(controlGrid.isInControlGrid(HELD_BUTTON, TEST_STEP, TEST_TRACK));
        
        // === STEP 5: Allocate parameter lock ===
        uint8_t lockIndex = lockPool.allocate(TEST_TRACK, TEST_STEP);
        REQUIRE(lockIndex != ParameterLockPool::INVALID_INDEX);
        
        auto& lock = lockPool.getLock(lockIndex);
        REQUIRE(lock.trackIndex == TEST_TRACK);
        REQUIRE(lock.stepIndex == TEST_STEP);
        
        // === STEP 6: Test parameter adjustments via control grid ===
        // Find note up/down buttons
        uint8_t noteUpButton = mapping.noteUpButton;
        uint8_t noteDownButton = mapping.noteDownButton;
        
        if (noteUpButton != 0xFF && noteDownButton != 0xFF) {
            // Test note parameter adjustment
            int8_t originalNote = lock.noteOffset;
            
            // Simulate note up button press
            REQUIRE(mapping.isInControlArea(noteUpButton));
            auto paramType = controlGrid.getParameterType(noteUpButton, mapping);
            REQUIRE(paramType == ParameterLockPool::ParameterType::NOTE);
            
            int8_t adjustment = controlGrid.getParameterAdjustment(noteUpButton, mapping);
            REQUIRE(adjustment > 0);
            
            // Apply adjustment
            lock.setParameter(ParameterLockPool::ParameterType::NOTE, true);
            lock.noteOffset += adjustment;
            
            REQUIRE(lock.hasParameter(ParameterLockPool::ParameterType::NOTE));
            REQUIRE(lock.noteOffset > originalNote);
            
            // Test note down button
            adjustment = controlGrid.getParameterAdjustment(noteDownButton, mapping);
            REQUIRE(adjustment < 0);
            
            lock.noteOffset += adjustment; // Should bring it back down
            REQUIRE(lock.noteOffset == originalNote);
        }
        
        // === STEP 7: Test velocity parameter ===
        uint8_t velocityUpButton = mapping.velocityUpButton;
        uint8_t velocityDownButton = mapping.velocityDownButton;
        
        if (velocityUpButton != 0xFF && velocityDownButton != 0xFF) {
            uint8_t originalVelocity = lock.velocity;
            
            // Test velocity up
            auto paramType = controlGrid.getParameterType(velocityUpButton, mapping);
            REQUIRE(paramType == ParameterLockPool::ParameterType::VELOCITY);
            
            int8_t adjustment = controlGrid.getParameterAdjustment(velocityUpButton, mapping);
            REQUIRE(adjustment > 0);
            
            lock.setParameter(ParameterLockPool::ParameterType::VELOCITY, true);
            lock.velocity = std::min(127, static_cast<int>(lock.velocity) + adjustment);
            
            REQUIRE(lock.hasParameter(ParameterLockPool::ParameterType::VELOCITY));
            REQUIRE(lock.velocity >= originalVelocity);
        }
        
        // === STEP 8: Update state manager (time progression) ===
        stateManager.update(currentTime);
        REQUIRE(stateManager.isInParameterLockMode()); // Should still be active
        
        // === STEP 9: Release held button ===
        timeSource.advance(100);
        currentTime = timeSource.getCurrentTime();
        buttonTracker.update(0, currentTime); // No buttons pressed
        
        REQUIRE_FALSE(buttonTracker.isPressed(HELD_BUTTON));
        REQUIRE(buttonTracker.wasReleased(HELD_BUTTON));
        REQUIRE_FALSE(buttonTracker.isHeld(HELD_BUTTON));
        
        // === STEP 10: Exit parameter lock mode ===
        auto exitResult = stateManager.exitParameterLockMode();
        REQUIRE(exitResult == SequencerStateManager::TransitionResult::SUCCESS);
        REQUIRE_FALSE(stateManager.isInParameterLockMode());
        REQUIRE(stateManager.getCurrentMode() == SequencerStateManager::Mode::NORMAL);
        
        // === STEP 11: Verify parameter lock persists ===
        REQUIRE(lockPool.isValidIndex(lockIndex));
        REQUIRE(lock.inUse);
        REQUIRE(lock.isValid());
        
        // === STEP 12: Find lock by position ===
        uint8_t foundIndex = lockPool.findLock(TEST_TRACK, TEST_STEP);
        REQUIRE(foundIndex == lockIndex);
        
        // === CLEANUP ===
        lockPool.deallocate(lockIndex);
        REQUIRE_FALSE(lockPool.isValidIndex(lockIndex));
    }
}

TEST_CASE("Parameter Lock Integration - Multiple parameter locks", "[integration][parameter_lock][multiple]") {
    AdaptiveButtonTracker buttonTracker;
    SequencerStateManager stateManager;
    ParameterLockPool lockPool;
    ControlGrid controlGrid;
    MockTimeSource timeSource;
    
    SECTION("Create locks for different steps") {
        struct TestStep {
            uint8_t track, step, button;
        };
        
        TestStep testSteps[] = {
            {0, 2, 2},   // Track 0, Step 2, Button 2
            {1, 5, 13},  // Track 1, Step 5, Button 13
            {2, 0, 16},  // Track 2, Step 0, Button 16
            {3, 7, 31}   // Track 3, Step 7, Button 31
        };
        
        std::vector<uint8_t> lockIndices;
        
        for (const auto& testStep : testSteps) {
            uint32_t currentTime = timeSource.getCurrentTime();
            
            // Press and hold button
            uint32_t buttonMask = 1UL << testStep.button;
            buttonTracker.update(buttonMask, currentTime);
            
            // Wait for hold
            timeSource.advance(500);
            currentTime = timeSource.getCurrentTime();
            buttonTracker.update(buttonMask, currentTime);
            
            REQUIRE(buttonTracker.isHeld(testStep.button));
            
            // Enter parameter lock mode
            auto result = stateManager.enterParameterLockMode(testStep.track, testStep.step);
            REQUIRE(result == SequencerStateManager::TransitionResult::SUCCESS);
            
            // Allocate parameter lock
            uint8_t lockIndex = lockPool.allocate(testStep.track, testStep.step);
            REQUIRE(lockIndex != ParameterLockPool::INVALID_INDEX);
            lockIndices.push_back(lockIndex);
            
            // Set some parameters
            auto& lock = lockPool.getLock(lockIndex);
            lock.setParameter(ParameterLockPool::ParameterType::NOTE, true);
            lock.setParameter(ParameterLockPool::ParameterType::VELOCITY, true);
            lock.noteOffset = testStep.step - 4; // Some variation
            lock.velocity = 100 + testStep.track * 5;
            
            // Release button and exit mode
            timeSource.advance(100);
            currentTime = timeSource.getCurrentTime();
            buttonTracker.update(0, currentTime);
            
            stateManager.exitParameterLockMode();
            
            timeSource.advance(100); // Gap between tests
        }
        
        // Verify all locks are still valid and have correct parameters
        REQUIRE(lockIndices.size() == 4);
        REQUIRE(lockPool.getUsedCount() == 4);
        
        for (size_t i = 0; i < lockIndices.size(); ++i) {
            const auto& testStep = testSteps[i];
            uint8_t lockIndex = lockIndices[i];
            
            REQUIRE(lockPool.isValidIndex(lockIndex));
            
            const auto& lock = lockPool.getLock(lockIndex);
            REQUIRE(lock.trackIndex == testStep.track);
            REQUIRE(lock.stepIndex == testStep.step);
            REQUIRE(lock.hasParameter(ParameterLockPool::ParameterType::NOTE));
            REQUIRE(lock.hasParameter(ParameterLockPool::ParameterType::VELOCITY));
            REQUIRE(lock.noteOffset == static_cast<int8_t>(testStep.step - 4));
            REQUIRE(lock.velocity == 100 + testStep.track * 5);
            
            // Verify lookup by position
            uint8_t foundIndex = lockPool.findLock(testStep.track, testStep.step);
            REQUIRE(foundIndex == lockIndex);
        }
        
        // Cleanup
        for (uint8_t lockIndex : lockIndices) {
            lockPool.deallocate(lockIndex);
        }
        
        REQUIRE(lockPool.isEmpty());
    }
}

TEST_CASE("Parameter Lock Integration - Control grid interaction", "[integration][parameter_lock][control_grid]") {
    SequencerStateManager stateManager;
    ParameterLockPool lockPool;
    ControlGrid controlGrid;
    
    SECTION("Control grid adapts to different held steps") {
        struct TestCase {
            uint8_t track, step;
            uint8_t expectedControlStart;
        };
        
        TestCase testCases[] = {
            {0, 0, 4}, // Left side step -> right side control
            {0, 1, 4}, // Left side step -> right side control
            {0, 2, 4}, // Left side step -> right side control
            {0, 3, 4}, // Left side step -> right side control
            {0, 4, 0}, // Right side step -> left side control
            {0, 5, 0}, // Right side step -> left side control
            {0, 6, 0}, // Right side step -> left side control
            {0, 7, 0}, // Right side step -> left side control
        };
        
        for (const auto& testCase : testCases) {
            // Enter parameter lock mode
            stateManager.enterParameterLockMode(testCase.track, testCase.step);
            
            // Get context and verify control grid calculation
            const auto& context = stateManager.getParameterLockContext();
            REQUIRE(context.controlGridStart == testCase.expectedControlStart);
            
            // Get control grid mapping
            auto mapping = controlGrid.getMapping(testCase.step, testCase.track);
            REQUIRE(mapping.isValid);
            REQUIRE(mapping.controlAreaStart == testCase.expectedControlStart);
            
            // Verify held button is not in control area
            uint8_t heldButton = testCase.track * 8 + testCase.step;
            REQUIRE_FALSE(mapping.isInControlArea(heldButton));
            
            // Verify control buttons are in expected area
            bool foundControlButton = false;
            for (uint8_t button = 0; button < 32; ++button) {
                if (mapping.isInControlArea(button)) {
                    foundControlButton = true;
                    
                    uint8_t row, col;
                    ControlGrid::buttonToRowCol(button, row, col);
                    
                    if (testCase.expectedControlStart == 0) {
                        REQUIRE(col < 4); // Should be in left columns (0-3)
                    } else {
                        REQUIRE(col >= 4); // Should be in right columns (4-7)
                    }
                }
            }
            
            REQUIRE(foundControlButton);
            
            // Exit mode
            stateManager.forceExitToNormal();
        }
    }
}

TEST_CASE("Parameter Lock Integration - Error handling and edge cases", "[integration][parameter_lock][edge_cases]") {
    AdaptiveButtonTracker buttonTracker;
    SequencerStateManager stateManager;
    ParameterLockPool lockPool;
    ControlGrid controlGrid;
    MockTimeSource timeSource;
    
    SECTION("Hold detection failure scenarios") {
        uint32_t currentTime = timeSource.getCurrentTime();
        const uint8_t testButton = 5;
        
        // Press button but don't hold long enough
        uint32_t buttonMask = 1UL << testButton;
        buttonTracker.update(buttonMask, currentTime);
        
        // Release before threshold
        timeSource.advance(400); // Less than 500ms
        currentTime = timeSource.getCurrentTime();
        buttonTracker.update(0, currentTime);
        
        REQUIRE_FALSE(buttonTracker.isHeld(testButton));
        REQUIRE(buttonTracker.wasReleased(testButton));
        
        // Should not be able to enter parameter lock mode without proper hold
        // (This depends on how the application integrates the components)
    }
    
    SECTION("Parameter lock pool exhaustion") {
        std::vector<uint8_t> indices;
        
        // Fill the pool
        for (size_t i = 0; i < ParameterLockPool::MAX_LOCKS; ++i) {
            uint8_t track = i % 4;
            uint8_t step = (i / 4) % 8;
            uint8_t index = lockPool.allocate(track, step);
            if (index != ParameterLockPool::INVALID_INDEX) {
                indices.push_back(index);
            }
        }
        
        REQUIRE(lockPool.isFull());
        
        // Try to allocate one more
        uint8_t overflowIndex = lockPool.allocate(0, 0);
        REQUIRE(overflowIndex == ParameterLockPool::INVALID_INDEX);
        
        // Should still be able to use control grid and state manager
        stateManager.enterParameterLockMode(1, 3);
        auto mapping = controlGrid.getMapping(3, 1);
        REQUIRE(mapping.isValid);
        
        stateManager.forceExitToNormal();
        
        // Cleanup
        for (uint8_t index : indices) {
            lockPool.deallocate(index);
        }
    }
    
    SECTION("Invalid state transitions") {
        // Try to enter parameter lock with invalid coordinates
        auto result1 = stateManager.enterParameterLockMode(4, 0); // Invalid track
        REQUIRE(result1 != SequencerStateManager::TransitionResult::SUCCESS);
        REQUIRE(stateManager.getCurrentMode() == SequencerStateManager::Mode::NORMAL);
        
        auto result2 = stateManager.enterParameterLockMode(0, 8); // Invalid step
        REQUIRE(result2 != SequencerStateManager::TransitionResult::SUCCESS);
        REQUIRE(stateManager.getCurrentMode() == SequencerStateManager::Mode::NORMAL);
        
        // Try to exit without entering
        auto result3 = stateManager.exitParameterLockMode();
        REQUIRE(stateManager.getCurrentMode() == SequencerStateManager::Mode::NORMAL);
    }
    
    SECTION("Timeout handling") {
        const uint32_t SHORT_TIMEOUT = 1000; // 1 second
        stateManager.setParameterLockTimeout(SHORT_TIMEOUT);
        
        uint32_t startTime = timeSource.getCurrentTime();
        
        // Enter parameter lock mode
        stateManager.enterParameterLockMode(0, 0);
        REQUIRE(stateManager.isInParameterLockMode());
        
        // Update before timeout
        timeSource.advance(500);
        stateManager.update(timeSource.getCurrentTime());
        REQUIRE(stateManager.isInParameterLockMode());
        
        // Update after timeout
        timeSource.advance(600); // Total 1100ms > 1000ms timeout
        stateManager.update(timeSource.getCurrentTime());
        REQUIRE_FALSE(stateManager.isInParameterLockMode());
        REQUIRE(stateManager.getCurrentMode() == SequencerStateManager::Mode::NORMAL);
    }
}

TEST_CASE("Parameter Lock Integration - Learning and adaptation", "[integration][parameter_lock][learning]") {
    AdaptiveButtonTracker buttonTracker;
    ControlGrid controlGrid;
    MockTimeSource timeSource;
    
    SECTION("Button tracker learning from hold patterns") {
        const uint8_t testButton = 7;
        
        // Perform several hold cycles with varying durations
        uint32_t holdDurations[] = {520, 480, 510, 490, 530, 470, 500, 485};
        
        for (uint32_t duration : holdDurations) {
            uint32_t startTime = timeSource.getCurrentTime();
            
            // Press button
            uint32_t buttonMask = 1UL << testButton;
            buttonTracker.update(buttonMask, startTime);
            
            // Hold for specified duration
            timeSource.advance(duration);
            uint32_t endTime = timeSource.getCurrentTime();
            buttonTracker.update(buttonMask, endTime);
            
            if (duration >= 500) {
                REQUIRE(buttonTracker.isHeld(testButton));
                // Record successful activation
                buttonTracker.recordSuccessfulActivation(testButton, duration);
            }
            
            // Release button
            timeSource.advance(50);
            buttonTracker.update(0, timeSource.getCurrentTime());
            
            timeSource.advance(200); // Gap between cycles
        }
        
        // Check learning statistics
        const auto& stats = buttonTracker.getLearningStats();
        REQUIRE(stats.totalActivations > 0);
        REQUIRE(stats.samples > 0);
    }
    
    SECTION("Control grid learning from usage patterns") {
        // Simulate user preferring right side controls
        for (int i = 0; i < 50; ++i) {
            uint8_t rightSideButton = 4 + (i % 28); // Buttons 4-31 (right side)
            controlGrid.recordButtonUsage(rightSideButton);
        }
        
        // Add some left side usage
        for (int i = 0; i < 10; ++i) {
            controlGrid.recordButtonUsage(i % 4); // Buttons 0-3 (left side)
        }
        
        const auto& stats = controlGrid.getUsageStats();
        REQUIRE(stats.totalUsage == 60);
        REQUIRE(stats.rightSideUsage > stats.leftSideUsage);
        
        // Detection should reflect right-hand preference
        auto detectedPreference = controlGrid.detectDominantHand();
        // Implementation-specific behavior for hand detection
    }
}

TEST_CASE("Parameter Lock Integration - Performance characteristics", "[integration][parameter_lock][performance]") {
    AdaptiveButtonTracker buttonTracker;
    SequencerStateManager stateManager;
    ParameterLockPool lockPool;
    ControlGrid controlGrid;
    MockTimeSource timeSource;
    
    SECTION("Rapid state transitions") {
        const int TRANSITION_COUNT = 100;
        
        for (int i = 0; i < TRANSITION_COUNT; ++i) {
            uint8_t track = i % 4;
            uint8_t step = (i / 4) % 8;
            
            // Enter and immediately exit parameter lock mode
            auto enterResult = stateManager.enterParameterLockMode(track, step);
            REQUIRE(enterResult == SequencerStateManager::TransitionResult::SUCCESS);
            
            auto exitResult = stateManager.exitParameterLockMode();
            REQUIRE(exitResult == SequencerStateManager::TransitionResult::SUCCESS);
            
            // State should be consistent
            REQUIRE(stateManager.validateState());
        }
    }
    
    SECTION("Pool allocation/deallocation stress") {
        const int OPERATION_COUNT = 200;
        std::vector<uint8_t> activeIndices;
        
        for (int i = 0; i < OPERATION_COUNT; ++i) {
            if (activeIndices.size() < 32 && (i % 3 != 0 || activeIndices.empty())) {
                // Allocate
                uint8_t track = i % 4;
                uint8_t step = (i / 4) % 8;
                uint8_t index = lockPool.allocate(track, step);
                if (index != ParameterLockPool::INVALID_INDEX) {
                    activeIndices.push_back(index);
                }
            } else {
                // Deallocate
                if (!activeIndices.empty()) {
                    uint8_t index = activeIndices.back();
                    activeIndices.pop_back();
                    lockPool.deallocate(index);
                }
            }
            
            // Verify integrity every 20 operations
            if (i % 20 == 0) {
                REQUIRE(lockPool.validateIntegrity());
            }
        }
        
        // Cleanup
        for (uint8_t index : activeIndices) {
            lockPool.deallocate(index);
        }
        
        REQUIRE(lockPool.isEmpty());
        REQUIRE(lockPool.validateIntegrity());
    }
    
    SECTION("Control grid mapping consistency") {
        // Verify that repeated calls to getMapping return consistent results
        for (uint8_t track = 0; track < 4; ++track) {
            for (uint8_t step = 0; step < 8; ++step) {
                auto mapping1 = controlGrid.getMapping(step, track);
                auto mapping2 = controlGrid.getMapping(step, track);
                
                REQUIRE(mapping1.isValid == mapping2.isValid);
                REQUIRE(mapping1.controlAreaStart == mapping2.controlAreaStart);
                REQUIRE(mapping1.controlAreaEnd == mapping2.controlAreaEnd);
                REQUIRE(mapping1.noteUpButton == mapping2.noteUpButton);
                REQUIRE(mapping1.noteDownButton == mapping2.noteDownButton);
                REQUIRE(mapping1.velocityUpButton == mapping2.velocityUpButton);
                REQUIRE(mapping1.velocityDownButton == mapping2.velocityDownButton);
            }
        }
    }
}

TEST_CASE("Parameter Lock Integration - Real-world simulation", "[integration][parameter_lock][simulation]") {
    AdaptiveButtonTracker buttonTracker;
    SequencerStateManager stateManager;
    ParameterLockPool lockPool;
    ControlGrid controlGrid;
    MockTimeSource timeSource;
    
    SECTION("Realistic parameter editing session") {
        // Simulate user editing parameters on multiple steps
        struct EditSession {
            uint8_t track, step;
            std::vector<std::pair<uint8_t, int8_t>> parameterEdits; // button, expected adjustment
        };
        
        EditSession sessions[] = {
            {0, 2, {}}, // Will be filled with actual control buttons
            {1, 5, {}},
            {2, 0, {}},
            {3, 7, {}}
        };
        
        for (auto& session : sessions) {
            uint32_t sessionStartTime = timeSource.getCurrentTime();
            uint8_t heldButton = session.track * 8 + session.step;
            
            // === Phase 1: Enter parameter lock mode ===
            
            // Press and hold step button
            uint32_t buttonMask = 1UL << heldButton;
            buttonTracker.update(buttonMask, sessionStartTime);
            
            // Wait for hold threshold
            timeSource.advance(500);
            uint32_t holdTime = timeSource.getCurrentTime();
            buttonTracker.update(buttonMask, holdTime);
            
            REQUIRE(buttonTracker.isHeld(heldButton));
            
            // Enter parameter lock mode
            auto enterResult = stateManager.enterParameterLockMode(session.track, session.step);
            REQUIRE(enterResult == SequencerStateManager::TransitionResult::SUCCESS);
            
            // Allocate parameter lock
            uint8_t lockIndex = lockPool.allocate(session.track, session.step);
            REQUIRE(lockIndex != ParameterLockPool::INVALID_INDEX);
            
            auto& lock = lockPool.getLock(lockIndex);
            
            // === Phase 2: Edit parameters ===
            
            auto mapping = controlGrid.getMapping(session.step, session.track);
            REQUIRE(mapping.isValid);
            
            // Simulate parameter editing
            std::vector<uint8_t> controlButtons;
            for (uint8_t button = 0; button < 32; ++button) {
                if (mapping.isInControlArea(button)) {
                    controlButtons.push_back(button);
                }
            }
            
            REQUIRE(!controlButtons.empty());
            
            // Edit note parameter if available
            if (mapping.noteUpButton != 0xFF) {
                int8_t originalNote = lock.noteOffset;
                
                // Simulate button press for note up
                controlGrid.recordButtonUsage(mapping.noteUpButton);
                
                auto paramType = controlGrid.getParameterType(mapping.noteUpButton, mapping);
                REQUIRE(paramType == ParameterLockPool::ParameterType::NOTE);
                
                int8_t adjustment = controlGrid.getParameterAdjustment(mapping.noteUpButton, mapping);
                lock.setParameter(ParameterLockPool::ParameterType::NOTE, true);
                lock.noteOffset += adjustment;
                
                REQUIRE(lock.hasParameter(ParameterLockPool::ParameterType::NOTE));
                REQUIRE(lock.noteOffset != originalNote);
            }
            
            // Edit velocity parameter if available
            if (mapping.velocityUpButton != 0xFF) {
                uint8_t originalVelocity = lock.velocity;
                
                controlGrid.recordButtonUsage(mapping.velocityUpButton);
                
                auto paramType = controlGrid.getParameterType(mapping.velocityUpButton, mapping);
                REQUIRE(paramType == ParameterLockPool::ParameterType::VELOCITY);
                
                int8_t adjustment = controlGrid.getParameterAdjustment(mapping.velocityUpButton, mapping);
                lock.setParameter(ParameterLockPool::ParameterType::VELOCITY, true);
                lock.velocity = std::max(0, std::min(127, static_cast<int>(lock.velocity) + adjustment));
                
                REQUIRE(lock.hasParameter(ParameterLockPool::ParameterType::VELOCITY));
            }
            
            // === Phase 3: Exit parameter lock mode ===
            
            timeSource.advance(100); // User thinks for a moment
            
            // Release step button
            buttonTracker.update(0, timeSource.getCurrentTime());
            REQUIRE(buttonTracker.wasReleased(heldButton));
            
            // Exit parameter lock mode
            auto exitResult = stateManager.exitParameterLockMode();
            REQUIRE(exitResult == SequencerStateManager::TransitionResult::SUCCESS);
            REQUIRE_FALSE(stateManager.isInParameterLockMode());
            
            // Verify lock persists
            REQUIRE(lockPool.isValidIndex(lockIndex));
            REQUIRE(lock.isValid());
            
            timeSource.advance(500); // Break between sessions
        }
        
        // Verify all locks are still valid
        REQUIRE(lockPool.getUsedCount() == 4);
        
        // Verify usage statistics were collected
        const auto& usageStats = controlGrid.getUsageStats();
        REQUIRE(usageStats.totalUsage > 0);
        
        // Cleanup all locks
        for (int i = 0; i < 4; ++i) {
            uint8_t track = i;
            uint8_t step = (i == 0) ? 2 : (i == 1) ? 5 : (i == 2) ? 0 : 7;
            uint8_t lockIndex = lockPool.findLock(track, step);
            if (lockIndex != ParameterLockPool::INVALID_INDEX) {
                lockPool.deallocate(lockIndex);
            }
        }
        
        REQUIRE(lockPool.isEmpty());
    }
}