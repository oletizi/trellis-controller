#ifndef INPUT_EVENT_H
#define INPUT_EVENT_H

#include <stdint.h>
#include <vector>

/**
 * @file InputEvent.h
 * @brief Platform-agnostic input event representation for the Input Layer Abstraction
 * 
 * This file defines the core input event structure that serves as the foundation
 * for cross-platform input handling. All platform-specific input implementations
 * must translate their native input events into this standardized format.
 * 
 * Design Principles:
 * - Zero heap allocation - uses fixed-size data structures
 * - Platform agnostic - no dependencies on specific hardware or OS
 * - Extensible - supports buttons, encoders, MIDI, and system events
 * - Minimal overhead - optimized for real-time embedded systems
 */

/**
 * @brief Platform-agnostic input event structure
 * 
 * Represents any input event that can occur in the system. This abstraction
 * allows the same business logic to work across different input devices and
 * platforms (NeoTrellis hardware, simulation, testing mocks, etc.).
 * 
 * Memory Layout:
 * - Total size: 16 bytes (efficient for embedded systems)
 * - No dynamic allocation
 * - Cache-friendly alignment
 */
struct InputEvent {
    /**
     * @brief Type of input event
     * 
     * Categorizes the event source and expected data interpretation.
     * Each type has specific requirements for the value and context fields.
     */
    enum class Type : uint8_t {
        /**
         * @brief Physical button press event
         * - deviceId: Button index/ID
         * - value: 1 for press, 0 for release
         * - context: Unused (0)
         */
        BUTTON_PRESS = 0,
        
        /**
         * @brief Physical button release event  
         * - deviceId: Button index/ID
         * - value: Duration of press in milliseconds
         * - context: Unused (0)
         */
        BUTTON_RELEASE = 1,
        
        /**
         * @brief Rotary encoder movement
         * - deviceId: Encoder index/ID
         * - value: Signed rotation delta (positive = clockwise)
         * - context: Encoder button state (if applicable)
         */
        ENCODER_TURN = 2,
        
        /**
         * @brief MIDI input message received
         * - deviceId: MIDI channel
         * - value: MIDI data byte 1 (note/CC number)
         * - context: MIDI data byte 2 (velocity/value)
         */
        MIDI_INPUT = 3,
        
        /**
         * @brief System-level events (power, USB, etc.)
         * - deviceId: System event type
         * - value: Event-specific data
         * - context: Additional event data
         */
        SYSTEM_EVENT = 4
    };
    
    Type type;              ///< Event type classification
    uint8_t deviceId;       ///< Which button/encoder/device (0-255)
    uint16_t reserved;      ///< Reserved for future use, maintain alignment
    uint32_t timestamp;     ///< When the event occurred (milliseconds)
    int32_t value;          ///< Button state, encoder delta, MIDI data
    uint32_t context;       ///< Additional context data
    
    /**
     * @brief Default constructor - creates invalid event
     * 
     * Creates an uninitialized event. All fields should be explicitly set
     * before use. The timestamp defaults to 0 which may be updated by
     * the input layer during event creation.
     */
    InputEvent() 
        : type(Type::SYSTEM_EVENT)
        , deviceId(0)
        , reserved(0)
        , timestamp(0)
        , value(0)
        , context(0) 
    {}
    
    /**
     * @brief Parameterized constructor
     * 
     * @param eventType Type of input event
     * @param device Device ID (button index, encoder ID, etc.)
     * @param eventTimestamp When the event occurred
     * @param eventValue Event-specific value (state, delta, etc.)
     * @param eventContext Additional event data
     */
    InputEvent(Type eventType, uint8_t device, uint32_t eventTimestamp, 
               int32_t eventValue, uint32_t eventContext = 0)
        : type(eventType)
        , deviceId(device)
        , reserved(0)
        , timestamp(eventTimestamp)
        , value(eventValue)
        , context(eventContext)
    {}
    
    /**
     * @brief Check if event represents a button press
     * 
     * @return true if this is a button press event
     */
    bool isButtonPress() const {
        return type == Type::BUTTON_PRESS && value != 0;
    }
    
    /**
     * @brief Check if event represents a button release
     * 
     * @return true if this is a button release event
     */
    bool isButtonRelease() const {
        return type == Type::BUTTON_RELEASE || (type == Type::BUTTON_PRESS && value == 0);
    }
    
    /**
     * @brief Check if event represents encoder movement
     * 
     * @return true if this is an encoder turn event with non-zero delta
     */
    bool isEncoderMovement() const {
        return type == Type::ENCODER_TURN && value != 0;
    }
    
    /**
     * @brief Check if this is a valid event
     * 
     * Validates that the event has reasonable values. Used for debugging
     * and input validation in the input layer implementations.
     * 
     * @return true if event appears valid
     */
    bool isValid() const {
        // Basic sanity checks
        return deviceId < 64 && timestamp > 0;  // Reasonable device ID and non-zero timestamp
    }
    
    // Factory methods for common event types
    
    /**
     * @brief Create button press event
     * 
     * @param buttonId Button index/ID
     * @param timestamp When the press occurred
     * @return InputEvent configured for button press
     */
    static InputEvent buttonPress(uint8_t buttonId, uint32_t timestamp) {
        return InputEvent(Type::BUTTON_PRESS, buttonId, timestamp, 1, 0);
    }
    
    /**
     * @brief Create button release event
     * 
     * @param buttonId Button index/ID  
     * @param timestamp When the release occurred
     * @param pressDuration How long the button was held (milliseconds)
     * @return InputEvent configured for button release
     */
    static InputEvent buttonRelease(uint8_t buttonId, uint32_t timestamp, uint32_t pressDuration = 0) {
        return InputEvent(Type::BUTTON_RELEASE, buttonId, timestamp, static_cast<int32_t>(pressDuration), 0);
    }
    
    /**
     * @brief Create encoder turn event
     * 
     * @param encoderId Encoder index/ID
     * @param timestamp When the turn occurred
     * @param delta Rotation amount (positive = clockwise)
     * @param buttonPressed Whether encoder button is currently pressed
     * @return InputEvent configured for encoder movement
     */
    static InputEvent encoderTurn(uint8_t encoderId, uint32_t timestamp, int32_t delta, bool buttonPressed = false) {
        return InputEvent(Type::ENCODER_TURN, encoderId, timestamp, delta, buttonPressed ? 1 : 0);
    }
    
    /**
     * @brief Create MIDI input event
     * 
     * @param channel MIDI channel (0-15)
     * @param timestamp When the MIDI message was received
     * @param data1 First MIDI data byte (note number, CC number)
     * @param data2 Second MIDI data byte (velocity, CC value)
     * @return InputEvent configured for MIDI input
     */
    static InputEvent midiInput(uint8_t channel, uint32_t timestamp, uint8_t data1, uint8_t data2) {
        return InputEvent(Type::MIDI_INPUT, channel, timestamp, data1, data2);
    }
};

/**
 * @brief Configuration for event filtering and processing
 * 
 * Allows input layers to configure which events to generate and how
 * to process them. This enables optimization by filtering out unwanted
 * events at the source rather than processing and discarding them later.
 */
struct InputEventFilter {
    bool enableButtons = true;          ///< Generate button events
    bool enableEncoders = true;         ///< Generate encoder events  
    bool enableMidi = true;             ///< Generate MIDI events
    bool enableSystemEvents = false;    ///< Generate system events
    
    uint32_t buttonDebounceMs = 20;     ///< Button debounce time
    uint32_t encoderDebounceMs = 5;     ///< Encoder debounce time
    
    uint8_t maxDeviceId = 31;           ///< Maximum expected device ID (for validation)
    
    /**
     * @brief Check if event type should be processed
     * 
     * @param eventType Type of event to check
     * @return true if this event type is enabled
     */
    bool shouldProcess(InputEvent::Type eventType) const {
        switch (eventType) {
            case InputEvent::Type::BUTTON_PRESS:
            case InputEvent::Type::BUTTON_RELEASE:
                return enableButtons;
            case InputEvent::Type::ENCODER_TURN:
                return enableEncoders;
            case InputEvent::Type::MIDI_INPUT:
                return enableMidi;
            case InputEvent::Type::SYSTEM_EVENT:
                return enableSystemEvents;
            default:
                return false;
        }
    }
};

#endif // INPUT_EVENT_H