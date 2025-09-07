#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <memory>
#include <chrono>

// Core components
#include "AdaptiveButtonTracker.h"
#include "SequencerStateManager.h"
#include "ParameterLockPool.h"
#include "ControlGrid.h"

// Simulation components (for testing)
#include "CursesInput.h"

using namespace Catch::Matchers;

// Test time source that can be controlled deterministically
class TestTimeSource : public IClock {
private:
    uint32_t currentTime_;
    
public:
    TestTimeSource(uint32_t startTime = 1000) : currentTime_(startTime) {}
    
    uint32_t getCurrentTime() const override { return currentTime_; }
    void advance(uint32_t ms) { currentTime_ += ms; }
    void setTime(uint32_t time) { currentTime_ = time; }
};

// Mock button input system that simulates the CursesInput behavior
class MockButtonInput {
private:
    uint32_t buttonStates_;
    TestTimeSource* timeSource_;
    
public:
    MockButtonInput(TestTimeSource* timeSource) : buttonStates_(0), timeSource_(timeSource) {}
    
    void pressButton(uint8_t buttonIndex) {
        buttonStates_ |= (1UL << buttonIndex);
    }
    
    void releaseButton(uint8_t buttonIndex) {
        buttonStates_ &= ~(1UL << buttonIndex);
    }
    
    void releaseAllButtons() {
        buttonStates_ = 0;
    }
    
    uint32_t getButtonMask() const {
        return buttonStates_;
    }
    
    // Simulate the key mapping from CursesInput for testing
    struct KeyMapping {
        char pressKey;
        char releaseKey;
        uint8_t buttonIndex;
        const char* description;
    };
    
    static constexpr KeyMapping KEY_MAPPINGS[] = {
        {'1', '!', 0, "Button 0 (Track 0, Step 0)"},
        {'2', '@', 1, "Button 1 (Track 0, Step 1)"},
        {'3', '#', 2, "Button 2 (Track 0, Step 2)"},
        {'4', '$', 3, "Button 3 (Track 0, Step 3)"},
        {'5', '%', 4, "Button 4 (Track 0, Step 4)"},
        {'6', '^', 5, "Button 5 (Track 0, Step 5)"},
        {'7', '&', 6, "Button 6 (Track 0, Step 6)"},
        {'8', '*', 7, "Button 7 (Track 0, Step 7)"},
        
        {'Q', 'q', 8, "Button 8 (Track 1, Step 0)"},
        {'W', 'w', 9, "Button 9 (Track 1, Step 1)"},
        {'E', 'e', 10, "Button 10 (Track 1, Step 2)"},
        {'R', 'r', 11, "Button 11 (Track 1, Step 3)"},
        {'T', 't', 12, "Button 12 (Track 1, Step 4)"},
        {'Y', 'y', 13, "Button 13 (Track 1, Step 5)"},
        {'U', 'u', 14, "Button 14 (Track 1, Step 6)"},
        {'I', 'i', 15, "Button 15 (Track 1, Step 7)"},
        
        {'A', 'a', 16, "Button 16 (Track 2, Step 0)"},
        {'S', 's', 17, "Button 17 (Track 2, Step 1)"},
        {'D', 'd', 18, "Button 18 (Track 2, Step 2)"},
        {'F', 'f', 19, "Button 19 (Track 2, Step 3)"},
        {'G', 'g', 20, "Button 20 (Track 2, Step 4)"},
        {'H', 'h', 21, "Button 21 (Track 2, Step 5)"},
        {'J', 'j', 22, "Button 22 (Track 2, Step 6)"},
        {'K', 'k', 23, "Button 23 (Track 2, Step 7)"},
        
        {'Z', 'z', 24, "Button 24 (Track 3, Step 0)"},
        {'X', 'x', 25, "Button 25 (Track 3, Step 1)"},
        {'C', 'c', 26, "Button 26 (Track 3, Step 2)"},
        {'V', 'v', 27, "Button 27 (Track 3, Step 3)"},
        {'B', 'b', 28, "Button 28 (Track 3, Step 4)"},
        {'N', 'n', 29, "Button 29 (Track 3, Step 5)"},
        {'M', 'm', 30, "Button 30 (Track 3, Step 6)"},
        {'<', ',', 31, "Button 31 (Track 3, Step 7)"}
    };
    
    uint8_t getButtonForKey(char key) const {
        for (const auto& mapping : KEY_MAPPINGS) {
            if (mapping.pressKey == key || mapping.releaseKey == key) {
                return mapping.buttonIndex;
            }
        }
        return 255; // Invalid
    }
    
    void simulateKeyPress(char key) {
        uint8_t button = getButtonForKey(key);
        if (button < 32) {
            pressButton(button);
        }
    }
    
    void simulateKeyRelease(char key) {
        uint8_t button = getButtonForKey(key);
        if (button < 32) {
            releaseButton(button);
        }
    }
};

// Complete parameter lock test system
class ParameterLockTestSystem {
private:
    std::unique_ptr<TestTimeSource> timeSource_;
    std::unique_ptr<AdaptiveButtonTracker> buttonTracker_;
    std::unique_ptr<SequencerStateManager> stateManager_;
    std::unique_ptr<ParameterLockPool> lockPool_;
    std::unique_ptr<ControlGrid> controlGrid_;
    std::unique_ptr<MockButtonInput> buttonInput_;
    
public:
    ParameterLockTestSystem() {
        timeSource_ = std::make_unique<TestTimeSource>();
        buttonTracker_ = std::make_unique<AdaptiveButtonTracker>();
        stateManager_ = std::make_unique<SequencerStateManager>();
        lockPool_ = std::make_unique<ParameterLockPool>();
        controlGrid_ = std::make_unique<ControlGrid>();
        buttonInput_ = std::make_unique<MockButtonInput>(timeSource_.get());
    }
    
    // Accessors
    TestTimeSource& getTimeSource() { return *timeSource_; }
    AdaptiveButtonTracker& getButtonTracker() { return *buttonTracker_; }
    SequencerStateManager& getStateManager() { return *stateManager_; }
    ParameterLockPool& getLockPool() { return *lockPool_; }
    ControlGrid& getControlGrid() { return *controlGrid_; }
    MockButtonInput& getButtonInput() { return *buttonInput_; }
    
    // Update system state
    void update() {
        uint32_t currentTime = timeSource_->getCurrentTime();
        uint32_t buttonMask = buttonInput_->getButtonMask();
        
        buttonTracker_->update(buttonMask, currentTime);
        stateManager_->update(currentTime);
    }
    
    // High-level operations
    bool simulateButtonHold(uint8_t buttonIndex, uint32_t holdDuration) {
        uint32_t startTime = timeSource_->getCurrentTime();
        
        // Press button
        buttonInput_->pressButton(buttonIndex);
        update();
        
        if (!buttonTracker_->isPressed(buttonIndex)) {
            return false; // Failed to press
        }
        
        // Wait for hold threshold
        timeSource_->advance(holdDuration);
        update();
        
        return buttonTracker_->isHeld(buttonIndex);
    }
    
    void simulateButtonRelease(uint8_t buttonIndex) {
        buttonInput_->releaseButton(buttonIndex);
        timeSource_->advance(10); // Small delay
        update();
    }
    
    // Simulate key-based input (like CursesInput)
    void simulateKeyHold(char pressKey, char releaseKey, uint32_t holdDuration) {
        uint8_t button = buttonInput_->getButtonForKey(pressKey);
        REQUIRE(button < 32);
        
        buttonInput_->simulateKeyPress(pressKey);
        update();
        
        timeSource_->advance(holdDuration);
        update();
        
        buttonInput_->simulateKeyRelease(releaseKey);
        update();
    }
    
    void simulateKeyPress(char key) {
        buttonInput_->simulateKeyPress(key);
        timeSource_->advance(10);
        update();
    }
    
    void simulateKeyRelease(char key) {
        buttonInput_->simulateKeyRelease(key);
        timeSource_->advance(10);
        update();
    }
};

TEST_CASE("Parameter Lock End-to-End - Complete workflow automation", "[end_to_end][parameter_lock][automated]") {
    ParameterLockTestSystem system;
    
    SECTION("Automated parameter lock editing session") {
        const uint8_t TEST_BUTTON = 11; // Button 11 (Track 1, Step 3) - mapped to 'R'/'r'
        const uint8_t TEST_TRACK = 1;
        const uint8_t TEST_STEP = 3;
        const uint32_t HOLD_THRESHOLD = 500;
        
        auto& timeSource = system.getTimeSource();
        auto& buttonTracker = system.getButtonTracker();
        auto& stateManager = system.getStateManager();
        auto& lockPool = system.getLockPool();
        auto& controlGrid = system.getControlGrid();
        auto& buttonInput = system.getButtonInput();
        
        INFO("Starting automated parameter lock test for Button " << (int)TEST_BUTTON << " (Track " << (int)TEST_TRACK << ", Step " << (int)TEST_STEP << ")");
        
        // === PHASE 1: Setup and Initial State Verification ===
        REQUIRE(stateManager.getCurrentMode() == SequencerStateManager::Mode::NORMAL);
        REQUIRE(lockPool.isEmpty());
        REQUIRE_FALSE(buttonTracker.isPressed(TEST_BUTTON));
        
        uint32_t testStartTime = timeSource.getCurrentTime();
        INFO("Test start time: " << testStartTime << "ms");
        
        // === PHASE 2: Simulate Step Button Hold ===
        INFO("Phase 2: Simulating button hold...");
        
        // Press button (simulate 'R' key press)
        buttonInput.simulateKeyPress('R');
        system.update();
        
        REQUIRE(buttonTracker.isPressed(TEST_BUTTON));
        REQUIRE_FALSE(buttonTracker.isHeld(TEST_BUTTON));
        INFO("Button pressed successfully");
        
        // Wait for hold threshold
        timeSource.advance(HOLD_THRESHOLD);
        system.update();
        
        REQUIRE(buttonTracker.isHeld(TEST_BUTTON));
        REQUIRE(buttonTracker.getHeldButton() == TEST_BUTTON);
        INFO("Hold threshold reached, button is held");
        
        // === PHASE 3: Enter Parameter Lock Mode ===
        INFO("Phase 3: Entering parameter lock mode...");
        
        auto enterResult = stateManager.enterParameterLockMode(TEST_TRACK, TEST_STEP);
        REQUIRE(enterResult == SequencerStateManager::TransitionResult::SUCCESS);
        REQUIRE(stateManager.isInParameterLockMode());
        
        const auto& context = stateManager.getParameterLockContext();
        REQUIRE(context.active);
        REQUIRE(context.heldStep == TEST_STEP);
        REQUIRE(context.heldTrack == TEST_TRACK);
        INFO("Parameter lock mode entered successfully");
        
        // === PHASE 4: Setup Parameter Lock ===
        INFO("Phase 4: Allocating parameter lock...");
        
        uint8_t lockIndex = lockPool.allocate(TEST_TRACK, TEST_STEP);
        REQUIRE(lockIndex != ParameterLockPool::INVALID_INDEX);
        
        auto& lock = lockPool.getLock(lockIndex);
        REQUIRE(lock.trackIndex == TEST_TRACK);
        REQUIRE(lock.stepIndex == TEST_STEP);
        INFO("Parameter lock allocated with index " << (int)lockIndex);
        
        // === PHASE 5: Get Control Grid Mapping ===
        INFO("Phase 5: Getting control grid mapping...");
        
        auto mapping = controlGrid.getMapping(TEST_STEP, TEST_TRACK);
        REQUIRE(mapping.isValid);
        
        // Step 3 is on left side, so control grid should be on right side (start at column 4)
        REQUIRE(mapping.controlAreaStart == 4);
        INFO("Control grid starts at column " << (int)mapping.controlAreaStart);
        
        // Verify held button is not in control area
        REQUIRE_FALSE(mapping.isInControlArea(TEST_BUTTON));
        INFO("Held button " << (int)TEST_BUTTON << " correctly excluded from control grid");
        
        // === PHASE 6: Test Parameter Adjustments ===
        INFO("Phase 6: Testing parameter adjustments...");
        
        // Test note parameter adjustment
        if (mapping.noteUpButton != 0xFF) {
            uint8_t noteUpButton = mapping.noteUpButton;
            INFO("Testing note up button: " << (int)noteUpButton);
            
            REQUIRE(mapping.isInControlArea(noteUpButton));
            
            auto paramType = controlGrid.getParameterType(noteUpButton, mapping);
            REQUIRE(paramType == ParameterLockPool::ParameterType::NOTE);
            
            int8_t adjustment = controlGrid.getParameterAdjustment(noteUpButton, mapping);
            REQUIRE(adjustment > 0);
            
            // Apply adjustment
            int8_t originalNote = lock.noteOffset;
            lock.setParameter(ParameterLockPool::ParameterType::NOTE, true);
            lock.noteOffset += adjustment;
            
            REQUIRE(lock.hasParameter(ParameterLockPool::ParameterType::NOTE));
            REQUIRE(lock.noteOffset != originalNote);
            INFO("Note parameter adjusted from " << (int)originalNote << " to " << (int)lock.noteOffset);
            
            // Record button usage for learning
            controlGrid.recordButtonUsage(noteUpButton);
        }
        
        // Test velocity parameter adjustment
        if (mapping.velocityUpButton != 0xFF) {
            uint8_t velocityUpButton = mapping.velocityUpButton;
            INFO("Testing velocity up button: " << (int)velocityUpButton);
            
            auto paramType = controlGrid.getParameterType(velocityUpButton, mapping);
            REQUIRE(paramType == ParameterLockPool::ParameterType::VELOCITY);
            
            int8_t adjustment = controlGrid.getParameterAdjustment(velocityUpButton, mapping);
            REQUIRE(adjustment > 0);
            
            // Apply adjustment
            uint8_t originalVelocity = lock.velocity;
            lock.setParameter(ParameterLockPool::ParameterType::VELOCITY, true);
            lock.velocity = std::min(127, static_cast<int>(lock.velocity) + adjustment);
            
            REQUIRE(lock.hasParameter(ParameterLockPool::ParameterType::VELOCITY));
            INFO("Velocity parameter adjusted from " << (int)originalVelocity << " to " << (int)lock.velocity);
            
            // Record button usage
            controlGrid.recordButtonUsage(velocityUpButton);
        }
        
        // === PHASE 7: Simulate Control Button Interaction ===
        INFO("Phase 7: Simulating control button interactions...");
        
        // Find some control buttons and simulate their usage
        std::vector<uint8_t> controlButtons;
        for (uint8_t button = 0; button < 32; ++button) {
            if (mapping.isInControlArea(button)) {
                controlButtons.push_back(button);
            }
        }
        
        REQUIRE(!controlButtons.empty());
        INFO("Found " << controlButtons.size() << " control buttons");
        
        // Simulate pressing and releasing a few control buttons
        for (size_t i = 0; i < std::min<size_t>(3, controlButtons.size()); ++i) {
            uint8_t controlButton = controlButtons[i];
            
            // Simulate button press
            buttonInput.pressButton(controlButton);
            timeSource.advance(50);
            system.update();
            
            // Record usage
            controlGrid.recordButtonUsage(controlButton);
            
            // Simulate button release
            buttonInput.releaseButton(controlButton);
            timeSource.advance(50);
            system.update();
            
            INFO("Simulated interaction with control button " << (int)controlButton);
        }
        
        // === PHASE 8: Test State Persistence ===
        INFO("Phase 8: Testing state persistence...");
        
        // Update system to ensure everything is still valid
        timeSource.advance(100);
        system.update();
        
        REQUIRE(stateManager.isInParameterLockMode());
        REQUIRE(buttonTracker.isHeld(TEST_BUTTON));
        REQUIRE(lockPool.isValidIndex(lockIndex));
        REQUIRE(lock.isValid());
        INFO("System state remains stable during parameter editing");
        
        // === PHASE 9: Exit Parameter Lock Mode ===
        INFO("Phase 9: Exiting parameter lock mode...");
        
        // Release the held step button (simulate 'r' key press)
        buttonInput.simulateKeyRelease('r');
        system.update();
        
        REQUIRE_FALSE(buttonTracker.isPressed(TEST_BUTTON));
        REQUIRE(buttonTracker.wasReleased(TEST_BUTTON));
        REQUIRE_FALSE(buttonTracker.isHeld(TEST_BUTTON));
        INFO("Step button released successfully");
        
        // Exit parameter lock mode
        auto exitResult = stateManager.exitParameterLockMode();
        REQUIRE(exitResult == SequencerStateManager::TransitionResult::SUCCESS);
        REQUIRE_FALSE(stateManager.isInParameterLockMode());
        REQUIRE(stateManager.getCurrentMode() == SequencerStateManager::Mode::NORMAL);
        INFO("Parameter lock mode exited successfully");
        
        // === PHASE 10: Verify Data Persistence ===
        INFO("Phase 10: Verifying data persistence...");
        
        // Parameter lock should still exist and be valid
        REQUIRE(lockPool.isValidIndex(lockIndex));
        REQUIRE(lock.inUse);
        REQUIRE(lock.isValid());
        REQUIRE(lock.trackIndex == TEST_TRACK);
        REQUIRE(lock.stepIndex == TEST_STEP);
        
        // Should be able to find it by position
        uint8_t foundIndex = lockPool.findLock(TEST_TRACK, TEST_STEP);
        REQUIRE(foundIndex == lockIndex);
        INFO("Parameter lock data persisted correctly");
        
        // Check if any parameters were set
        bool hasParameters = lock.hasParameter(ParameterLockPool::ParameterType::NOTE) ||
                           lock.hasParameter(ParameterLockPool::ParameterType::VELOCITY);
        if (hasParameters) {
            INFO("Parameter lock contains active parameters as expected");
        }
        
        // === PHASE 11: Verify Learning Data ===
        INFO("Phase 11: Verifying learning data...");
        
        const auto& usageStats = controlGrid.getUsageStats();
        REQUIRE(usageStats.totalUsage > 0);
        INFO("Control grid recorded " << usageStats.totalUsage << " button interactions");
        
        const auto& learningStats = buttonTracker.getLearningStats();
        // Learning stats might not be updated unless explicitly recorded
        INFO("Button tracker learning stats: " << learningStats.samples << " samples");
        
        // === PHASE 12: Performance Metrics ===
        INFO("Phase 12: Performance metrics...");
        
        uint32_t totalTestTime = timeSource.getCurrentTime() - testStartTime;
        INFO("Total test execution time: " << totalTestTime << "ms");
        
        auto poolStats = lockPool.getStats();
        INFO("Pool utilization: " << poolStats.utilization << " (" << poolStats.usedSlots << "/" << poolStats.totalSlots << " slots)");
        
        // === CLEANUP ===
        INFO("Cleanup: Deallocating parameter lock...");
        lockPool.deallocate(lockIndex);
        REQUIRE_FALSE(lockPool.isValidIndex(lockIndex));
        REQUIRE(lockPool.isEmpty());
        
        INFO("=== PARAMETER LOCK END-TO-END TEST COMPLETED SUCCESSFULLY ===");
    }
}

TEST_CASE("Parameter Lock End-to-End - Multiple simultaneous locks", "[end_to_end][parameter_lock][multiple]") {
    ParameterLockTestSystem system;
    
    SECTION("Create and manage multiple parameter locks") {
        struct TestStep {
            uint8_t track, step, buttonIndex;
            char pressKey, releaseKey;
        };
        
        TestStep testSteps[] = {
            {0, 1, 1, '2', '@'},   // Button 1
            {1, 4, 12, 'T', 't'},  // Button 12
            {2, 6, 22, 'J', 'j'},  // Button 22
            {3, 2, 26, 'C', 'c'}   // Button 26
        };
        
        std::vector<uint8_t> lockIndices;
        
        auto& timeSource = system.getTimeSource();
        auto& stateManager = system.getStateManager();
        auto& lockPool = system.getLockPool();
        
        INFO("Testing multiple parameter locks creation and management");
        
        // Create locks for each step
        for (const auto& testStep : testSteps) {
            INFO("Creating lock for Track " << (int)testStep.track << ", Step " << (int)testStep.step << " (Button " << (int)testStep.buttonIndex << ")");
            
            // Simulate button hold
            bool holdSuccessful = system.simulateButtonHold(testStep.buttonIndex, 500);
            REQUIRE(holdSuccessful);
            
            // Enter parameter lock mode
            auto result = stateManager.enterParameterLockMode(testStep.track, testStep.step);
            REQUIRE(result == SequencerStateManager::TransitionResult::SUCCESS);
            
            // Allocate parameter lock
            uint8_t lockIndex = lockPool.allocate(testStep.track, testStep.step);
            REQUIRE(lockIndex != ParameterLockPool::INVALID_INDEX);
            lockIndices.push_back(lockIndex);
            
            // Configure some parameters
            auto& lock = lockPool.getLock(lockIndex);
            lock.setParameter(ParameterLockPool::ParameterType::NOTE, true);
            lock.setParameter(ParameterLockPool::ParameterType::VELOCITY, true);
            lock.noteOffset = (testStep.step % 8) - 4; // Some variation
            lock.velocity = 90 + (testStep.track * 8);
            
            // Exit parameter lock mode
            system.simulateButtonRelease(testStep.buttonIndex);
            stateManager.exitParameterLockMode();
            
            timeSource.advance(200); // Pause between steps
        }
        
        // Verify all locks exist and are correct
        REQUIRE(lockIndices.size() == 4);
        REQUIRE(lockPool.getUsedCount() == 4);
        
        for (size_t i = 0; i < testSteps.size(); ++i) {
            const auto& testStep = testSteps[i];
            uint8_t lockIndex = lockIndices[i];
            
            REQUIRE(lockPool.isValidIndex(lockIndex));
            
            const auto& lock = lockPool.getLock(lockIndex);
            REQUIRE(lock.trackIndex == testStep.track);
            REQUIRE(lock.stepIndex == testStep.step);
            REQUIRE(lock.hasParameter(ParameterLockPool::ParameterType::NOTE));
            REQUIRE(lock.hasParameter(ParameterLockPool::ParameterType::VELOCITY));
            
            // Verify lookup works
            uint8_t foundIndex = lockPool.findLock(testStep.track, testStep.step);
            REQUIRE(foundIndex == lockIndex);
            
            INFO("Lock " << i << " verified: Track " << (int)lock.trackIndex << ", Step " << (int)lock.stepIndex << ", Note " << (int)lock.noteOffset << ", Velocity " << (int)lock.velocity);
        }
        
        // Test pool statistics
        auto stats = lockPool.getStats();
        REQUIRE(stats.usedSlots == 4);
        REQUIRE(stats.totalAllocations == 4);
        REQUIRE(stats.integrityValid);
        
        INFO("Pool statistics: " << stats.usedSlots << "/" << stats.totalSlots << " used, utilization: " << stats.utilization);
        
        // Cleanup all locks
        for (uint8_t lockIndex : lockIndices) {
            lockPool.deallocate(lockIndex);
        }
        
        REQUIRE(lockPool.isEmpty());
        INFO("All locks cleaned up successfully");
    }
}

TEST_CASE("Parameter Lock End-to-End - Error conditions and recovery", "[end_to_end][parameter_lock][error_handling]") {
    ParameterLockTestSystem system;
    
    SECTION("Handle timeout scenarios") {
        const uint32_t SHORT_TIMEOUT = 1000; // 1 second
        
        auto& timeSource = system.getTimeSource();
        auto& stateManager = system.getStateManager();
        auto& buttonTracker = system.getButtonTracker();
        
        stateManager.setParameterLockTimeout(SHORT_TIMEOUT);
        
        INFO("Testing parameter lock timeout handling");
        
        // Enter parameter lock mode
        bool holdSuccessful = system.simulateButtonHold(5, 500); // Button 5
        REQUIRE(holdSuccessful);
        
        auto result = stateManager.enterParameterLockMode(0, 5);
        REQUIRE(result == SequencerStateManager::TransitionResult::SUCCESS);
        REQUIRE(stateManager.isInParameterLockMode());
        
        uint32_t startTime = timeSource.getCurrentTime();
        INFO("Entered parameter lock mode at time " << startTime);
        
        // Wait for timeout
        timeSource.advance(SHORT_TIMEOUT + 100);
        system.update();
        
        // Should have exited due to timeout
        REQUIRE_FALSE(stateManager.isInParameterLockMode());
        REQUIRE(stateManager.getCurrentMode() == SequencerStateManager::Mode::NORMAL);
        
        INFO("Parameter lock mode correctly timed out after " << SHORT_TIMEOUT << "ms");
        
        // Clean up button state
        system.simulateButtonRelease(5);
    }
    
    SECTION("Handle pool exhaustion gracefully") {
        auto& lockPool = system.getLockPool();
        
        INFO("Testing parameter lock pool exhaustion");
        
        std::vector<uint8_t> indices;
        
        // Fill the pool to capacity
        for (size_t i = 0; i < ParameterLockPool::MAX_LOCKS; ++i) {
            uint8_t track = i % 4;
            uint8_t step = (i / 4) % 8;
            uint8_t index = lockPool.allocate(track, step);
            
            if (index != ParameterLockPool::INVALID_INDEX) {
                indices.push_back(index);
            }
        }
        
        REQUIRE(lockPool.isFull());
        INFO("Pool filled to capacity with " << indices.size() << " locks");
        
        // Try to allocate one more (should fail gracefully)
        uint8_t overflowIndex = lockPool.allocate(0, 0);
        REQUIRE(overflowIndex == ParameterLockPool::INVALID_INDEX);
        
        auto stats = lockPool.getStats();
        REQUIRE(stats.failedAllocations > 0);
        INFO("Pool correctly rejected overflow allocation");
        
        // System should still be functional
        auto& stateManager = system.getStateManager();
        auto result = stateManager.enterParameterLockMode(1, 3);
        REQUIRE(result == SequencerStateManager::TransitionResult::SUCCESS);
        
        stateManager.forceExitToNormal();
        INFO("State manager remains functional despite pool exhaustion");
        
        // Cleanup
        for (uint8_t index : indices) {
            lockPool.deallocate(index);
        }
    }
}

TEST_CASE("Parameter Lock End-to-End - Performance benchmarking", "[end_to_end][parameter_lock][performance]") {
    ParameterLockTestSystem system;
    
    SECTION("Measure operation timing") {
        const int OPERATION_COUNT = 100;
        
        auto& timeSource = system.getTimeSource();
        auto& stateManager = system.getStateManager();
        auto& lockPool = system.getLockPool();
        
        INFO("Benchmarking parameter lock operations (" << OPERATION_COUNT << " iterations)");
        
        uint32_t totalOperationTime = 0;
        
        for (int i = 0; i < OPERATION_COUNT; ++i) {
            uint8_t track = i % 4;
            uint8_t step = (i / 4) % 8;
            uint8_t buttonIndex = track * 8 + step;
            
            uint32_t operationStart = timeSource.getCurrentTime();
            
            // Complete parameter lock cycle
            bool holdSuccessful = system.simulateButtonHold(buttonIndex, 500);
            REQUIRE(holdSuccessful);
            
            auto enterResult = stateManager.enterParameterLockMode(track, step);
            REQUIRE(enterResult == SequencerStateManager::TransitionResult::SUCCESS);
            
            uint8_t lockIndex = lockPool.allocate(track, step);
            REQUIRE(lockIndex != ParameterLockPool::INVALID_INDEX);
            
            // Simulate some parameter editing
            auto& lock = lockPool.getLock(lockIndex);
            lock.setParameter(ParameterLockPool::ParameterType::NOTE, true);
            lock.noteOffset = (i % 24) - 12;
            
            // Exit
            system.simulateButtonRelease(buttonIndex);
            stateManager.exitParameterLockMode();
            
            lockPool.deallocate(lockIndex);
            
            uint32_t operationEnd = timeSource.getCurrentTime();
            uint32_t operationDuration = operationEnd - operationStart;
            totalOperationTime += operationDuration;
            
            // Small gap between operations
            timeSource.advance(10);
        }
        
        uint32_t averageOperationTime = totalOperationTime / OPERATION_COUNT;
        
        INFO("Performance results:");
        INFO("  Total time: " << totalOperationTime << "ms");
        INFO("  Average per operation: " << averageOperationTime << "ms");
        INFO("  Operations per second: " << (1000.0f / averageOperationTime));
        
        // Performance should be reasonable (less than 50ms per complete operation)
        REQUIRE(averageOperationTime < 50);
        
        // Verify final state is clean
        REQUIRE(lockPool.isEmpty());
        REQUIRE(stateManager.getCurrentMode() == SequencerStateManager::Mode::NORMAL);
    }
    
    SECTION("Memory usage validation") {
        auto& lockPool = system.getLockPool();
        auto& controlGrid = system.getControlGrid();
        
        INFO("Validating memory usage and pool integrity");
        
        // Create and destroy many locks to test for memory leaks
        const int CYCLE_COUNT = 200;
        
        for (int cycle = 0; cycle < CYCLE_COUNT; ++cycle) {
            std::vector<uint8_t> tempIndices;
            
            // Allocate several locks
            for (int i = 0; i < 10; ++i) {
                uint8_t track = (cycle + i) % 4;
                uint8_t step = (cycle * 2 + i) % 8;
                uint8_t index = lockPool.allocate(track, step);
                if (index != ParameterLockPool::INVALID_INDEX) {
                    tempIndices.push_back(index);
                }
            }
            
            // Use control grid
            auto mapping = controlGrid.getMapping(cycle % 8, cycle % 4);
            REQUIRE(mapping.isValid);
            
            // Deallocate locks
            for (uint8_t index : tempIndices) {
                lockPool.deallocate(index);
            }
            
            // Validate integrity periodically
            if (cycle % 50 == 0) {
                REQUIRE(lockPool.validateIntegrity());
            }
        }
        
        // Final validation
        REQUIRE(lockPool.isEmpty());
        REQUIRE(lockPool.validateIntegrity());
        
        auto finalStats = lockPool.getStats();
        INFO("Final pool stats:");
        INFO("  Total allocations: " << finalStats.totalAllocations);
        INFO("  Total deallocations: " << finalStats.totalDeallocations);
        INFO("  Failed allocations: " << finalStats.failedAllocations);
        INFO("  Final utilization: " << finalStats.utilization);
        
        REQUIRE(finalStats.integrityValid);
    }
}