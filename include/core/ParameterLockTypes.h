#ifndef PARAMETER_LOCK_TYPES_H
#define PARAMETER_LOCK_TYPES_H

#include <stdint.h>
#include <cstddef>
#include "ParameterLockPool.h"

/**
 * Embedded-optimized data structures for parameter locks.
 * Designed for memory efficiency and cache-friendly access patterns.
 */

/**
 * Enhanced step data structure with parameter lock support.
 * Total size: 1 byte per step (bit-packed for efficiency)
 */
struct StepData {
    bool active : 1;           // Step is active/enabled
    bool hasLock : 1;          // Step has parameter locks
    uint8_t lockIndex : 6;     // Index into parameter lock pool (0-63)
    
    // Default constructor
    StepData() : active(false), hasLock(false), lockIndex(0) {}
    
    // Set lock index and mark as having lock
    void setLockIndex(uint8_t index) {
        if (index < 64) {
            lockIndex = index;
            hasLock = true;
        }
    }
    
    // Clear lock association
    void clearLock() {
        hasLock = false;
        lockIndex = 0;
    }
    
    // Get lock index (returns INVALID_INDEX if no lock)
    uint8_t getLockIndex() const {
        return hasLock ? lockIndex : ParameterLockPool::INVALID_INDEX;
    }
    
    // Validate data integrity
    bool isValid() const {
        return !hasLock || lockIndex < 64;
    }
};

/**
 * Pattern storage with parameter lock support.
 * Total memory: 32 bytes for 4 tracks × 8 steps
 */
using PatternData = StepData[4][8];

/**
 * Track default parameters - used when step has no parameter locks
 */
struct TrackDefaults {
    uint8_t note;              // Default MIDI note (0-127)
    uint8_t velocity;          // Default velocity (0-127)
    uint8_t length;            // Default gate length in ticks
    uint8_t channel;           // MIDI channel (0-15)
    bool muted;                // Track mute state
    uint8_t volume;            // Track volume (0-127)
    
    // Default constructor with sensible defaults
    TrackDefaults() 
        : note(60)             // Middle C
        , velocity(100)        // Medium velocity
        , length(12)           // Half step length
        , channel(0)           // Channel 1
        , muted(false)
        , volume(127) {}        // Full volume
    
    // Validate parameter ranges
    bool isValid() const {
        return note <= 127 && 
               velocity <= 127 && 
               channel <= 15 && 
               volume <= 127;
    }
};

/**
 * Pre-calculated parameters for real-time performance.
 * These are computed ahead of time to avoid calculations during step triggers.
 */
struct CalculatedParameters {
    uint8_t note;              // Final note value (0-127)
    uint8_t velocity;          // Final velocity (0-127) 
    uint8_t length;            // Final gate length in ticks
    uint8_t channel;           // MIDI channel (0-15)
    bool valid;                // Parameters are valid and up-to-date
    uint32_t calculationTime;  // When parameters were calculated (for cache invalidation)
    
    // Default constructor
    CalculatedParameters() 
        : note(60), velocity(100), length(12), channel(0), valid(false), calculationTime(0) {}
    
    // Copy constructor from track defaults
    explicit CalculatedParameters(const TrackDefaults& defaults)
        : note(defaults.note)
        , velocity(defaults.velocity)
        , length(defaults.length)
        , channel(defaults.channel)
        , valid(true)
        , calculationTime(0) {}
    
    // Validate parameter ranges
    bool isValid() const {
        return valid && 
               note <= 127 && 
               velocity <= 127 && 
               channel <= 15;
    }
    
    // Mark as invalid (for cache invalidation)
    void invalidate() {
        valid = false;
        calculationTime = 0;
    }
};

/**
 * Parameter constraint validation
 */
struct ParameterConstraints {
    // Note constraints
    struct NoteConstraints {
        uint8_t minNote;       // Minimum allowed note
        uint8_t maxNote;       // Maximum allowed note
        bool enabled;          // Constraints are active
        
        NoteConstraints() : minNote(0), maxNote(127), enabled(false) {}
        
        bool isValid(uint8_t note) const {
            return !enabled || (note >= minNote && note <= maxNote);
        }
    };
    
    // Velocity constraints  
    struct VelocityConstraints {
        uint8_t minVelocity;   // Minimum velocity
        uint8_t maxVelocity;   // Maximum velocity
        bool enabled;          // Constraints are active
        
        VelocityConstraints() : minVelocity(1), maxVelocity(127), enabled(false) {}
        
        bool isValid(uint8_t velocity) const {
            return !enabled || (velocity >= minVelocity && velocity <= maxVelocity);
        }
    };
    
    NoteConstraints note;
    VelocityConstraints velocity;
    // Add more constraint types as needed
    
    // Validate all constraints for a parameter set
    bool validateParameters(const CalculatedParameters& params) const {
        return note.isValid(params.note) && velocity.isValid(params.velocity);
    }
};

/**
 * Memory usage statistics for monitoring
 */
struct MemoryStats {
    size_t patternDataSize;        // Bytes used by pattern data
    size_t trackDefaultsSize;      // Bytes used by track defaults
    size_t calculatedParamsSize;   // Bytes used by calculated parameters
    size_t parameterLocksSize;     // Bytes used by parameter locks
    size_t totalUsage;             // Total memory usage
    float poolUtilization;         // Parameter lock pool utilization
    
    MemoryStats() : patternDataSize(0), trackDefaultsSize(0), calculatedParamsSize(0),
                   parameterLocksSize(0), totalUsage(0), poolUtilization(0.0f) {}
};

/**
 * Performance monitoring for real-time validation
 */
struct PerformanceStats {
    uint32_t parameterCalcTime;    // Microseconds for parameter calculation
    uint32_t stepTriggerTime;      // Microseconds for step trigger
    uint32_t maxCalcTime;          // Maximum calculation time observed
    uint32_t avgCalcTime;          // Average calculation time
    uint32_t samples;              // Number of timing samples
    bool realTimeViolation;        // Real-time constraint violated
    
    PerformanceStats() : parameterCalcTime(0), stepTriggerTime(0), maxCalcTime(0),
                        avgCalcTime(0), samples(0), realTimeViolation(false) {}
    
    // Update statistics with new timing sample
    void updateTiming(uint32_t newTime) {
        if (samples == 0) {
            avgCalcTime = newTime;
        } else {
            // Running average
            avgCalcTime = (avgCalcTime * samples + newTime) / (samples + 1);
        }
        
        if (newTime > maxCalcTime) {
            maxCalcTime = newTime;
        }
        
        ++samples;
        
        // Check for real-time violation (budget: 10 microseconds)
        if (newTime > 10) {
            realTimeViolation = true;
        }
    }
    
    // Reset statistics
    void reset() {
        parameterCalcTime = 0;
        stepTriggerTime = 0;
        maxCalcTime = 0;
        avgCalcTime = 0;
        samples = 0;
        realTimeViolation = false;
    }
};

// Compile-time size validation to ensure memory efficiency
static_assert(sizeof(StepData) == 1, "StepData must be exactly 1 byte");
static_assert(sizeof(TrackDefaults) == 6, "TrackDefaults should be 6 bytes or less");
static_assert(sizeof(CalculatedParameters) <= 12, "CalculatedParameters should be 12 bytes or less");

#endif // PARAMETER_LOCK_TYPES_H