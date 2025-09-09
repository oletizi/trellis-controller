#ifndef MOCK_INPUT_LAYER_H
#define MOCK_INPUT_LAYER_H

#include "IInputLayer.h"
#include "InputEvent.h"
#include "InputSystemConfiguration.h"
#include "IClock.h"
#include "IDebugOutput.h"
#include <queue>
#include <vector>
#include <stdint.h>

/**
 * @file MockInputLayer.h
 * @brief Programmable mock input layer for unit testing
 * 
 * Provides a controllable input layer implementation that can be programmed
 * with predefined sequences of input events for deterministic unit testing.
 * Supports timing control, event validation, and state inspection.
 * 
 * Key Features:
 * - Programmable event sequences with precise timing
 * - Controllable time advancement for deterministic tests
 * - Event validation and verification capabilities
 * - Configurable failure simulation for error path testing
 * - Complete state inspection for test assertions
 * 
 * Usage in Tests:
 * 1. Program expected event sequence with addProgrammedEvent()
 * 2. Initialize mock with test configuration
 * 3. Advance time and poll to trigger events
 * 4. Verify expected events are generated correctly
 */
class MockInputLayer : public IInputLayer {
public:
    /**
     * @brief Programmed input event with timing
     * 
     * Represents an event that will be generated at a specific
     * time during testing. Used to create deterministic test
     * scenarios with precise timing control.
     */
    struct ProgrammedEvent {
        InputEvent event;           ///< The event to generate
        uint32_t triggerTime;      ///< When to generate the event
        bool triggered = false;    ///< Whether event has been generated
        
        ProgrammedEvent(const InputEvent& evt, uint32_t time) 
            : event(evt), triggerTime(time) {}
    };
    
    /**
     * @brief Default constructor
     * 
     * Creates empty mock ready for programming and initialization.
     */
    MockInputLayer();
    
    /**
     * @brief Virtual destructor ensures proper cleanup
     */
    ~MockInputLayer() override;
    
    // IInputLayer interface implementation
    bool initialize(const InputSystemConfiguration& config, 
                   const InputLayerDependencies& deps) override;
    void shutdown() override;
    bool poll() override;
    bool getNextEvent(InputEvent& event) override;
    bool hasEvents() const override;
    bool setConfiguration(const InputSystemConfiguration& config) override;
    InputSystemConfiguration getConfiguration() const override;
    uint8_t getCurrentButtonStates(bool* buttonStates, uint8_t maxButtons) const override;
    InputState getCurrentInputState() const override;
    InputLayerStatus getStatus() const override;
    uint8_t flush() override;
    uint8_t clearEvents() override;
    
    // Mock-specific functionality for testing
    
    /**
     * @brief Program an input event to be generated at specific time
     * 
     * @param event The input event to generate
     * @param triggerTime When to generate the event (relative to test start)
     */
    void addProgrammedEvent(const InputEvent& event, uint32_t triggerTime);
    
    /**
     * @brief Program a button press event
     * 
     * @param buttonId Button index (0-31)
     * @param pressTime When to generate the press
     */
    void addButtonPress(uint8_t buttonId, uint32_t pressTime);
    
    /**
     * @brief Program a button release event
     * 
     * @param buttonId Button index (0-31)
     * @param releaseTime When to generate the release
     * @param pressDuration How long the button was held
     */
    void addButtonRelease(uint8_t buttonId, uint32_t releaseTime, uint32_t pressDuration = 100);
    
    /**
     * @brief Program a complete button tap (press + release)
     * 
     * @param buttonId Button index (0-31)
     * @param tapTime When to start the tap
     * @param tapDuration Duration of the tap (default 50ms)
     */
    void addButtonTap(uint8_t buttonId, uint32_t tapTime, uint32_t tapDuration = 50);
    
    /**
     * @brief Program a button hold (press + delayed release)
     * 
     * @param buttonId Button index (0-31)
     * @param pressTime When to start the hold
     * @param holdDuration How long to hold (default 600ms)
     */
    void addButtonHold(uint8_t buttonId, uint32_t pressTime, uint32_t holdDuration = 600);
    
    /**
     * @brief Immediately inject an event into the event queue
     * 
     * Unlike addProgrammedEvent, this bypasses timing and immediately
     * makes the event available for getNextEvent(). Useful for
     * integration tests that need precise event control.
     * 
     * @param event The event to inject immediately
     */
    void injectEvent(const InputEvent& event);
    
    /**
     * @brief Clear all programmed events
     * 
     * Removes all pending programmed events and resets state.
     * Useful for setting up new test scenarios.
     */
    void clearProgrammedEvents();
    
    /**
     * @brief Simulate hardware failure
     * 
     * @param shouldFail If true, poll() will return false
     */
    void setHardwareFailure(bool shouldFail);
    
    /**
     * @brief Get number of remaining programmed events
     * 
     * @return Count of events not yet triggered
     */
    uint32_t getRemainingProgrammedEvents() const;
    
    /**
     * @brief Check if all programmed events have been triggered
     * 
     * @return true if no more events are scheduled
     */
    bool allEventsTriggered() const;
    
    /**
     * @brief Get list of events generated so far
     * 
     * Useful for test verification - check that expected events
     * were actually generated in the correct order.
     * 
     * @return Vector of all events that have been made available
     */
    std::vector<InputEvent> getGeneratedEvents() const;
    
    /**
     * @brief Force immediate button state
     * 
     * Directly sets button state without generating events.
     * Useful for testing getCurrentButtonStates().
     * 
     * @param buttonId Button index (0-31)
     * @param pressed New button state
     */
    void setButtonState(uint8_t buttonId, bool pressed);
    
    /**
     * @brief Set all button states at once
     * 
     * @param buttonStates Array of 32 button states
     */
    void setAllButtonStates(const bool* buttonStates);

private:
    // Configuration and dependencies
    InputSystemConfiguration config_;
    IClock* clock_ = nullptr;
    IDebugOutput* debug_ = nullptr;
    
    // State management
    bool initialized_ = false;
    bool hardwareFailure_ = false;
    bool buttonStates_[32]; // 4x8 grid flattened
    
    // Event management
    std::vector<ProgrammedEvent> programmedEvents_;
    std::queue<InputEvent> eventQueue_;
    std::vector<InputEvent> generatedEvents_;
    
    // Statistics
    mutable InputLayerStatus status_;
    
    /**
     * @brief Process programmed events for current time
     * 
     * Checks all programmed events and generates those
     * whose trigger time has been reached.
     * 
     * @return Number of events generated
     */
    uint32_t processProgrammedEvents();
    
    /**
     * @brief Update button state based on event
     * 
     * @param event Input event that may change button state
     */
    void updateButtonStateFromEvent(const InputEvent& event);
    
    /**
     * @brief Validate configuration for mock layer
     * 
     * @param config Configuration to validate
     * @return true if configuration is acceptable
     */
    bool validateConfiguration(const InputSystemConfiguration& config) const;
    
    /**
     * @brief Update performance statistics
     */
    void updateStatistics() const;
};

#endif // MOCK_INPUT_LAYER_H