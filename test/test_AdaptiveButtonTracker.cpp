#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/generators/catch_generators.hpp>
#include "AdaptiveButtonTracker.h"

using namespace Catch::Matchers;

TEST_CASE("AdaptiveButtonTracker - Basic functionality", "[adaptive_button_tracker]") {
    AdaptiveButtonTracker tracker;
    
    SECTION("Initial state should be clear") {
        for (uint8_t i = 0; i < AdaptiveButtonTracker::MAX_BUTTONS; ++i) {
            REQUIRE_FALSE(tracker.isPressed(i));
            REQUIRE_FALSE(tracker.wasPressed(i));
            REQUIRE_FALSE(tracker.wasReleased(i));
            REQUIRE_FALSE(tracker.isHeld(i));
        }
        REQUIRE(tracker.getHeldButton() == 0xFF);
    }
    
    SECTION("Button press detection") {
        uint32_t currentTime = 1000;
        uint32_t buttonMask = 0x00000001; // Button 0 pressed
        
        tracker.update(buttonMask, currentTime);
        
        REQUIRE(tracker.isPressed(0));
        REQUIRE(tracker.wasPressed(0));
        REQUIRE_FALSE(tracker.wasReleased(0));
        REQUIRE_FALSE(tracker.isHeld(0)); // Not held yet (< 500ms)
        
        // Other buttons should be unaffected
        for (uint8_t i = 1; i < AdaptiveButtonTracker::MAX_BUTTONS; ++i) {
            REQUIRE_FALSE(tracker.isPressed(i));
            REQUIRE_FALSE(tracker.wasPressed(i));
        }
    }
    
    SECTION("Button release detection") {
        uint32_t currentTime = 1000;
        uint32_t buttonMask = 0x00000001; // Button 0 pressed
        
        // Press button
        tracker.update(buttonMask, currentTime);
        REQUIRE(tracker.wasPressed(0));
        
        // Clear wasPressed flag
        currentTime += 50;
        tracker.update(buttonMask, currentTime);
        REQUIRE_FALSE(tracker.wasPressed(0));
        REQUIRE(tracker.isPressed(0));
        
        // Release button
        currentTime += 50;
        buttonMask = 0x00000000; // No buttons pressed
        tracker.update(buttonMask, currentTime);
        
        REQUIRE_FALSE(tracker.isPressed(0));
        REQUIRE(tracker.wasReleased(0));
        REQUIRE_FALSE(tracker.isHeld(0));
    }
}

TEST_CASE("AdaptiveButtonTracker - Hold detection timing", "[adaptive_button_tracker][timing]") {
    AdaptiveButtonTracker tracker;
    
    SECTION("Hold threshold validation - default 500ms") {
        const uint32_t HOLD_THRESHOLD = 500;
        uint32_t startTime = 1000;
        uint32_t buttonMask = 0x00000001; // Button 0 pressed
        
        // Press button at startTime
        tracker.update(buttonMask, startTime);
        REQUIRE(tracker.isPressed(0));
        REQUIRE_FALSE(tracker.isHeld(0));
        
        // Check at threshold - 1ms (should NOT be held)
        uint32_t almostHeld = startTime + HOLD_THRESHOLD - 1;
        tracker.update(buttonMask, almostHeld);
        REQUIRE(tracker.isPressed(0));
        REQUIRE_FALSE(tracker.isHeld(0));
        REQUIRE(tracker.getHoldDuration(0, almostHeld) == HOLD_THRESHOLD - 1);
        
        // Check at exact threshold (should be held)
        uint32_t exactThreshold = startTime + HOLD_THRESHOLD;
        tracker.update(buttonMask, exactThreshold);
        REQUIRE(tracker.isPressed(0));
        REQUIRE(tracker.isHeld(0));
        REQUIRE(tracker.getHeldButton() == 0);
        REQUIRE(tracker.getHoldDuration(0, exactThreshold) == HOLD_THRESHOLD);
        
        // Check beyond threshold (should still be held)
        uint32_t beyondThreshold = startTime + HOLD_THRESHOLD + 100;
        tracker.update(buttonMask, beyondThreshold);
        REQUIRE(tracker.isPressed(0));
        REQUIRE(tracker.isHeld(0));
        REQUIRE(tracker.getHoldDuration(0, beyondThreshold) == HOLD_THRESHOLD + 100);
    }
    
    SECTION("Multiple buttons - only first held button reported") {
        uint32_t currentTime = 1000;
        
        // Press button 0
        tracker.update(0x00000001, currentTime);
        
        // Press button 1 after 100ms
        currentTime += 100;
        tracker.update(0x00000003, currentTime); // Both buttons pressed
        
        // Wait for button 0 to be held (400ms more)
        currentTime += 400;
        tracker.update(0x00000003, currentTime);
        
        REQUIRE(tracker.isHeld(0));
        REQUIRE_FALSE(tracker.isHeld(1)); // Not held long enough
        REQUIRE(tracker.getHeldButton() == 0);
        
        // Wait for button 1 to also be held
        currentTime += 100;
        tracker.update(0x00000003, currentTime);
        
        REQUIRE(tracker.isHeld(0));
        REQUIRE(tracker.isHeld(1));
        REQUIRE(tracker.getHeldButton() == 0); // Still returns first held
    }
    
    SECTION("Hold processed flag prevents duplicate events") {
        uint32_t currentTime = 1000;
        uint32_t buttonMask = 0x00000001; // Button 0 pressed
        
        tracker.update(buttonMask, currentTime);
        
        // Wait for hold
        currentTime += 500;
        tracker.update(buttonMask, currentTime);
        REQUIRE(tracker.isHeld(0));
        
        // Mark as processed
        tracker.markHoldProcessed(0);
        const auto& state = tracker.getButtonState(0);
        REQUIRE(state.holdProcessed);
        
        // Should still be held but marked as processed
        REQUIRE(tracker.isHeld(0));
    }
}

TEST_CASE("AdaptiveButtonTracker - Edge cases and validation", "[adaptive_button_tracker][validation]") {
    AdaptiveButtonTracker tracker;
    
    SECTION("Invalid button indices") {
        REQUIRE_FALSE(tracker.isValidButton(AdaptiveButtonTracker::MAX_BUTTONS));
        REQUIRE_FALSE(tracker.isValidButton(255));
        REQUIRE(tracker.isValidButton(0));
        REQUIRE(tracker.isValidButton(AdaptiveButtonTracker::MAX_BUTTONS - 1));
        
        // Operations on invalid buttons should be safe
        REQUIRE_FALSE(tracker.isPressed(255));
        REQUIRE_FALSE(tracker.wasPressed(255));
        REQUIRE_FALSE(tracker.wasReleased(255));
        REQUIRE_FALSE(tracker.isHeld(255));
        REQUIRE(tracker.getHoldDuration(255, 1000) == 0);
    }
    
    SECTION("Time wraparound handling") {
        uint32_t nearWrap = 0xFFFFFFF0; // Close to uint32_t max
        uint32_t afterWrap = 0x00000010; // After wraparound
        
        tracker.update(0x00000001, nearWrap);
        REQUIRE(tracker.isPressed(0));
        
        // This tests that the system handles time wraparound gracefully
        tracker.update(0x00000001, afterWrap);
        // System should continue to work (though hold timing may be affected)
        REQUIRE(tracker.isPressed(0));
    }
    
    SECTION("Force button state for testing") {
        uint32_t currentTime = 1000;
        
        // Force button 5 to be pressed
        tracker.forceButtonState(5, true, currentTime);
        REQUIRE(tracker.isPressed(5));
        
        // Force release
        currentTime += 100;
        tracker.forceButtonState(5, false, currentTime);
        REQUIRE_FALSE(tracker.isPressed(5));
    }
    
    SECTION("Button state structure validation") {
        const auto& state = tracker.getButtonState(0);
        REQUIRE_FALSE(state.pressed);
        REQUIRE_FALSE(state.wasPressed);
        REQUIRE_FALSE(state.wasReleased);
        REQUIRE_FALSE(state.isHeld);
        REQUIRE_FALSE(state.holdProcessed);
        REQUIRE(state.holdDuration == 0);
    }
}

TEST_CASE("AdaptiveButtonTracker - Learning system", "[adaptive_button_tracker][learning]") {
    AdaptiveButtonTracker tracker;
    
    SECTION("Learning profile validation") {
        auto profile = tracker.getProfile();
        REQUIRE(profile.isValid());
        REQUIRE(profile.threshold == 500);
        REQUIRE(profile.minThreshold == 300);
        REQUIRE(profile.maxThreshold == 700);
        REQUIRE_THAT(profile.adaptationRate, WithinAbs(0.1f, 0.001f));
        REQUIRE(profile.learningEnabled);
        
        // Test invalid profile
        AdaptiveButtonTracker::HoldProfile invalidProfile;
        invalidProfile.threshold = 800; // Above max
        REQUIRE_FALSE(invalidProfile.isValid());
        
        invalidProfile.threshold = 200; // Below min
        REQUIRE_FALSE(invalidProfile.isValid());
        
        invalidProfile.threshold = 500;
        invalidProfile.adaptationRate = 1.5f; // Above 1.0
        REQUIRE_FALSE(invalidProfile.isValid());
    }
    
    SECTION("Learning statistics initialization") {
        const auto& stats = tracker.getLearningStats();
        REQUIRE(stats.totalActivations == 0);
        REQUIRE(stats.falseActivations == 0);
        REQUIRE(stats.missedActivations == 0);
        REQUIRE_THAT(stats.successRate, WithinAbs(0.0f, 0.001f));
        REQUIRE(stats.avgHoldDuration == 0);
        REQUIRE(stats.samples == 0);
        REQUIRE_FALSE(stats.hasData);
    }
    
    SECTION("Record learning events") {
        // Record successful activation
        tracker.recordSuccessfulActivation(0, 450);
        auto stats = tracker.getLearningStats();
        REQUIRE(stats.totalActivations == 1);
        REQUIRE(stats.samples == 1);
        
        // Record false activation
        tracker.recordFalseActivation(1, 350);
        stats = tracker.getLearningStats();
        REQUIRE(stats.falseActivations == 1);
        
        // Record missed activation
        tracker.recordMissedActivation(2, 250);
        stats = tracker.getLearningStats();
        REQUIRE(stats.missedActivations == 1);
    }
    
    SECTION("Learning can be disabled") {
        tracker.setLearningEnabled(false);
        REQUIRE_FALSE(tracker.getProfile().learningEnabled);
        
        tracker.setLearningEnabled(true);
        REQUIRE(tracker.getProfile().learningEnabled);
    }
    
    SECTION("Reset learning statistics") {
        tracker.recordSuccessfulActivation(0, 450);
        REQUIRE(tracker.getLearningStats().totalActivations == 1);
        
        tracker.resetLearning();
        const auto& stats = tracker.getLearningStats();
        REQUIRE(stats.totalActivations == 0);
        REQUIRE(stats.samples == 0);
    }
}

TEST_CASE("AdaptiveButtonTracker - Comprehensive workflow", "[adaptive_button_tracker][workflow]") {
    AdaptiveButtonTracker tracker;
    
    SECTION("Complete press-hold-release cycle") {
        uint32_t currentTime = 1000;
        const uint8_t testButton = 5;
        const uint32_t buttonMask = 1 << testButton;
        
        // Initial state
        REQUIRE_FALSE(tracker.isPressed(testButton));
        REQUIRE_FALSE(tracker.isHeld(testButton));
        
        // Step 1: Press button
        tracker.update(buttonMask, currentTime);
        REQUIRE(tracker.isPressed(testButton));
        REQUIRE(tracker.wasPressed(testButton));
        REQUIRE_FALSE(tracker.isHeld(testButton));
        REQUIRE(tracker.getHoldDuration(testButton, currentTime) == 0);
        
        // Step 2: Update without time change (wasPressed should clear)
        tracker.update(buttonMask, currentTime);
        REQUIRE(tracker.isPressed(testButton));
        REQUIRE_FALSE(tracker.wasPressed(testButton));
        REQUIRE_FALSE(tracker.isHeld(testButton));
        
        // Step 3: Wait almost to threshold
        currentTime += 499;
        tracker.update(buttonMask, currentTime);
        REQUIRE(tracker.isPressed(testButton));
        REQUIRE_FALSE(tracker.isHeld(testButton));
        REQUIRE(tracker.getHoldDuration(testButton, currentTime) == 499);
        
        // Step 4: Cross threshold
        currentTime += 1;
        tracker.update(buttonMask, currentTime);
        REQUIRE(tracker.isPressed(testButton));
        REQUIRE(tracker.isHeld(testButton));
        REQUIRE(tracker.getHeldButton() == testButton);
        REQUIRE(tracker.getHoldDuration(testButton, currentTime) == 500);
        
        // Step 5: Continue holding
        currentTime += 100;
        tracker.update(buttonMask, currentTime);
        REQUIRE(tracker.isPressed(testButton));
        REQUIRE(tracker.isHeld(testButton));
        REQUIRE(tracker.getHoldDuration(testButton, currentTime) == 600);
        
        // Step 6: Release button
        currentTime += 50;
        tracker.update(0, currentTime);
        REQUIRE_FALSE(tracker.isPressed(testButton));
        REQUIRE_FALSE(tracker.isHeld(testButton));
        REQUIRE(tracker.wasReleased(testButton));
        REQUIRE(tracker.getHeldButton() == 0xFF);
        
        // Final state should be clean
        const auto& state = tracker.getButtonState(testButton);
        REQUIRE_FALSE(state.pressed);
        REQUIRE(state.holdDuration == 650); // Total time held
    }
}

// Parametric tests for different button indices
TEST_CASE("AdaptiveButtonTracker - All buttons functional", "[adaptive_button_tracker][parametric]") {
    AdaptiveButtonTracker tracker;
    
    auto buttonIndex = GENERATE(values({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31}));
    
    SECTION("Button " + std::to_string(buttonIndex) + " press/hold/release") {
        uint32_t currentTime = 1000;
        uint32_t buttonMask = 1UL << buttonIndex;
        
        // Press
        tracker.update(buttonMask, currentTime);
        REQUIRE(tracker.isPressed(buttonIndex));
        
        // Hold
        currentTime += 500;
        tracker.update(buttonMask, currentTime);
        REQUIRE(tracker.isHeld(buttonIndex));
        
        // Release
        tracker.update(0, currentTime + 100);
        REQUIRE_FALSE(tracker.isPressed(buttonIndex));
        REQUIRE(tracker.wasReleased(buttonIndex));
    }
}

// Mock time class for deterministic testing
class MockClock {
private:
    uint32_t currentTime_;
    
public:
    MockClock(uint32_t startTime = 0) : currentTime_(startTime) {}
    
    uint32_t getCurrentTime() const { return currentTime_; }
    void advance(uint32_t ms) { currentTime_ += ms; }
    void setTime(uint32_t time) { currentTime_ = time; }
};

TEST_CASE("AdaptiveButtonTracker - Timing precision tests", "[adaptive_button_tracker][timing][precision]") {
    AdaptiveButtonTracker tracker;
    MockClock mockClock(1000);
    
    SECTION("Precise threshold boundaries") {
        const uint32_t THRESHOLD = 500;
        uint32_t buttonMask = 0x00000001; // Button 0
        
        // Press at t=1000
        tracker.update(buttonMask, mockClock.getCurrentTime());
        REQUIRE(tracker.isPressed(0));
        REQUIRE_FALSE(tracker.isHeld(0));
        
        // Check at t=1499 (threshold - 1)
        mockClock.advance(THRESHOLD - 1);
        tracker.update(buttonMask, mockClock.getCurrentTime());
        REQUIRE_FALSE(tracker.isHeld(0));
        
        // Check at t=1500 (exact threshold)
        mockClock.advance(1);
        tracker.update(buttonMask, mockClock.getCurrentTime());
        REQUIRE(tracker.isHeld(0));
        
        // Verify duration calculation
        REQUIRE(tracker.getHoldDuration(0, mockClock.getCurrentTime()) == THRESHOLD);
    }
    
    SECTION("Rapid press/release cycles") {
        uint32_t buttonMask = 0x00000001; // Button 0
        
        for (int cycle = 0; cycle < 5; ++cycle) {
            // Press
            tracker.update(buttonMask, mockClock.getCurrentTime());
            REQUIRE(tracker.isPressed(0));
            REQUIRE(tracker.wasPressed(0));
            
            // Short hold (not long enough)
            mockClock.advance(100);
            tracker.update(buttonMask, mockClock.getCurrentTime());
            REQUIRE(tracker.isPressed(0));
            REQUIRE_FALSE(tracker.wasPressed(0));
            REQUIRE_FALSE(tracker.isHeld(0));
            
            // Release
            mockClock.advance(50);
            tracker.update(0, mockClock.getCurrentTime());
            REQUIRE_FALSE(tracker.isPressed(0));
            REQUIRE(tracker.wasReleased(0));
            
            // Gap between cycles
            mockClock.advance(50);
            tracker.update(0, mockClock.getCurrentTime());
            REQUIRE_FALSE(tracker.wasReleased(0));
        }
    }
}