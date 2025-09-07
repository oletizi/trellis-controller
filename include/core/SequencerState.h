#ifndef SEQUENCER_STATE_H
#define SEQUENCER_STATE_H

#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

/**
 * Persistent state representation for the step sequencer.
 * Can serialize/deserialize all sequencer state to/from JSON.
 */
class SequencerState {
public:
    // Forward declaration
    class StepSequencer;
    
    /**
     * Snapshot of a single step's state
     */
    struct StepState {
        bool active = false;
        bool hasLock = false;
        uint8_t lockIndex = 0xFF;  // Parameter lock index if any
        
        // Serialize to JSON object
        std::string toJson() const;
        
        // Deserialize from JSON object
        bool fromJson(const std::string& json);
    };
    
    /**
     * Snapshot of parameter lock state
     */
    struct ParameterLockState {
        bool inUse = false;
        uint8_t stepIndex = 0xFF;
        uint8_t trackIndex = 0xFF;
        uint16_t activeLocks = 0;    // Bitmask of active parameters
        int8_t noteOffset = 0;       // -64 to +63 semitones
        uint8_t velocity = 100;      // 0-127 MIDI velocity
        uint8_t length = 12;         // Gate time in ticks
        
        // Serialize to JSON object
        std::string toJson() const;
        
        // Deserialize from JSON object
        bool fromJson(const std::string& json);
    };
    
    /**
     * Snapshot of button tracker state
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
        
        // Serialize to JSON object
        std::string toJson() const;
        
        // Deserialize from JSON object  
        bool fromJson(const std::string& json);
    };
    
    /**
     * Complete sequencer state snapshot
     */
    struct Snapshot {
        // Basic sequencer state
        uint16_t bpm = 120;
        uint8_t stepCount = 8;
        uint8_t currentStep = 0;
        bool playing = false;
        uint32_t currentTime = 0;
        uint32_t tickCounter = 0;
        
        // Pattern state (4 tracks x 8 steps)
        StepState pattern[4][8];
        
        // Parameter locks (64 slots)
        ParameterLockState parameterLocks[64];
        uint8_t usedLockCount = 0;
        
        // Button states (32 buttons)
        ButtonState buttons[32];
        
        // Parameter lock mode state
        bool inParameterLockMode = false;
        uint8_t heldTrack = 0xFF;
        uint8_t heldStep = 0xFF;
        
        // Track settings
        uint8_t trackVolumes[4] = {100, 100, 100, 100};
        bool trackMutes[4] = {false, false, false, false};
        uint8_t trackNotes[4] = {36, 37, 38, 39};
        uint8_t trackChannels[4] = {9, 9, 9, 9};
        
        // Serialize entire state to JSON
        std::string toJson() const;
        
        // Deserialize from JSON
        bool fromJson(const std::string& json);
        
        // Compare two snapshots
        bool equals(const Snapshot& other) const;
        
        // Get human-readable summary
        std::string getSummary() const;
    };
    
public:
    /**
     * Constructor
     */
    SequencerState();
    
    /**
     * Capture current state from sequencer
     * @param sequencer Sequencer instance to snapshot
     * @return State snapshot
     */
    Snapshot capture(const StepSequencer& sequencer) const;
    
    /**
     * Restore state to sequencer
     * @param sequencer Sequencer instance to restore to
     * @param snapshot State to restore
     * @return true if successful
     */
    bool restore(StepSequencer& sequencer, const Snapshot& snapshot) const;
    
    /**
     * Save snapshot to file
     * @param snapshot State to save
     * @param filename File path
     * @return true if successful
     */
    bool saveToFile(const Snapshot& snapshot, const std::string& filename) const;
    
    /**
     * Load snapshot from file
     * @param filename File path
     * @return Loaded snapshot (check with snapshot.toJson() != "" for success)
     */
    Snapshot loadFromFile(const std::string& filename) const;
    
    /**
     * Verify snapshot matches expected JSON
     * @param snapshot Current state
     * @param expectedJson Expected state as JSON
     * @return true if they match
     */
    bool verifyState(const Snapshot& snapshot, const std::string& expectedJson) const;
    
    /**
     * Get diff between two snapshots
     * @param before First snapshot
     * @param after Second snapshot  
     * @return Human-readable diff
     */
    std::string getDiff(const Snapshot& before, const Snapshot& after) const;
    
private:
    // Helper methods for JSON parsing
    std::string escapeJson(const std::string& str) const;
    std::string unescapeJson(const std::string& str) const;
    bool parseJsonBool(const std::string& json, const std::string& key, bool& value) const;
    bool parseJsonInt(const std::string& json, const std::string& key, int& value) const;
    bool parseJsonString(const std::string& json, const std::string& key, std::string& value) const;
};

#endif // SEQUENCER_STATE_H