#ifndef JSON_STATE_H
#define JSON_STATE_H

#include "../external/nlohmann_json.hpp"
#include <stdint.h>
#include <string>

/**
 * JSON serialization for sequencer state using nlohmann/json.
 * Provides type-safe, validated serialization based on JSON schema.
 */

// Forward declarations
class StepSequencer;
class AdaptiveButtonTracker;

namespace JsonState {
    
    /**
     * Step state representation
     */
    struct StepData {
        bool active = false;
        bool hasLock = false;
        uint8_t lockIndex = 255;
        
        // JSON serialization
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(StepData, active, hasLock, lockIndex)
    };
    
    /**
     * Parameter lock representation
     */
    struct ParameterLock {
        bool inUse = false;
        uint8_t stepIndex = 255;
        uint8_t trackIndex = 255;
        uint16_t activeLocks = 0;
        int8_t noteOffset = 0;
        uint8_t velocity = 100;
        uint8_t length = 12;
        
        // JSON serialization
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(ParameterLock, inUse, stepIndex, trackIndex, 
                                      activeLocks, noteOffset, velocity, length)
    };
    
    /**
     * Button state representation
     */
    struct ButtonState {
        bool pressed = false;
        bool wasPressed = false;
        bool wasReleased = false;
        uint32_t pressTime = 0;
        uint32_t releaseTime = 0;
        bool isHeld = false;
        bool holdProcessed = false;
        uint32_t holdDuration = 0;
        
        // JSON serialization
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(ButtonState, pressed, wasPressed, wasReleased,
                                      pressTime, releaseTime, isHeld, holdProcessed, holdDuration)
    };
    
    /**
     * Track settings representation
     */
    struct TrackSettings {
        uint8_t volume = 100;
        bool muted = false;
        uint8_t note = 36;
        uint8_t channel = 9;
        
        // JSON serialization
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(TrackSettings, volume, muted, note, channel)
    };
    
    /**
     * Parameter lock mode state
     */
    struct ParameterLockMode {
        bool active = false;
        uint8_t heldTrack = 255;
        uint8_t heldStep = 255;
        
        // JSON serialization
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(ParameterLockMode, active, heldTrack, heldStep)
    };
    
    /**
     * Core sequencer state
     */
    struct SequencerCore {
        uint16_t bpm = 120;
        uint8_t stepCount = 8;
        uint8_t currentStep = 0;
        bool playing = false;
        uint32_t currentTime = 0;
        uint32_t tickCounter = 0;
        
        // JSON serialization
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(SequencerCore, bpm, stepCount, currentStep,
                                      playing, currentTime, tickCounter)
    };
    
    /**
     * Complete state snapshot
     */
    struct Snapshot {
        std::string version = "1.0.0";
        std::string timestamp;
        
        SequencerCore sequencer;
        StepData pattern[4][8];
        ParameterLock parameterLocks[64];
        ButtonState buttons[32];
        ParameterLockMode parameterLockMode;
        TrackSettings tracks[4];
        
        /**
         * Convert to JSON string
         */
        std::string toJson() const;
        
        /**
         * Load from JSON string
         */
        bool fromJson(const std::string& json);
        
        /**
         * Save to file
         */
        bool saveToFile(const std::string& filename) const;
        
        /**
         * Load from file
         */
        bool loadFromFile(const std::string& filename);
        
        /**
         * Validate against schema
         */
        bool validate() const;
        
        /**
         * Compare with another snapshot
         */
        bool equals(const Snapshot& other) const;
        
        /**
         * Get human-readable diff
         */
        std::string getDiff(const Snapshot& other) const;
        
        /**
         * Get summary string
         */
        std::string getSummary() const;
    };
    
    /**
     * Capture state from StepSequencer
     */
    Snapshot captureState(const StepSequencer& sequencer);
    
    /**
     * Restore state to StepSequencer
     */
    bool restoreState(StepSequencer& sequencer, const Snapshot& snapshot);
    
} // namespace JsonState

// Custom JSON conversions for arrays
namespace nlohmann {
    template <>
    struct adl_serializer<JsonState::Snapshot> {
        static void to_json(json& j, const JsonState::Snapshot& s);
        static void from_json(const json& j, JsonState::Snapshot& s);
    };
}

#endif // JSON_STATE_H