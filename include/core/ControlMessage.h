#ifndef CONTROL_MESSAGE_H
#define CONTROL_MESSAGE_H

#include <stdint.h>
#include <string>

/**
 * Control messages for non-realtime sequencer testing.
 * Allows deterministic testing by sending discrete events.
 */
namespace ControlMessage {
    
    enum class Type : uint8_t {
        KEY_PRESS = 0,      // Button press event
        KEY_RELEASE = 1,    // Button release event  
        CLOCK_TICK = 2,     // Clock advance by N ticks
        TIME_ADVANCE = 3,   // Advance time by N milliseconds
        START = 4,          // Start sequencer
        STOP = 5,           // Stop sequencer
        RESET = 6,          // Reset sequencer
        SAVE_STATE = 7,     // Save current state to file
        LOAD_STATE = 8,     // Load state from file
        VERIFY_STATE = 9,   // Check state matches expected
        SET_TEMPO = 10,     // Set BPM
        QUERY_STATE = 11    // Output current state info
    };
    
    struct Message {
        Type type;
        uint32_t timestamp;     // When this message should be processed
        uint32_t param1;        // Button index, tick count, BPM, etc.
        uint32_t param2;        // Additional parameter if needed
        std::string stringParam; // For file paths, state checks
        
        Message(Type t = Type::CLOCK_TICK, uint32_t ts = 0, uint32_t p1 = 0, uint32_t p2 = 0, const std::string& sp = "")
            : type(t), timestamp(ts), param1(p1), param2(p2), stringParam(sp) {}
        
        // Factory methods for common messages
        static Message keyPress(uint32_t button, uint32_t timestamp = 0) {
            return Message(Type::KEY_PRESS, timestamp, button, 0);
        }
        
        static Message keyRelease(uint32_t button, uint32_t timestamp = 0) {
            return Message(Type::KEY_RELEASE, timestamp, button, 0);
        }
        
        static Message clockTick(uint32_t ticks = 1, uint32_t timestamp = 0) {
            return Message(Type::CLOCK_TICK, timestamp, ticks, 0);
        }
        
        static Message timeAdvance(uint32_t milliseconds, uint32_t timestamp = 0) {
            return Message(Type::TIME_ADVANCE, timestamp, milliseconds, 0);
        }
        
        static Message start(uint32_t timestamp = 0) {
            return Message(Type::START, timestamp, 0, 0);
        }
        
        static Message stop(uint32_t timestamp = 0) {
            return Message(Type::STOP, timestamp, 0, 0);
        }
        
        static Message saveState(const std::string& filename, uint32_t timestamp = 0) {
            return Message(Type::SAVE_STATE, timestamp, 0, 0, filename);
        }
        
        static Message loadState(const std::string& filename, uint32_t timestamp = 0) {
            return Message(Type::LOAD_STATE, timestamp, 0, 0, filename);
        }
        
        static Message verifyState(const std::string& expectedState, uint32_t timestamp = 0) {
            return Message(Type::VERIFY_STATE, timestamp, 0, 0, expectedState);
        }
        
        static Message setTempo(uint32_t bpm, uint32_t timestamp = 0) {
            return Message(Type::SET_TEMPO, timestamp, bpm, 0);
        }
        
        // Convert to string for logging/debugging
        std::string toString() const;
    };
}

#endif // CONTROL_MESSAGE_H