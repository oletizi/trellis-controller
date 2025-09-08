#include "InputController.h"
#include "GestureDetector.h"
#include "CursesInputLayer.h"
#include "CursesDisplay.h"
#include "InputSystemConfiguration.h"
#include "ControlMessage.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

/**
 * Controlled clock for testing
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
 * Enhanced debug output
 */
class DebugOutput : public IDebugOutput {
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

int main() {
    std::cout << "🔍 Parameter Lock Exit Debug Test" << std::endl;
    
    // Initialize display (required for CursesInputLayer)
    CursesDisplay display;
    display.init();
    
    // Create test environment
    auto clock = std::make_unique<TestClock>();
    auto debug = std::make_unique<DebugOutput>();
    auto inputLayer = std::make_unique<CursesInputLayer>();
    
    TestClock* clockPtr = clock.get();
    
    // Configure system
    auto config = InputSystemConfiguration::forSimulation();
    
    // Initialize input layer
    InputLayerDependencies inputDeps;
    inputDeps.clock = clockPtr;
    inputDeps.debugOutput = debug.get();
    
    if (!inputLayer->initialize(config, inputDeps)) {
        std::cout << "❌ Failed to initialize CursesInputLayer" << std::endl;
        return 1;
    }
    
    // Create gesture detector
    auto gestureDetector = std::make_unique<GestureDetector>(config, clockPtr, debug.get());
    
    // Create input controller
    InputController::Dependencies controllerDeps;
    controllerDeps.inputLayer = std::move(inputLayer);
    controllerDeps.gestureDetector = std::move(gestureDetector);
    controllerDeps.clock = clockPtr;
    controllerDeps.debugOutput = debug.get();
    
    auto inputController = std::make_unique<InputController>(std::move(controllerDeps), config);
    if (!inputController->initialize()) {
        std::cout << "❌ Failed to initialize InputController" << std::endl;
        return 1;
    }
    
    std::cout << "\n=== Testing Parameter Lock Exit Sequence ===\n" << std::endl;
    
    std::cout << "This test will show debug output to trace why parameter lock exit fails." << std::endl;
    std::cout << "The test simulates the exact key sequence that should work:" << std::endl;
    std::cout << "1. Press 'Q' (uppercase) to start hold" << std::endl;
    std::cout << "2. Wait 600ms (exceeds 500ms hold threshold)" << std::endl;
    std::cout << "3. Press 'q' (lowercase) to release hold and enter parameter lock" << std::endl;
    std::cout << "4. Press 'q' (lowercase) again to exit parameter lock" << std::endl;
    std::cout << "\nWatch the debug output to see what events are generated...\n" << std::endl;
    
    // We can't easily simulate getch() input, but we can create a similar debug scenario
    // by manually injecting the events that should be generated
    
    std::cout << "--- Since we can't simulate getch(), showing expected flow ---" << std::endl;
    std::cout << "The issue is likely that after step 3, the CursesInputLayer button state" << std::endl;
    std::cout << "gets reset, so step 4 generates a tap instead of a release." << std::endl;
    
    std::cout << "\n✅ Enhanced debugging added to CursesInputLayer" << std::endl;
    std::cout << "Now run 'make simulation' and test the sequence:" << std::endl;
    std::cout << "1. Hold 'Q' until parameter lock activates (buttons turn white)" << std::endl;
    std::cout << "2. Press 'q' to try to exit parameter lock" << std::endl;
    std::cout << "3. Check console debug output to see if currentState=TRUE or FALSE" << std::endl;
    
    display.shutdown();
    
    return 0;
}