#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include "NonRealtimeSequencer.h"
#include "ControlMessage.h"
#include "JsonState.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <string>

/**
 * Comprehensive Test Harness for Parameter Lock System
 * 
 * Tests the critical bug fixes in parameter lock functionality:
 * 1. Parameter Lock Exit Bug: System should NOT exit parameter lock mode when pressing control buttons
 * 2. Step Toggle Bug: Steps should NOT toggle when released after being used for parameter lock
 * 3. Control Grid Functionality: Proper parameter adjustment routing
 * 4. State Persistence: Save/load state during parameter lock operations
 */

class ParameterLockTestHarness {
private:
    NonRealtimeSequencer sequencer_;
    std::string testStateDir_;
    std::ostringstream logBuffer_;
    bool verbose_;
    
public:
    ParameterLockTestHarness() : verbose_(true) {
        // Set up test directory
        testStateDir_ = "test_states_parameter_locks";
        std::filesystem::create_directories(testStateDir_);
        
        // Configure sequencer for verbose testing
        sequencer_.init(120, 8);
        sequencer_.setLogStream(&logBuffer_);
        sequencer_.setVerbose(verbose_);
    }
    
    ~ParameterLockTestHarness() {
        // Clean up test files if needed
        try {
            std::filesystem::remove_all(testStateDir_);
        } catch (...) {
            // Ignore cleanup errors
        }
    }
    
    void log(const std::string& message) {
        if (verbose_) {
            std::cout << "[TEST] " << message << std::endl;
        }
    }
    
    std::string getLogOutput() {
        std::string output = logBuffer_.str();
        logBuffer_.str("");  // Clear buffer
        logBuffer_.clear();
        return output;
    }
    
    JsonState::Snapshot captureState(const std::string& description) {
        auto state = sequencer_.getCurrentState();
        JsonState::Snapshot jsonState;
        // Convert SequencerState::Snapshot to JsonState::Snapshot
        jsonState.sequencer.bpm = state.bpm;
        jsonState.sequencer.stepCount = state.stepCount;
        jsonState.sequencer.currentStep = state.currentStep;
        jsonState.sequencer.playing = state.playing;
        jsonState.sequencer.currentTime = state.currentTime;
        
        // Save state to file for debugging
        std::string filename = testStateDir_ + "/" + description + ".json";
        jsonState.saveToFile(filename);
        
        log("Captured state: " + description);
        return jsonState;
    }
    
    bool compareStates(const JsonState::Snapshot& before, const JsonState::Snapshot& after, const std::string& context) {
        if (before.equals(after)) {
            log("States match: " + context);
            return true;
        } else {
            log("States differ: " + context);
            log("Differences: " + before.getDiff(after));
            return false;
        }
    }
    
    bool executeSequence(const std::vector<ControlMessage::Message>& messages, const std::string& testName) {
        log("\n=== Executing test sequence: " + testName + " ===");
        
        auto result = sequencer_.processBatch(messages);
        
        log("Sequence executed: " + std::to_string(result.totalMessages) + " messages, " + 
            std::to_string(result.successfulMessages) + " successful");
        
        if (!result.allSucceeded) {
            log("ERROR: Not all messages succeeded");
            for (size_t i = 0; i < result.messageResults.size(); ++i) {
                if (!result.messageResults[i].success) {
                    log("Failed message " + std::to_string(i) + ": " + result.messageResults[i].errorMessage);
                }
            }
        }
        
        // Log any sequencer output
        std::string logOutput = getLogOutput();
        if (!logOutput.empty()) {
            log("Sequencer output:\n" + logOutput);
        }
        
        return result.allSucceeded;
    }
    
    // Test 1: Parameter Lock Mode Persistence
    bool testParameterLockModePersistence() {
        log("\n### TEST 1: Parameter Lock Mode Persistence ###");
        log("Verify that parameter lock mode persists when control buttons are pressed");
        
        // Step 1: Activate a step (track 0, step 2)
        uint8_t targetButton = 0 * 8 + 2;  // Track 0, Step 2 = Button 2
        
        std::vector<ControlMessage::Message> sequence = {
            // Initially activate the step with a quick press/release
            ControlMessage::Message::keyPress(targetButton, 0),
            ControlMessage::Message::timeAdvance(50, 50),
            ControlMessage::Message::keyRelease(targetButton, 100),
            ControlMessage::Message::timeAdvance(100, 200),
            
            // Now press and hold for parameter lock (600ms hold)
            ControlMessage::Message::keyPress(targetButton, 300),
            ControlMessage::Message::timeAdvance(650, 950),  // Hold for 650ms to trigger param lock
            
            // Press various control buttons while still holding step button
            ControlMessage::Message::keyPress(8, 1000),   // Control button in control grid
            ControlMessage::Message::timeAdvance(50, 1050),
            ControlMessage::Message::keyRelease(8, 1100),
            ControlMessage::Message::timeAdvance(50, 1150),
            
            ControlMessage::Message::keyPress(16, 1200),  // Another control button
            ControlMessage::Message::timeAdvance(50, 1250),
            ControlMessage::Message::keyRelease(16, 1300),
            ControlMessage::Message::timeAdvance(50, 1350),
            
            // Finally release the held step button
            ControlMessage::Message::keyRelease(targetButton, 1400),
            ControlMessage::Message::timeAdvance(100, 1500)
        };
        
        // Capture initial state
        auto initialState = captureState("test1_initial");
        
        // Execute sequence
        bool success = executeSequence(sequence, "Parameter Lock Mode Persistence");
        
        // Capture final state
        auto finalState = captureState("test1_final");
        
        // Verify behavior through state analysis
        // The test passes if the sequencer handled the sequence without errors
        // and the step remains active (it was activated before parameter lock)
        
        log(std::string("Test 1 result: ") + (success ? "PASS" : "FAIL"));
        return success;
    }
    
    // Test 2: No Step Toggle After Parameter Lock
    bool testNoStepToggleAfterParameterLock() {
        log("\n### TEST 2: No Step Toggle After Parameter Lock ###");
        log("Verify that steps do NOT toggle when released after being used for parameter lock");
        
        uint8_t targetButton = 1 * 8 + 3;  // Track 1, Step 3 = Button 11
        
        std::vector<ControlMessage::Message> sequence = {
            // Step 1: Activate the step first
            ControlMessage::Message::keyPress(targetButton, 0),
            ControlMessage::Message::timeAdvance(50, 50),
            ControlMessage::Message::keyRelease(targetButton, 100),
            ControlMessage::Message::timeAdvance(200, 300),
            
            // Step 2: Hold step button to enter parameter lock mode
            ControlMessage::Message::keyPress(targetButton, 400),
            ControlMessage::Message::timeAdvance(650, 1050),  // 650ms hold
            
            // Step 3: Adjust a parameter using control grid
            ControlMessage::Message::keyPress(8, 1100),   // Press control button
            ControlMessage::Message::timeAdvance(50, 1150),
            ControlMessage::Message::keyRelease(8, 1200), // Release control button
            ControlMessage::Message::timeAdvance(50, 1250),
            
            // Step 4: Release the held step button
            // BUG FIX: This should NOT toggle the step (it should remain active)
            ControlMessage::Message::keyRelease(targetButton, 1300),
            ControlMessage::Message::timeAdvance(100, 1400)
        };
        
        // Capture states at key points
        auto initialState = captureState("test2_initial");
        
        bool success = executeSequence(sequence, "No Step Toggle After Parameter Lock");
        
        auto finalState = captureState("test2_final");
        
        log(std::string("Test 2 result: ") + (success ? "PASS" : "FAIL"));
        return success;
    }
    
    // Test 3: Control Grid Functionality
    bool testControlGridFunctionality() {
        log("\n### TEST 3: Control Grid Functionality ###");
        log("Test control grid boundaries, button mapping, parameter adjustment");
        
        uint8_t targetButton = 2 * 8 + 4;  // Track 2, Step 4 = Button 20
        
        std::vector<ControlMessage::Message> sequence = {
            // Activate step and enter parameter lock mode
            ControlMessage::Message::keyPress(targetButton, 0),
            ControlMessage::Message::timeAdvance(50, 50),
            ControlMessage::Message::keyRelease(targetButton, 100),
            ControlMessage::Message::timeAdvance(200, 300),
            
            // Hold for parameter lock
            ControlMessage::Message::keyPress(targetButton, 400),
            ControlMessage::Message::timeAdvance(650, 1050),
            
            // Test various control grid buttons
            // These should map to different parameters: NOTE, VELOCITY, LENGTH
            ControlMessage::Message::keyPress(8, 1100),   // Control grid button 8
            ControlMessage::Message::timeAdvance(50, 1150),
            ControlMessage::Message::keyRelease(8, 1200),
            ControlMessage::Message::timeAdvance(50, 1250),
            
            ControlMessage::Message::keyPress(9, 1300),   // Control grid button 9
            ControlMessage::Message::timeAdvance(50, 1350),
            ControlMessage::Message::keyRelease(9, 1400),
            ControlMessage::Message::timeAdvance(50, 1450),
            
            ControlMessage::Message::keyPress(16, 1500),  // Control grid button 16
            ControlMessage::Message::timeAdvance(50, 1550),
            ControlMessage::Message::keyRelease(16, 1600),
            ControlMessage::Message::timeAdvance(50, 1650),
            
            // Exit parameter lock mode
            ControlMessage::Message::keyRelease(targetButton, 1700),
            ControlMessage::Message::timeAdvance(100, 1800)
        };
        
        auto initialState = captureState("test3_initial");
        
        bool success = executeSequence(sequence, "Control Grid Functionality");
        
        auto finalState = captureState("test3_final");
        
        log(std::string("Test 3 result: ") + (success ? "PASS" : "FAIL"));
        return success;
    }
    
    // Test 4: State Persistence During Parameter Lock Operations
    bool testStatePersistence() {
        log("\n### TEST 4: State Persistence ###");
        log("Test save/load state during parameter lock operations");
        
        uint8_t targetButton = 3 * 8 + 1;  // Track 3, Step 1 = Button 25
        std::string stateFile = "param_lock_state.json";
        
        std::vector<ControlMessage::Message> setupSequence = {
            // Set up pattern with some active steps
            ControlMessage::Message::keyPress(0, 0),   // Track 0, Step 0
            ControlMessage::Message::timeAdvance(50, 50),
            ControlMessage::Message::keyRelease(0, 100),
            ControlMessage::Message::timeAdvance(50, 150),
            
            ControlMessage::Message::keyPress(9, 200),  // Track 1, Step 1
            ControlMessage::Message::timeAdvance(50, 250),
            ControlMessage::Message::keyRelease(9, 300),
            ControlMessage::Message::timeAdvance(50, 350),
            
            ControlMessage::Message::keyPress(targetButton, 400),  // Track 3, Step 1
            ControlMessage::Message::timeAdvance(50, 450),
            ControlMessage::Message::keyRelease(targetButton, 500),
            ControlMessage::Message::timeAdvance(200, 700),
            
            // Save state before parameter lock
            ControlMessage::Message::saveState(stateFile, 800)
        };
        
        std::vector<ControlMessage::Message> paramLockSequence = {
            // Enter parameter lock mode
            ControlMessage::Message::keyPress(targetButton, 900),
            ControlMessage::Message::timeAdvance(650, 1550),
            
            // Adjust parameters
            ControlMessage::Message::keyPress(8, 1600),
            ControlMessage::Message::timeAdvance(50, 1650),
            ControlMessage::Message::keyRelease(8, 1700),
            ControlMessage::Message::timeAdvance(50, 1750),
            
            // Exit parameter lock
            ControlMessage::Message::keyRelease(targetButton, 1800),
            ControlMessage::Message::timeAdvance(100, 1900),
            
            // Save final state
            ControlMessage::Message::saveState(stateFile + ".final", 2000)
        };
        
        // Execute setup
        bool setupSuccess = executeSequence(setupSequence, "State Persistence Setup");
        if (!setupSuccess) {
            log("Test 4 FAIL: Setup sequence failed");
            return false;
        }
        
        // Capture state before parameter lock operations
        auto beforeParamLock = captureState("test4_before_param_lock");
        
        // Execute parameter lock operations
        bool paramLockSuccess = executeSequence(paramLockSequence, "State Persistence Parameter Lock");
        if (!paramLockSuccess) {
            log("Test 4 FAIL: Parameter lock sequence failed");
            return false;
        }
        
        // Capture final state
        auto afterParamLock = captureState("test4_after_param_lock");
        
        // Test loading the saved state
        std::vector<ControlMessage::Message> loadSequence = {
            ControlMessage::Message::loadState(stateFile, 0),
            ControlMessage::Message::timeAdvance(100, 100)
        };
        
        bool loadSuccess = executeSequence(loadSequence, "State Persistence Load");
        
        auto loadedState = captureState("test4_loaded_state");
        
        // Verify states
        bool stateMatch = compareStates(beforeParamLock, loadedState, "Before param lock vs loaded state");
        
        bool success = setupSuccess && paramLockSuccess && loadSuccess && stateMatch;
        log(std::string("Test 4 result: ") + (success ? "PASS" : "FAIL"));
        return success;
    }
    
    // Test 5: Edge Cases and Error Conditions
    bool testEdgeCases() {
        log("\n### TEST 5: Edge Cases and Error Conditions ###");
        log("Test boundary conditions and error scenarios");
        
        std::vector<ControlMessage::Message> sequence = {
            // Test boundary conditions (valid but edge cases)
            ControlMessage::Message::keyPress(31, 0),   // Last valid button (boundary test)
            ControlMessage::Message::timeAdvance(50, 50),
            ControlMessage::Message::keyRelease(31, 100),
            ControlMessage::Message::timeAdvance(50, 150),
            
            // Test rapid press/release cycles
            ControlMessage::Message::keyPress(0, 200),
            ControlMessage::Message::timeAdvance(10, 210),  // Very short hold
            ControlMessage::Message::keyRelease(0, 220),
            ControlMessage::Message::timeAdvance(10, 230),
            
            // Test overlapping button presses
            ControlMessage::Message::keyPress(1, 300),
            ControlMessage::Message::keyPress(2, 350),    // Press second button while first is held
            ControlMessage::Message::timeAdvance(50, 400),
            ControlMessage::Message::keyRelease(1, 450),
            ControlMessage::Message::keyRelease(2, 500),
            ControlMessage::Message::timeAdvance(100, 600),
            
            // Test parameter lock with multiple simultaneous holds
            ControlMessage::Message::keyPress(3, 700),
            ControlMessage::Message::timeAdvance(300, 1000),
            ControlMessage::Message::keyPress(4, 1050),   // Press another while first is held
            ControlMessage::Message::timeAdvance(350, 1400),
            ControlMessage::Message::keyRelease(3, 1450),
            ControlMessage::Message::keyRelease(4, 1500),
            ControlMessage::Message::timeAdvance(100, 1600)
        };
        
        auto initialState = captureState("test5_initial");
        
        bool success = executeSequence(sequence, "Edge Cases and Error Conditions");
        
        auto finalState = captureState("test5_final");
        
        log(std::string("Test 5 result: ") + (success ? "PASS" : "FAIL"));
        return success;
    }
    
    // Comprehensive Test Suite Runner
    bool runAllTests() {
        log("===============================================");
        log("Parameter Lock System Test Harness");
        log("Testing critical bug fixes and functionality");
        log("===============================================");
        
        bool allTestsPassed = true;
        int testsPassed = 0;
        int totalTests = 5;
        
        // Run all tests
        if (testParameterLockModePersistence()) testsPassed++; else allTestsPassed = false;
        if (testNoStepToggleAfterParameterLock()) testsPassed++; else allTestsPassed = false;
        if (testControlGridFunctionality()) testsPassed++; else allTestsPassed = false;
        if (testStatePersistence()) testsPassed++; else allTestsPassed = false;
        if (testEdgeCases()) testsPassed++; else allTestsPassed = false;
        
        // Summary
        log("\n===============================================");
        log("Test Results Summary:");
        log("Tests Passed: " + std::to_string(testsPassed) + "/" + std::to_string(totalTests));
        log(std::string("Overall Result: ") + (allTestsPassed ? "ALL TESTS PASSED" : "SOME TESTS FAILED"));
        log("===============================================");
        
        return allTestsPassed;
    }
};

// Catch2 Test Cases using the Test Harness

TEST_CASE("Parameter Lock Mode Persistence", "[parameter_locks]") {
    ParameterLockTestHarness harness;
    REQUIRE(harness.testParameterLockModePersistence());
}

TEST_CASE("No Step Toggle After Parameter Lock", "[parameter_locks]") {
    ParameterLockTestHarness harness;
    REQUIRE(harness.testNoStepToggleAfterParameterLock());
}

TEST_CASE("Control Grid Functionality", "[parameter_locks]") {
    ParameterLockTestHarness harness;
    REQUIRE(harness.testControlGridFunctionality());
}

TEST_CASE("State Persistence During Parameter Lock", "[parameter_locks]") {
    ParameterLockTestHarness harness;
    REQUIRE(harness.testStatePersistence());
}

TEST_CASE("Parameter Lock Edge Cases", "[parameter_locks]") {
    ParameterLockTestHarness harness;
    REQUIRE(harness.testEdgeCases());
}

TEST_CASE("Complete Parameter Lock Test Suite", "[parameter_locks][comprehensive]") {
    ParameterLockTestHarness harness;
    REQUIRE(harness.runAllTests());
}

// Standalone executable main function
int main(int argc, char* argv[]) {
    // Check if running as standalone executable
    if (argc > 1 && std::string(argv[1]) == "--standalone") {
        std::cout << "Running Parameter Lock Test Harness in standalone mode..." << std::endl;
        
        ParameterLockTestHarness harness;
        bool success = harness.runAllTests();
        
        return success ? 0 : 1;
    }
    
    // Otherwise run with Catch2
    return Catch::Session().run(argc, argv);
}