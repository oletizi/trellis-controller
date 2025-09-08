#include "InputController.h"
#include "GestureDetector.h"
#include "MockInputLayer.h"
#include "InputSystemConfiguration.h"
#include "ControlMessage.h"
#include <iostream>
#include <memory>
#include <cstdarg>
#include <cstdio>

/**
 * Mock Clock for deterministic testing
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
 * Simple debug output for testing
 */
class TestDebugOutput : public IDebugOutput {
public:
    void log(const std::string& message) override {
        std::cout << "[DEBUG] " << message << std::endl;
    }
    
    void logf(const char* format, ...) override {
        va_list args;
        va_start(args, format);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        log(std::string(buffer));
    }
};

/**
 * Test the complete InputController pipeline
 */
bool testInputControllerPipeline() {
    std::cout << "\n=== Testing InputController Pipeline ===" << std::endl;
    
    // Create test dependencies
    auto clock = std::make_unique<TestClock>();
    auto debug = std::make_unique<TestDebugOutput>();
    auto mockInput = std::make_unique<MockInputLayer>();
    
    // Keep raw pointers for control
    TestClock* clockPtr = clock.get();
    MockInputLayer* mockPtr = mockInput.get();
    
    // Create configuration
    auto config = InputSystemConfiguration::forTesting();
    
    // Set up input layer dependencies
    InputLayerDependencies inputDeps;
    inputDeps.clock = clockPtr;
    inputDeps.debugOutput = debug.get();
    
    // Initialize mock input layer
    if (!mockPtr->initialize(config, inputDeps)) {
        std::cout << "❌ Failed to initialize MockInputLayer" << std::endl;
        return false;
    }
    
    // Create gesture detector
    auto gestureDetector = std::make_unique<GestureDetector>(config, clockPtr, debug.get());
    
    // Create InputController dependencies
    InputController::Dependencies controllerDeps;
    controllerDeps.inputLayer = std::move(mockInput);
    controllerDeps.gestureDetector = std::move(gestureDetector);
    controllerDeps.clock = clockPtr;
    controllerDeps.debugOutput = debug.get();
    
    // Create InputController
    auto controller = std::make_unique<InputController>(std::move(controllerDeps), config);
    
    if (!controller->initialize()) {
        std::cout << "❌ Failed to initialize InputController" << std::endl;
        return false;
    }
    
    std::cout << "✅ InputController pipeline created successfully" << std::endl;
    
    // Test 1: Quick tap should generate TOGGLE_STEP
    std::cout << "\n--- Test 1: Quick Tap (Step Toggle) ---" << std::endl;
    
    clockPtr->setTime(100);
    mockPtr->addButtonTap(5, 100, 50); // Button 5, at time 100, duration 50ms (quick tap)
    
    clockPtr->setTime(100);
    controller->poll();
    
    clockPtr->setTime(160); // After tap completes
    controller->poll();
    
    if (controller->hasMessages()) {
        ControlMessage::Message msg;
        if (controller->getNextMessage(msg)) {
            if (msg.type == ControlMessage::Type::TOGGLE_STEP) {
                std::cout << "✅ Quick tap generated TOGGLE_STEP message" << std::endl;
            } else {
                std::cout << "❌ Expected TOGGLE_STEP, got: " << static_cast<int>(msg.type) << std::endl;
                return false;
            }
        }
    } else {
        std::cout << "❌ No messages generated for quick tap" << std::endl;
        return false;
    }
    
    // Test 2: Long hold should generate ENTER_PARAM_LOCK
    std::cout << "\n--- Test 2: Long Hold (Parameter Lock) ---" << std::endl;
    
    clockPtr->setTime(200);
    mockPtr->addButtonHold(10, 200, 600); // Button 10, at time 200, hold for 600ms
    
    clockPtr->setTime(200);
    controller->poll();
    
    clockPtr->setTime(810); // After hold completes  
    controller->poll();
    
    if (controller->hasMessages()) {
        ControlMessage::Message msg;
        if (controller->getNextMessage(msg)) {
            if (msg.type == ControlMessage::Type::ENTER_PARAM_LOCK) {
                std::cout << "✅ Long hold generated ENTER_PARAM_LOCK message" << std::endl;
            } else {
                std::cout << "❌ Expected ENTER_PARAM_LOCK, got: " << static_cast<int>(msg.type) << std::endl;
                return false;
            }
        }
    } else {
        std::cout << "❌ No messages generated for long hold" << std::endl;
        return false;
    }
    
    std::cout << "✅ All InputController pipeline tests passed!" << std::endl;
    return true;
}

int main() {
    std::cout << "🧪 Input Controller Integration Test" << std::endl;
    std::cout << "Testing complete Input Layer Abstraction pipeline..." << std::endl;
    
    bool success = testInputControllerPipeline();
    
    if (success) {
        std::cout << "\n🎉 ALL TESTS PASSED!" << std::endl;
        std::cout << "✅ Input Layer Abstraction refactoring is working correctly" << std::endl;
        std::cout << "✅ Parameter locks and step toggling are functional" << std::endl;
        return 0;
    } else {
        std::cout << "\n❌ TESTS FAILED!" << std::endl;
        std::cout << "The Input Layer Abstraction refactoring needs fixes" << std::endl;
        return 1;
    }
}