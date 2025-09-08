#include "InputStateProcessor.h"
#include <iostream>
#include <cassert>

/**
 * Simple test clock
 */
class TestClock : public IClock {
    uint32_t time_ = 0;
public:
    uint32_t getCurrentTime() const override { return time_; }
    void delay(uint32_t ms) override { time_ += ms; }
    void reset() override { time_ = 0; }
    void setTime(uint32_t t) { time_ = t; }
};

/**
 * Test debug output
 */
class TestDebug : public IDebugOutput {
public:
    std::vector<std::string> messages;
    
    void log(const std::string& message) override {
        messages.push_back(message);
        std::cout << "[DEBUG] " << message << std::endl;
    }
    
    void logf(const char* format, ...) override {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        log(buffer);
    }
};

bool testParameterLockExit() {
    std::cout << "\n=== Testing Parameter Lock Exit ===" << std::endl;
    
    TestClock clock;
    TestDebug debug;
    
    InputStateProcessor::Dependencies deps;
    deps.clock = &clock;
    deps.debugOutput = &debug;
    
    InputStateProcessor processor(deps);
    
    // Test scenario: Parameter lock exit when lowercase 'q' is pressed
    // Button 8 = 'q'/'Q' key (track 1, step 0)
    
    // State 1: Not in parameter lock, button not pressed
    InputState state1(0x00000000, false, 0, 0);
    
    // State 2: Button 8 pressed (uppercase 'Q')
    InputState state2(0x00000100, false, 0, 0);  // Bit 8 set
    
    // State 3: Button 8 held long enough (>500ms), then released - enters parameter lock
    InputState state3(0x00000000, true, 8, 30);  // Released, param lock active, lock button = 8, timing = 600ms
    
    // State 4: Button 8 pressed again (lowercase 'q' in param lock mode)
    InputState state4(0x00000100, true, 8, 0);  // Pressed again
    
    // State 5: Button 8 released (should exit parameter lock)
    InputState state5(0x00000000, true, 8, 2);  // Released with short duration
    
    // Test the state transitions
    clock.setTime(100);
    
    // Transition 1->2: Button press (no message expected)
    auto messages = processor.translateState(state2, state1, clock.getCurrentTime());
    assert(messages.empty());
    std::cout << "✅ Button press generates no immediate message" << std::endl;
    
    // Transition 2->3: Long hold release enters parameter lock
    clock.setTime(700);
    messages = processor.translateState(state3, state2, clock.getCurrentTime());
    assert(messages.size() == 1);
    assert(messages[0].type == ControlMessage::Type::ENTER_PARAM_LOCK);
    assert(messages[0].param1 == 1);  // track 1
    assert(messages[0].param2 == 0);  // step 0
    std::cout << "✅ Long hold release enters parameter lock" << std::endl;
    
    // Transition 3->4: Press lock button again in param lock mode
    clock.setTime(1000);
    messages = processor.translateState(state4, state3, clock.getCurrentTime());
    assert(messages.empty());  // No adjustment for lock button
    std::cout << "✅ Lock button press in param lock generates no adjustment" << std::endl;
    
    // Transition 4->5: Release lock button - should exit parameter lock
    clock.setTime(1050);
    messages = processor.translateState(state5, state4, clock.getCurrentTime());
    assert(messages.size() == 1);
    assert(messages[0].type == ControlMessage::Type::EXIT_PARAM_LOCK);
    std::cout << "✅ Lock button release exits parameter lock!" << std::endl;
    
    return true;
}

bool testParameterAdjustment() {
    std::cout << "\n=== Testing Parameter Adjustment ===" << std::endl;
    
    TestClock clock;
    TestDebug debug;
    
    InputStateProcessor::Dependencies deps;
    deps.clock = &clock;
    deps.debugOutput = &debug;
    
    InputStateProcessor processor(deps);
    
    // In parameter lock mode, pressing other buttons adjusts parameters
    InputState paramLockState(0x00000000, true, 8, 0);  // In param lock, lock button = 8
    InputState adjustState(0x00000001, true, 8, 0);     // Button 0 pressed
    
    auto messages = processor.translateState(adjustState, paramLockState, 100);
    assert(messages.size() == 1);
    assert(messages[0].type == ControlMessage::Type::ADJUST_PARAMETER);
    std::cout << "✅ Parameter adjustment works in param lock mode" << std::endl;
    
    return true;
}

bool testStepToggle() {
    std::cout << "\n=== Testing Step Toggle ===" << std::endl;
    
    TestClock clock;
    TestDebug debug;
    
    InputStateProcessor::Dependencies deps;
    deps.clock = &clock;
    deps.debugOutput = &debug;
    
    InputStateProcessor processor(deps);
    
    // Quick tap (short press/release) toggles step
    InputState unpressed(0x00000000, false, 0, 0);
    InputState pressed(0x00000001, false, 0, 0);     // Button 0 pressed
    InputState released(0x00000000, false, 0, 2);     // Released quickly (40ms)
    
    // Press generates no message
    auto messages = processor.translateState(pressed, unpressed, 100);
    assert(messages.empty());
    
    // Quick release generates toggle
    messages = processor.translateState(released, pressed, 140);
    assert(messages.size() == 1);
    assert(messages[0].type == ControlMessage::Type::TOGGLE_STEP);
    assert(messages[0].param1 == 0);  // track 0
    assert(messages[0].param2 == 0);  // step 0
    std::cout << "✅ Quick tap toggles step" << std::endl;
    
    return true;
}

int main() {
    std::cout << "🧪 InputStateProcessor Test Suite" << std::endl;
    std::cout << "Testing bitwise state encoding solution..." << std::endl;
    
    bool success = true;
    
    success &= testParameterLockExit();
    success &= testParameterAdjustment();
    success &= testStepToggle();
    
    if (success) {
        std::cout << "\n🎉 ALL TESTS PASSED!" << std::endl;
        std::cout << "✅ InputStateProcessor correctly handles all state transitions" << std::endl;
        std::cout << "✅ Parameter lock exit works with bitwise state encoding!" << std::endl;
        return 0;
    } else {
        std::cout << "\n❌ TESTS FAILED!" << std::endl;
        return 1;
    }
}