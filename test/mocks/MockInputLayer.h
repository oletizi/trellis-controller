#ifndef MOCK_INPUT_LAYER_H
#define MOCK_INPUT_LAYER_H

#include "IInputLayer.h"
#include "InputEvent.h"
#include "InputSystemConfiguration.h"
#include <queue>
#include <vector>

/**
 * @file MockInputLayer.h
 * @brief Mock implementation of IInputLayer for deterministic testing
 * 
 * Provides complete control over input events for testing the gesture
 * detection and input processing pipeline. Allows injection of specific
 * event sequences and verification of proper event handling.
 */
class MockInputLayer : public IInputLayer {
public:
    MockInputLayer() = default;
    ~MockInputLayer() override = default;

    // IInputLayer interface
    bool initialize(const InputSystemConfiguration& config, 
                   const InputLayerDependencies& deps) override {
        config_ = config;
        deps_ = deps;
        initialized_ = true;
        status_.initialized = true;
        status_.errorCount = 0;
        return true;
    }

    void shutdown() override {
        initialized_ = false;
        eventQueue_ = std::queue<InputEvent>(); // Clear queue
        status_.initialized = false;
    }

    bool poll() override {
        if (!initialized_) return false;
        
        status_.pollCount++;
        status_.lastPollTime = deps_.clock ? deps_.clock->getCurrentTime() : 0;
        return true;
    }

    bool getNextEvent(InputEvent& event) override {
        if (eventQueue_.empty()) {
            return false;
        }
        
        event = eventQueue_.front();
        eventQueue_.pop();
        status_.eventsProcessed++;
        return true;
    }

    bool hasEvents() const override {
        return !eventQueue_.empty();
    }

    bool setConfiguration(const InputSystemConfiguration& config) override {
        config_ = config;
        return true;
    }

    InputSystemConfiguration getConfiguration() const override {
        return config_;
    }

    uint8_t getCurrentButtonStates(bool* buttonStates, uint8_t maxButtons) const override {
        if (!buttonStates) return 0;
        
        uint8_t count = std::min(static_cast<uint8_t>(32), maxButtons);
        for (uint8_t i = 0; i < count; ++i) {
            buttonStates[i] = currentButtonStates_[i];
        }
        return count;
    }

    InputLayerStatus getStatus() const override {
        return status_;
    }

    uint8_t flush() override {
        uint8_t flushed = static_cast<uint8_t>(eventQueue_.size());
        eventQueue_ = std::queue<InputEvent>(); // Clear queue
        return flushed;
    }

    uint8_t clearEvents() override {
        return flush();
    }

    // Mock-specific methods for test control
    
    /**
     * @brief Inject an input event into the mock layer
     * 
     * @param event Event to inject
     */
    void injectEvent(const InputEvent& event) {
        eventQueue_.push(event);
        
        // Update button state tracking
        if (event.deviceId < 32) {
            if (event.type == InputEvent::Type::BUTTON_PRESS) {
                currentButtonStates_[event.deviceId] = true;
            } else if (event.type == InputEvent::Type::BUTTON_RELEASE) {
                currentButtonStates_[event.deviceId] = false;
            }
        }
    }

    /**
     * @brief Inject a sequence of events
     * 
     * @param events Vector of events to inject in order
     */
    void injectEvents(const std::vector<InputEvent>& events) {
        for (const auto& event : events) {
            injectEvent(event);
        }
    }

    /**
     * @brief Clear all pending events
     */
    void clearAllEvents() {
        eventQueue_ = std::queue<InputEvent>();
        
        // Reset button states
        for (int i = 0; i < 32; ++i) {
            currentButtonStates_[i] = false;
        }
    }

    /**
     * @brief Get number of pending events
     * 
     * @return Number of events in queue
     */
    size_t getPendingEventCount() const {
        return eventQueue_.size();
    }

    /**
     * @brief Set button state directly (for testing state queries)
     * 
     * @param buttonId Button index (0-31)
     * @param pressed Whether button is pressed
     */
    void setButtonState(uint8_t buttonId, bool pressed) {
        if (buttonId < 32) {
            currentButtonStates_[buttonId] = pressed;
        }
    }

    /**
     * @brief Simulate error condition
     * 
     * @param errorCount Number of errors to simulate
     */
    void simulateError(uint16_t errorCount = 1) {
        status_.errorCount += errorCount;
    }

    /**
     * @brief Reset all mock state
     */
    void reset() {
        clearAllEvents();
        status_ = InputLayerStatus{};
        if (initialized_) {
            status_.initialized = true;
        }
    }

private:
    // Configuration and dependencies
    InputSystemConfiguration config_;
    InputLayerDependencies deps_;
    
    // State
    bool initialized_ = false;
    std::queue<InputEvent> eventQueue_;
    bool currentButtonStates_[32] = {false};
    mutable InputLayerStatus status_;
};

#endif // MOCK_INPUT_LAYER_H