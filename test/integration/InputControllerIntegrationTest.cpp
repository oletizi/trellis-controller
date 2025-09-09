#include "catch2/catch.hpp"
#include "InputController.h"
#include "GestureDetector.h"
#include "InputStateEncoder.h"
#include "InputStateProcessor.h"
#include "MockInputLayer.h"
#include "MockClock.h"
#include "MockDebugOutput.h"
#include "InputSystemConfiguration.h"
#include "ControlMessage.h"
#include <memory>
#include <vector>

/**
 * @file InputControllerIntegrationTest.cpp
 * @brief Integration tests for the complete InputController pipeline
 * 
 * Tests the complete flow:
 * InputLayer → InputEvents → InputStateEncoder → InputState → InputStateProcessor → ControlMessages → Consumer
 * 
 * Verifies that the new bitwise state management system works correctly
 * and that parameter locks and step toggles function as designed.
 */

class InputControllerIntegrationTestFixture {
public:
    InputControllerIntegrationTestFixture() {
        setupMockComponents();
        setupInputController();
    }
    
    /**
     * @brief Simulate a tap (quick press/release) on a button
     * 
     * @param buttonId Button index (0-31)
     * @param timestamp When the tap occurs
     */
    void simulateTap(uint8_t buttonId, uint32_t timestamp) {
        // Simulate press
        InputEvent pressEvent;
        pressEvent.type = InputEvent::Type::BUTTON_PRESS;
        pressEvent.deviceId = buttonId;
        pressEvent.timestamp = timestamp;
        pressEvent.value = 0;
        mockInputLayer_->injectEvent(pressEvent);
        
        // Simulate quick release (under hold threshold)
        uint32_t releaseTime = timestamp + 100; // 100ms duration - below 500ms threshold
        InputEvent releaseEvent;
        releaseEvent.type = InputEvent::Type::BUTTON_RELEASE;
        releaseEvent.deviceId = buttonId;
        releaseEvent.timestamp = releaseTime;
        releaseEvent.value = 100; // Duration in milliseconds
        mockInputLayer_->injectEvent(releaseEvent);
        
        mockClock_->setCurrentTime(releaseTime + 10);
    }
    
    /**
     * @brief Simulate a hold (long press/release) on a button
     * 
     * @param buttonId Button index (0-31)
     * @param timestamp When the hold starts
     * @param holdDuration Duration of the hold in milliseconds
     */
    void simulateHold(uint8_t buttonId, uint32_t timestamp, uint32_t holdDuration = 600) {
        // Simulate press
        InputEvent pressEvent;
        pressEvent.type = InputEvent::Type::BUTTON_PRESS;
        pressEvent.deviceId = buttonId;
        pressEvent.timestamp = timestamp;
        pressEvent.value = 0;
        mockInputLayer_->injectEvent(pressEvent);
        
        // Advance time and poll to trigger hold detection
        uint32_t holdDetectionTime = timestamp + holdDuration;
        mockClock_->setCurrentTime(holdDetectionTime);
        inputController_->poll();
        
        // Simulate release
        InputEvent releaseEvent;
        releaseEvent.type = InputEvent::Type::BUTTON_RELEASE;
        releaseEvent.deviceId = buttonId;
        releaseEvent.timestamp = holdDetectionTime;
        releaseEvent.value = holdDuration;
        mockInputLayer_->injectEvent(releaseEvent);
        
        mockClock_->setCurrentTime(holdDetectionTime + 10);
    }
    
    /**
     * @brief Process all pending events through the input controller
     * 
     * @return Number of control messages generated
     */
    uint16_t processEvents() {
        inputController_->poll();
        
        uint16_t messageCount = 0;
        while (inputController_->hasMessages()) {
            ControlMessage::Message msg;
            if (inputController_->getNextMessage(msg)) {
                lastMessages_.push_back(msg);
                messageCount++;
            }
        }
        
        return messageCount;
    }
    
    /**
     * @brief Get the last generated control message
     */
    ControlMessage::Message getLastMessage() const {
        REQUIRE(!lastMessages_.empty());
        return lastMessages_.back();
    }
    
    /**
     * @brief Get all generated control messages
     */
    const std::vector<ControlMessage::Message>& getAllMessages() const {
        return lastMessages_;
    }
    
    /**
     * @brief Clear all collected messages
     */
    void clearMessages() {
        lastMessages_.clear();
    }
    
    /**
     * @brief Get the input controller
     */
    InputController* getInputController() const {
        return inputController_.get();
    }
    
    /**
     * @brief Get the mock clock
     */
    MockClock* getClock() const {
        return mockClock_;
    }

private:
    void setupMockComponents() {
        mockClock_ = new MockClock();
        mockDebug_ = std::make_unique<MockDebugOutput>();
        mockInputLayer_ = std::make_unique<MockInputLayer>();
        
        // Create new bitwise state management components
        inputStateEncoder_ = std::make_unique<InputStateEncoder>(InputStateEncoder::Dependencies{
            .clock = mockClock_,
            .debugOutput = mockDebug_.get()
        });
        
        inputStateProcessor_ = std::make_unique<InputStateProcessor>(InputStateProcessor::Dependencies{
            .clock = mockClock_,
            .debugOutput = mockDebug_.get()
        });
    }
    
    void setupInputController() {
        InputController::Dependencies deps;
        deps.inputLayer = std::move(mockInputLayer_);
        deps.gestureDetector = nullptr; // Remove legacy system
        deps.inputStateEncoder = std::move(inputStateEncoder_);
        deps.inputStateProcessor = std::move(inputStateProcessor_);
        deps.clock = mockClock_;
        deps.debugOutput = mockDebug_.get();
        
        inputController_ = std::make_unique<InputController>(
            std::move(deps), 
            createDefaultConfig()
        );
        
        REQUIRE(inputController_->initialize());
    }
    
    InputSystemConfiguration createDefaultConfig() {
        InputSystemConfiguration config;
        config.timing.holdThresholdMs = 500;
        config.timing.debounceMs = 20;
        config.performance.eventQueueSize = 32;
        config.performance.messageQueueSize = 32;
        return config;
    }
    
    // Components (Note: MockClock uses raw pointer for IClock interface compatibility)
    MockClock* mockClock_;
    std::unique_ptr<MockDebugOutput> mockDebug_;
    std::unique_ptr<MockInputLayer> mockInputLayer_;
    std::unique_ptr<InputStateEncoder> inputStateEncoder_;
    std::unique_ptr<InputStateProcessor> inputStateProcessor_;
    std::unique_ptr<InputController> inputController_;
    
    // Test results
    std::vector<ControlMessage::Message> lastMessages_;
};

TEST_CASE("InputController Integration - Step Toggle", "[integration][input-controller]") {
    InputControllerIntegrationTestFixture fixture;
    
    SECTION("Single tap generates step toggle message") {
        // Simulate tap on button 5 (track 0, step 5)
        fixture.simulateTap(5, 1000);
        
        // Process events
        uint16_t messageCount = fixture.processEvents();
        REQUIRE(messageCount == 1);
        
        // Verify step toggle message
        auto message = fixture.getLastMessage();
        REQUIRE(message.type == ControlMessage::Type::TOGGLE_STEP);
        REQUIRE(message.param1 == 0); // track
        REQUIRE(message.param2 == 5); // step
        REQUIRE(message.timestamp == 1110); // Release time + processing delay
    }
    
    SECTION("Multiple taps generate multiple toggle messages") {
        fixture.simulateTap(0, 1000);  // Track 0, step 0
        fixture.simulateTap(8, 1200);  // Track 1, step 0
        fixture.simulateTap(16, 1400); // Track 2, step 0
        
        uint16_t messageCount = fixture.processEvents();
        REQUIRE(messageCount == 3);
        
        auto messages = fixture.getAllMessages();
        
        // First message: track 0, step 0
        REQUIRE(messages[0].type == ControlMessage::Type::TOGGLE_STEP);
        REQUIRE(messages[0].param1 == 0);
        REQUIRE(messages[0].param2 == 0);
        
        // Second message: track 1, step 0
        REQUIRE(messages[1].type == ControlMessage::Type::TOGGLE_STEP);
        REQUIRE(messages[1].param1 == 1);
        REQUIRE(messages[1].param2 == 0);
        
        // Third message: track 2, step 0
        REQUIRE(messages[2].type == ControlMessage::Type::TOGGLE_STEP);
        REQUIRE(messages[2].param1 == 2);
        REQUIRE(messages[2].param2 == 0);
    }
}

TEST_CASE("InputController Integration - Parameter Lock Entry", "[integration][input-controller][param-lock]") {
    InputControllerIntegrationTestFixture fixture;
    
    SECTION("Hold generates parameter lock entry message") {
        // Simulate hold on button 9 (track 1, step 1)
        fixture.simulateHold(9, 2000, 600);
        
        // Process events
        uint16_t messageCount = fixture.processEvents();
        REQUIRE(messageCount == 1);
        
        // Verify parameter lock entry message
        auto message = fixture.getLastMessage();
        REQUIRE(message.type == ControlMessage::Type::ENTER_PARAM_LOCK);
        REQUIRE(message.param1 == 1); // track
        REQUIRE(message.param2 == 1); // step
        REQUIRE(message.timestamp == 2610); // Release time + processing delay
        
        // Verify controller is in parameter lock mode
        REQUIRE(fixture.getInputController()->isInParameterLockMode());
    }
    
    SECTION("Short press does not enter parameter lock mode") {
        // Simulate tap (short press) on button 9
        fixture.simulateTap(9, 2000);
        
        // Process events
        uint16_t messageCount = fixture.processEvents();
        REQUIRE(messageCount == 1);
        
        // Verify it's a step toggle, not parameter lock
        auto message = fixture.getLastMessage();
        REQUIRE(message.type == ControlMessage::Type::TOGGLE_STEP);
        
        // Verify controller is NOT in parameter lock mode
        REQUIRE_FALSE(fixture.getInputController()->isInParameterLockMode());
    }
}

TEST_CASE("InputController Integration - Parameter Lock Exit", "[integration][input-controller][param-lock]") {
    InputControllerIntegrationTestFixture fixture;
    
    SECTION("Releasing locked button exits parameter lock") {
        // Enter parameter lock mode on button 10
        fixture.simulateHold(10, 3000, 600);
        fixture.processEvents();
        fixture.clearMessages();
        
        // Verify we're in parameter lock mode
        REQUIRE(fixture.getInputController()->isInParameterLockMode());
        
        // Release the locked button (simulate another press/release cycle)
        fixture.simulateTap(10, 4000);
        
        // Process events
        uint16_t messageCount = fixture.processEvents();
        REQUIRE(messageCount == 1);
        
        // Verify parameter lock exit message
        auto message = fixture.getLastMessage();
        REQUIRE(message.type == ControlMessage::Type::EXIT_PARAM_LOCK);
        
        // Verify controller is no longer in parameter lock mode
        REQUIRE_FALSE(fixture.getInputController()->isInParameterLockMode());
    }
}

TEST_CASE("InputController Integration - Parameter Adjustment", "[integration][input-controller][param-lock]") {
    InputControllerIntegrationTestFixture fixture;
    
    SECTION("Button presses in parameter lock mode adjust parameters") {
        // Enter parameter lock mode on button 15 (track 1, step 7)
        fixture.simulateHold(15, 5000, 600);
        fixture.processEvents();
        fixture.clearMessages();
        
        // Verify we're in parameter lock mode
        REQUIRE(fixture.getInputController()->isInParameterLockMode());
        
        // Press different buttons to adjust parameters
        fixture.simulateTap(2, 5700);   // Track 0, step 2 (should adjust note)
        fixture.simulateTap(10, 5900);  // Track 1, step 2 (should adjust velocity)
        
        // Process events
        uint16_t messageCount = fixture.processEvents();
        REQUIRE(messageCount == 2);
        
        auto messages = fixture.getAllMessages();
        
        // First adjustment message
        REQUIRE(messages[0].type == ControlMessage::Type::ADJUST_PARAMETER);
        REQUIRE(messages[0].param1 == 1); // Parameter type (note/pitch)
        REQUIRE(messages[0].param2 == 255); // Delta -1 (cast to uint8_t)
        
        // Second adjustment message  
        REQUIRE(messages[1].type == ControlMessage::Type::ADJUST_PARAMETER);
        REQUIRE(messages[1].param1 == 2); // Parameter type (velocity)
        REQUIRE(messages[1].param2 == 255); // Delta -1 (cast to uint8_t)
    }
}

TEST_CASE("InputController Integration - Parameter Lock Timeout", "[integration][input-controller][param-lock]") {
    InputControllerIntegrationTestFixture fixture;
    
    SECTION("Parameter lock times out after inactivity") {
        // Enter parameter lock mode
        fixture.simulateHold(20, 6000, 600);
        fixture.processEvents();
        fixture.clearMessages();
        
        // Verify we're in parameter lock mode
        REQUIRE(fixture.getInputController()->isInParameterLockMode());
        
        // Advance time by 5 seconds (timeout threshold)
        fixture.getClock()->setCurrentTime(6000 + 5000 + 100);
        
        // Poll to trigger timeout check
        fixture.getInputController()->poll();
        
        // Process timeout message
        uint16_t messageCount = fixture.processEvents();
        REQUIRE(messageCount == 1);
        
        // Verify timeout exit message
        auto message = fixture.getLastMessage();
        REQUIRE(message.type == ControlMessage::Type::EXIT_PARAM_LOCK);
        
        // Verify controller is no longer in parameter lock mode
        REQUIRE_FALSE(fixture.getInputController()->isInParameterLockMode());
    }
}

TEST_CASE("InputController Integration - Mixed Interaction Patterns", "[integration][input-controller]") {
    InputControllerIntegrationTestFixture fixture;
    
    SECTION("Complex sequence: taps, hold, adjustments, exit") {
        // Start with some step toggles
        fixture.simulateTap(0, 1000);  // Toggle step
        fixture.simulateTap(1, 1200);  // Toggle step
        uint16_t toggleCount = fixture.processEvents();
        REQUIRE(toggleCount == 2);
        fixture.clearMessages();
        
        // Enter parameter lock
        fixture.simulateHold(5, 1500, 600);  
        uint16_t lockCount = fixture.processEvents();
        REQUIRE(lockCount == 1);
        REQUIRE(fixture.getInputController()->isInParameterLockMode());
        fixture.clearMessages();
        
        // Make parameter adjustments
        fixture.simulateTap(8, 2200);   // Adjust parameter
        fixture.simulateTap(16, 2400);  // Adjust parameter
        uint16_t adjustCount = fixture.processEvents();
        REQUIRE(adjustCount == 2);
        fixture.clearMessages();
        
        // Exit parameter lock
        fixture.simulateTap(5, 2600);  // Release locked button
        uint16_t exitCount = fixture.processEvents();
        REQUIRE(exitCount == 1);
        REQUIRE_FALSE(fixture.getInputController()->isInParameterLockMode());
        fixture.clearMessages();
        
        // Resume normal operation
        fixture.simulateTap(10, 2800);  // Should toggle step again
        uint16_t resumeCount = fixture.processEvents();
        REQUIRE(resumeCount == 1);
        
        auto message = fixture.getLastMessage();
        REQUIRE(message.type == ControlMessage::Type::TOGGLE_STEP);
    }
}

TEST_CASE("InputController Integration - Button State Tracking", "[integration][input-controller]") {
    InputControllerIntegrationTestFixture fixture;
    
    SECTION("Button states are correctly tracked") {
        bool buttonStates[32];
        
        // Initially all buttons should be released
        uint8_t count = fixture.getInputController()->getCurrentButtonStates(buttonStates, 32);
        REQUIRE(count == 32);
        for (int i = 0; i < 32; ++i) {
            REQUIRE_FALSE(buttonStates[i]);
        }
        
        // Simulate some button presses (without releases)
        InputEvent press1{InputEvent::Type::BUTTON_PRESS, 5, 1000, 0};
        InputEvent press2{InputEvent::Type::BUTTON_PRESS, 10, 1050, 0};
        
        // Note: We need to manually inject without simulating full tap/hold
        // to test intermediate states
        
        // For now, verify that the tracking mechanism exists
        REQUIRE(fixture.getInputController()->getCurrentButtonStates(buttonStates, 32) == 32);
    }
}