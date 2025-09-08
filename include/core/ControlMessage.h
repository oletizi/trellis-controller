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
        // Legacy low-level events (for backwards compatibility)
        KEY_PRESS = 0,              // Button press event (legacy)
        KEY_RELEASE = 1,            // Button release event (legacy)
        
        // Semantic sequencer commands
        TOGGLE_STEP = 2,            // Toggle step on/off (param1=track, param2=step)
        ENTER_PARAM_LOCK = 3,       // Enter parameter lock mode (param1=track, param2=step)
        EXIT_PARAM_LOCK = 4,        // Exit parameter lock mode
        ADJUST_PARAMETER = 5,       // Adjust parameter (param1=paramType, param2=delta as signed)
        
        // Timing and control
        CLOCK_TICK = 10,            // Clock advance by N ticks
        TIME_ADVANCE = 11,          // Advance time by N milliseconds
        START = 12,                 // Start sequencer
        STOP = 13,                  // Stop sequencer
        RESET = 14,                 // Reset sequencer
        
        // State management
        SAVE_STATE = 20,            // Save current state to file
        LOAD_STATE = 21,            // Load state from file
        VERIFY_STATE = 22,          // Check state matches expected
        QUERY_STATE = 23,           // Output current state info
        
        // Configuration
        SET_TEMPO = 30,             // Set BPM
        
        // System events
        SYSTEM_EVENT = 40           // System events (quit, power, etc.)
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
        
        // Semantic message factory methods
        static Message toggleStep(uint8_t track, uint8_t step, uint32_t timestamp = 0) {
            return Message(Type::TOGGLE_STEP, timestamp, track, step);
        }
        
        static Message enterParamLock(uint8_t track, uint8_t step, uint32_t timestamp = 0) {
            return Message(Type::ENTER_PARAM_LOCK, timestamp, track, step);
        }
        
        static Message exitParamLock(uint32_t timestamp = 0) {
            return Message(Type::EXIT_PARAM_LOCK, timestamp, 0, 0);
        }
        
        static Message adjustParameter(uint8_t paramType, int8_t delta, uint32_t timestamp = 0) {
            return Message(Type::ADJUST_PARAMETER, timestamp, paramType, static_cast<uint32_t>(delta));
        }
        
        // Convert to string for logging/debugging
        std::string toString() const;
    };
}

#endif // CONTROL_MESSAGE_H