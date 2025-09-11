#include "ShiftBasedGestureDetector.h"
#include "InputStateProcessor.h"
#include "InputSystemConfiguration.h"
#include "ControlMessage.h"
#include "InputEvent.h"
#include "IClock.h"
#include "IDebugOutput.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>
#include <memory>

/**
 * @file test_shift_based_gesture_detector.cpp
 * @brief Unit tests for ShiftBasedGestureDetector Phase 2 implementation
 * 
 * Tests the SHIFT-based parameter lock system with bank-aware controls:
 * - SHIFT + button press → enter parameter lock mode
 * - Bank-aware control mapping (left triggers right, right triggers left)
 * - Control button presses → parameter adjustments
 * - Lock button press → exit parameter lock mode
 * - 64-bit state encoding integration
 */

/**
 * @brief Test clock for controlled timing
 */
class TestClock : public IClock {
private:
    uint32_t time_ = 0;

public:
    uint32_t getCurrentTime() const override { return time_; }
    void delay(uint32_t ms) override { time_ += ms; }
    void reset() override { time_ = 0; }
    
    void setTime(uint32_t time) { time_ = time; }
    void advance(uint32_t ms) { time_ += ms; }
};

/**
 * @brief Test debug output for capturing debug messages
 */
class TestDebugOutput : public IDebugOutput {
public:
    std::vector<std::string> messages;
    
    void log(const std::string& message) override {
        messages.push_back(message);
    }
    
    void logf(const char* format, ...) override {
        va_list args;
        va_start(args, format);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        log(std::string(buffer));
    }
    
    void clear() { messages.clear(); }
};

/**
 * @brief Test fixture for ShiftBasedGestureDetector tests
 */
class ShiftBasedGestureDetectorTest {
public:
    TestClock clock_;
    TestDebugOutput debug_;
    std::unique_ptr<InputStateProcessor> stateProcessor_;
    std::unique_ptr<ShiftBasedGestureDetector> gestureDetector_;
    InputSystemConfiguration config_;
    
    ShiftBasedGestureDetectorTest() {
        // Initialize dependencies
        InputStateProcessor::Dependencies processorDeps;
        processorDeps.clock = &clock_;
        processorDeps.debugOutput = &debug_;
        
        stateProcessor_ = std::make_unique<InputStateProcessor>(processorDeps);
        gestureDetector_ = std::make_unique<ShiftBasedGestureDetector>(stateProcessor_.get());
        
        // Configure for testing
        config_ = InputSystemConfiguration::forTesting();
        gestureDetector_->setConfiguration(config_);
    }
    
    /**
     * @brief Process input event and return generated messages
     */
    std::vector<ControlMessage::Message> processEvent(const InputEvent& event) {
        std::vector<ControlMessage::Message> messages;
        gestureDetector_->processInputEvent(event, messages);
        return messages;
    }
    
    /**
     * @brief Create button press event
     */
    InputEvent buttonPress(uint8_t buttonId, uint32_t timestamp = 100) {
        return InputEvent::buttonPress(buttonId, timestamp);
    }
    
    /**
     * @brief Create button release event
     */
    InputEvent buttonRelease(uint8_t buttonId, uint32_t timestamp = 200, uint32_t duration = 100) {
        return InputEvent::buttonRelease(buttonId, timestamp, duration);
    }
    
    /**
     * @brief Create SHIFT + button press event
     */
    InputEvent shiftButtonPress(uint8_t buttonId, uint32_t timestamp = 100, uint8_t bankId = 0) {
        return InputEvent::shiftButtonPress(buttonId, timestamp, bankId);
    }
    
    /**
     * @brief Create SHIFT + button release event
     */
    InputEvent shiftButtonRelease(uint8_t buttonId, uint32_t timestamp = 200, uint32_t duration = 100, uint8_t bankId = 0) {
        return InputEvent::shiftButtonRelease(buttonId, timestamp, duration, bankId);
    }
};

TEST_CASE("ShiftBasedGestureDetector - Basic initialization", "[shift-gesture]") {
    ShiftBasedGestureDetectorTest test;
    
    SECTION("Initial state") {
        REQUIRE_FALSE(test.gestureDetector_->isInParameterLockMode());
        
        // Check button states
        bool buttonStates[32];
        uint8_t numButtons = test.gestureDetector_->getCurrentButtonStates(buttonStates, 32);
        REQUIRE(numButtons == 32);
        
        // All buttons should be released initially
        for (int i = 0; i < 32; ++i) {
            REQUIRE_FALSE(buttonStates[i]);
        }
    }
    
    SECTION("Reset functionality") {
        // Put detector in some state first
        auto messages = test.processEvent(test.shiftButtonPress(2)); // Left bank button
        REQUIRE(test.gestureDetector_->isInParameterLockMode());
        
        // Reset should clear state
        test.gestureDetector_->reset();
        REQUIRE_FALSE(test.gestureDetector_->isInParameterLockMode());
    }
}

TEST_CASE("ShiftBasedGestureDetector - Bank detection", "[shift-gesture]") {
    ShiftBasedGestureDetectorTest test;
    
    SECTION("Left bank buttons (0-3, 8-11, 16-19, 24-27)") {
        // Test row 0: buttons 0-3
        for (uint8_t buttonId = 0; buttonId <= 3; ++buttonId) {
            auto messages = test.processEvent(test.shiftButtonPress(buttonId));
            REQUIRE(messages.size() == 1);
            REQUIRE(messages[0].type == ControlMessage::Type::ENTER_PARAM_LOCK);
            REQUIRE(test.gestureDetector_->isInParameterLockMode());
            test.gestureDetector_->reset();
        }
        
        // Test row 1: buttons 8-11
        for (uint8_t buttonId = 8; buttonId <= 11; ++buttonId) {
            auto messages = test.processEvent(test.shiftButtonPress(buttonId));
            REQUIRE(messages.size() == 1);
            REQUIRE(messages[0].type == ControlMessage::Type::ENTER_PARAM_LOCK);
            test.gestureDetector_->reset();
        }
        
        // Test row 2: buttons 16-19
        for (uint8_t buttonId = 16; buttonId <= 19; ++buttonId) {
            auto messages = test.processEvent(test.shiftButtonPress(buttonId));
            REQUIRE(messages.size() == 1);
            REQUIRE(messages[0].type == ControlMessage::Type::ENTER_PARAM_LOCK);
            test.gestureDetector_->reset();
        }
        
        // Test row 3: buttons 24-27
        for (uint8_t buttonId = 24; buttonId <= 27; ++buttonId) {
            auto messages = test.processEvent(test.shiftButtonPress(buttonId));
            REQUIRE(messages.size() == 1);
            REQUIRE(messages[0].type == ControlMessage::Type::ENTER_PARAM_LOCK);
            test.gestureDetector_->reset();
        }
    }
    
    SECTION("Right bank buttons (4-7, 12-15, 20-23, 28-31)") {
        // Test row 0: buttons 4-7
        for (uint8_t buttonId = 4; buttonId <= 7; ++buttonId) {
            auto messages = test.processEvent(test.shiftButtonPress(buttonId));
            REQUIRE(messages.size() == 1);
            REQUIRE(messages[0].type == ControlMessage::Type::ENTER_PARAM_LOCK);
            REQUIRE(test.gestureDetector_->isInParameterLockMode());
            test.gestureDetector_->reset();
        }
        
        // Test row 1: buttons 12-15
        for (uint8_t buttonId = 12; buttonId <= 15; ++buttonId) {
            auto messages = test.processEvent(test.shiftButtonPress(buttonId));
            REQUIRE(messages.size() == 1);
            REQUIRE(messages[0].type == ControlMessage::Type::ENTER_PARAM_LOCK);
            test.gestureDetector_->reset();
        }
        
        // Test row 2: buttons 20-23
        for (uint8_t buttonId = 20; buttonId <= 23; ++buttonId) {
            auto messages = test.processEvent(test.shiftButtonPress(buttonId));
            REQUIRE(messages.size() == 1);
            REQUIRE(messages[0].type == ControlMessage::Type::ENTER_PARAM_LOCK);
            test.gestureDetector_->reset();
        }
        
        // Test row 3: buttons 28-31
        for (uint8_t buttonId = 28; buttonId <= 31; ++buttonId) {
            auto messages = test.processEvent(test.shiftButtonPress(buttonId));
            REQUIRE(messages.size() == 1);
            REQUIRE(messages[0].type == ControlMessage::Type::ENTER_PARAM_LOCK);
            test.gestureDetector_->reset();
        }
    }
}

TEST_CASE("ShiftBasedGestureDetector - Parameter lock entry and exit", "[shift-gesture]") {
    ShiftBasedGestureDetectorTest test;
    
    SECTION("Enter parameter lock from left bank") {
        // SHIFT + button 2 (left bank) should enter parameter lock
        auto messages = test.processEvent(test.shiftButtonPress(2));
        
        REQUIRE(messages.size() == 1);
        REQUIRE(messages[0].type == ControlMessage::Type::ENTER_PARAM_LOCK);
        REQUIRE(messages[0].param1 == 0); // track 0
        REQUIRE(messages[0].param2 == 2); // step 2
        REQUIRE(test.gestureDetector_->isInParameterLockMode());
    }
    
    SECTION("Enter parameter lock from right bank") {
        // SHIFT + button 6 (right bank) should enter parameter lock
        auto messages = test.processEvent(test.shiftButtonPress(6));
        
        REQUIRE(messages.size() == 1);
        REQUIRE(messages[0].type == ControlMessage::Type::ENTER_PARAM_LOCK);
        REQUIRE(messages[0].param1 == 0); // track 0
        REQUIRE(messages[0].param2 == 6); // step 6
        REQUIRE(test.gestureDetector_->isInParameterLockMode());
    }
    
    SECTION("Exit parameter lock by pressing lock button") {
        // Enter parameter lock first
        test.processEvent(test.shiftButtonPress(2));
        REQUIRE(test.gestureDetector_->isInParameterLockMode());
        
        // Press the same button (2) to exit
        auto messages = test.processEvent(test.buttonPress(2));
        
        REQUIRE(messages.size() == 1);
        REQUIRE(messages[0].type == ControlMessage::Type::EXIT_PARAM_LOCK);
        REQUIRE_FALSE(test.gestureDetector_->isInParameterLockMode());
    }
}

TEST_CASE("ShiftBasedGestureDetector - Bank-aware control mapping", "[shift-gesture]") {
    ShiftBasedGestureDetectorTest test;
    
    SECTION("Left bank trigger → Right bank controls") {
        // Enter parameter lock from left bank (button 2)
        test.processEvent(test.shiftButtonPress(2));
        REQUIRE(test.gestureDetector_->isInParameterLockMode());
        
        // Test right bank control buttons
        // Right bank controls: b=note-(28), g=note+(20), n=velocity-(29), h=velocity+(21)
        
        // Test note- control (button 28 = 'b')
        auto messages = test.processEvent(test.buttonPress(28));
        REQUIRE(messages.size() == 1);
        REQUIRE(messages[0].type == ControlMessage::Type::ADJUST_PARAMETER);
        REQUIRE(messages[0].param1 == 0); // NOTE_MINUS
        REQUIRE(static_cast<int8_t>(messages[0].param2) == -1); // delta = -1
        
        // Test note+ control (button 20 = 'g')
        messages = test.processEvent(test.buttonPress(20));
        REQUIRE(messages.size() == 1);
        REQUIRE(messages[0].type == ControlMessage::Type::ADJUST_PARAMETER);
        REQUIRE(messages[0].param1 == 1); // NOTE_PLUS
        REQUIRE(static_cast<int8_t>(messages[0].param2) == 1); // delta = 1
        
        // Test velocity- control (button 29 = 'n')
        messages = test.processEvent(test.buttonPress(29));
        REQUIRE(messages.size() == 1);
        REQUIRE(messages[0].type == ControlMessage::Type::ADJUST_PARAMETER);
        REQUIRE(messages[0].param1 == 2); // VELOCITY_MINUS
        REQUIRE(static_cast<int8_t>(messages[0].param2) == -10); // delta = -10
        
        // Test velocity+ control (button 21 = 'h')
        messages = test.processEvent(test.buttonPress(21));
        REQUIRE(messages.size() == 1);
        REQUIRE(messages[0].type == ControlMessage::Type::ADJUST_PARAMETER);
        REQUIRE(messages[0].param1 == 3); // VELOCITY_PLUS
        REQUIRE(static_cast<int8_t>(messages[0].param2) == 10); // delta = 10
    }
    
    SECTION("Right bank trigger → Left bank controls") {
        // Enter parameter lock from right bank (button 6)
        test.processEvent(test.shiftButtonPress(6));
        REQUIRE(test.gestureDetector_->isInParameterLockMode());
        
        // Test left bank control buttons
        // Left bank controls: z=note-(24), a=note+(16), x=velocity-(25), s=velocity+(17)
        
        // Test note- control (button 24 = 'z')
        auto messages = test.processEvent(test.buttonPress(24));
        REQUIRE(messages.size() == 1);
        REQUIRE(messages[0].type == ControlMessage::Type::ADJUST_PARAMETER);
        REQUIRE(messages[0].param1 == 0); // NOTE_MINUS
        REQUIRE(static_cast<int8_t>(messages[0].param2) == -1); // delta = -1
        
        // Test note+ control (button 16 = 'a')
        messages = test.processEvent(test.buttonPress(16));
        REQUIRE(messages.size() == 1);
        REQUIRE(messages[0].type == ControlMessage::Type::ADJUST_PARAMETER);
        REQUIRE(messages[0].param1 == 1); // NOTE_PLUS
        REQUIRE(static_cast<int8_t>(messages[0].param2) == 1); // delta = 1
        
        // Test velocity- control (button 25 = 'x')
        messages = test.processEvent(test.buttonPress(25));
        REQUIRE(messages.size() == 1);
        REQUIRE(messages[0].type == ControlMessage::Type::ADJUST_PARAMETER);
        REQUIRE(messages[0].param1 == 2); // VELOCITY_MINUS
        REQUIRE(static_cast<int8_t>(messages[0].param2) == -10); // delta = -10
        
        // Test velocity+ control (button 17 = 's')
        messages = test.processEvent(test.buttonPress(17));
        REQUIRE(messages.size() == 1);
        REQUIRE(messages[0].type == ControlMessage::Type::ADJUST_PARAMETER);
        REQUIRE(messages[0].param1 == 3); // VELOCITY_PLUS
        REQUIRE(static_cast<int8_t>(messages[0].param2) == 10); // delta = 10
    }
}

TEST_CASE("ShiftBasedGestureDetector - Normal step toggle mode", "[shift-gesture]") {
    ShiftBasedGestureDetectorTest test;
    
    SECTION("Regular button press generates step toggle") {
        // Regular button press when not in parameter lock mode
        auto messages = test.processEvent(test.buttonPress(5));
        
        REQUIRE(messages.size() == 1);
        REQUIRE(messages[0].type == ControlMessage::Type::TOGGLE_STEP);
        REQUIRE(messages[0].param1 == 0); // track 0
        REQUIRE(messages[0].param2 == 5); // step 5
        REQUIRE_FALSE(test.gestureDetector_->isInParameterLockMode());
    }
    
    SECTION("Multiple button presses generate multiple toggles") {
        auto messages1 = test.processEvent(test.buttonPress(1));
        auto messages2 = test.processEvent(test.buttonPress(9)); // Track 1, step 1
        auto messages3 = test.processEvent(test.buttonPress(17)); // Track 2, step 1
        
        REQUIRE(messages1.size() == 1);
        REQUIRE(messages1[0].type == ControlMessage::Type::TOGGLE_STEP);
        REQUIRE(messages1[0].param1 == 0); // track 0
        REQUIRE(messages1[0].param2 == 1); // step 1
        
        REQUIRE(messages2.size() == 1);
        REQUIRE(messages2[0].type == ControlMessage::Type::TOGGLE_STEP);
        REQUIRE(messages2[0].param1 == 1); // track 1
        REQUIRE(messages2[0].param2 == 1); // step 1
        
        REQUIRE(messages3.size() == 1);
        REQUIRE(messages3[0].type == ControlMessage::Type::TOGGLE_STEP);
        REQUIRE(messages3[0].param1 == 2); // track 2
        REQUIRE(messages3[0].param2 == 1); // step 1
    }
}

TEST_CASE("ShiftBasedGestureDetector - Invalid operations", "[shift-gesture]") {
    ShiftBasedGestureDetectorTest test;
    
    SECTION("Non-control buttons ignored during parameter lock") {
        // Enter parameter lock from left bank (enables right bank controls)
        test.processEvent(test.shiftButtonPress(2));
        REQUIRE(test.gestureDetector_->isInParameterLockMode());
        
        // Press a button that's not a control button (button 10)
        auto messages = test.processEvent(test.buttonPress(10));
        
        // Should generate no messages (ignored)
        REQUIRE(messages.size() == 0);
        REQUIRE(test.gestureDetector_->isInParameterLockMode()); // Still in parameter lock
    }
    
    SECTION("Button releases don't generate actions") {
        auto messages = test.processEvent(test.buttonRelease(5));
        REQUIRE(messages.size() == 0);
        
        auto shiftMessages = test.processEvent(test.shiftButtonRelease(5));
        REQUIRE(shiftMessages.size() == 0);
    }
}

TEST_CASE("ShiftBasedGestureDetector - Button state tracking", "[shift-gesture]") {
    ShiftBasedGestureDetectorTest test;
    
    SECTION("Button state updates correctly") {
        bool buttonStates[32];
        
        // Initially all buttons released
        uint8_t numButtons = test.gestureDetector_->getCurrentButtonStates(buttonStates, 32);
        REQUIRE(numButtons == 32);
        REQUIRE_FALSE(buttonStates[5]);
        
        // Press button 5
        test.processEvent(test.buttonPress(5));
        test.gestureDetector_->getCurrentButtonStates(buttonStates, 32);
        REQUIRE(buttonStates[5]);
        
        // Release button 5
        test.processEvent(test.buttonRelease(5));
        test.gestureDetector_->getCurrentButtonStates(buttonStates, 32);
        REQUIRE_FALSE(buttonStates[5]);
    }
    
    SECTION("SHIFT button state tracking") {
        bool buttonStates[32];
        
        // Press SHIFT+button 5
        test.processEvent(test.shiftButtonPress(5));
        test.gestureDetector_->getCurrentButtonStates(buttonStates, 32);
        REQUIRE(buttonStates[5]);
        
        // Release SHIFT+button 5
        test.processEvent(test.shiftButtonRelease(5));
        test.gestureDetector_->getCurrentButtonStates(buttonStates, 32);
        REQUIRE_FALSE(buttonStates[5]);
    }
}

TEST_CASE("ShiftBasedGestureDetector - Configuration", "[shift-gesture]") {
    ShiftBasedGestureDetectorTest test;
    
    SECTION("Configuration update") {
        InputSystemConfiguration newConfig = InputSystemConfiguration::forNeoTrellis();
        test.gestureDetector_->setConfiguration(newConfig);
        
        // Configuration should be accepted without errors
        // (No direct way to verify configuration was stored, but no crash is good)
        REQUIRE(true);
    }
}

TEST_CASE("ShiftBasedGestureDetector - Timing updates", "[shift-gesture]") {
    ShiftBasedGestureDetectorTest test;
    
    SECTION("Timing updates don't generate messages") {
        std::vector<ControlMessage::Message> messages;
        uint8_t count = test.gestureDetector_->updateTiming(1000, messages);
        
        REQUIRE(count == 0);
        REQUIRE(messages.size() == 0);
    }
}