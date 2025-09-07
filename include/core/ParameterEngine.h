#ifndef PARAMETER_ENGINE_H
#define PARAMETER_ENGINE_H

#include <stdint.h>
#include <cstddef>
#include "ParameterLockPool.h"
#include "ParameterLockTypes.h"
#include "IClock.h"

/**
 * Pre-calculated parameter engine for real-time performance.
 * Calculates all step parameters ahead of time to avoid calculations during step triggers.
 * Maintains <10 microsecond step trigger performance target.
 */
class ParameterEngine {
public:
    /**
     * Engine statistics for monitoring
     */
    struct EngineStats {
        uint32_t totalCalculations;      // Total parameter calculations performed
        uint32_t cacheHits;              // Number of cache hits
        uint32_t cacheMisses;            // Number of cache misses
        float cacheHitRate;              // Cache hit rate (0.0-1.0)
        uint32_t maxCalcTime;            // Maximum calculation time (microseconds)
        uint32_t avgCalcTime;            // Average calculation time (microseconds)
        uint32_t invalidations;          // Number of cache invalidations
        bool realTimeViolation;          // Real-time constraint violated
        
        EngineStats()
            : totalCalculations(0), cacheHits(0), cacheMisses(0)
            , cacheHitRate(0.0f), maxCalcTime(0), avgCalcTime(0)
            , invalidations(0), realTimeViolation(false) {}
    };
    
    /**
     * Parameter calculation context
     */
    struct CalculationContext {
        uint8_t track;                   // Track being calculated
        uint8_t step;                    // Step being calculated
        const StepData* stepData;        // Step data with lock index
        const TrackDefaults* defaults;   // Track default parameters
        const ParameterLockPool::ParameterLock* lock; // Parameter lock (may be null)
        uint32_t currentTime;            // Current time for cache validation
        bool forceRecalculate;           // Force recalculation even if cached
        
        CalculationContext()
            : track(0), step(0), stepData(nullptr), defaults(nullptr)
            , lock(nullptr), currentTime(0), forceRecalculate(false) {}
    };

public:
    static constexpr uint8_t MAX_TRACKS = 4;
    static constexpr uint8_t MAX_STEPS = 8;
    static constexpr uint32_t CACHE_LIFETIME_MS = 100;  // Cache validity time
    
    /**
     * Constructor
     * @param lockPool Parameter lock pool reference
     * @param clock Clock interface for timing
     */
    ParameterEngine(ParameterLockPool* lockPool, IClock* clock = nullptr);
    
    /**
     * Destructor
     */
    ~ParameterEngine();
    
    /**
     * Pre-calculate parameters for a specific step
     * @param track Track index (0-3)
     * @param step Step index (0-7)
     * @param stepData Step data containing lock index
     * @param defaults Track default parameters
     */
    void prepareStep(uint8_t track, uint8_t step, 
                    const StepData& stepData, 
                    const TrackDefaults& defaults);
    
    /**
     * Pre-calculate all steps in a column (for next step preparation)
     * @param nextStep Next step index (0-7)
     * @param patternData Pattern data array
     * @param trackDefaults Array of track defaults
     */
    void prepareNextStep(uint8_t nextStep, 
                        const PatternData& patternData,
                        const TrackDefaults trackDefaults[MAX_TRACKS]);
    
    /**
     * Get pre-calculated parameters (fast path, <10 microseconds)
     * @param track Track index (0-3)
     * @param step Step index (0-7)
     * @return Reference to calculated parameters
     */
    const CalculatedParameters& getParameters(uint8_t track, uint8_t step) const;
    
    /**
     * Apply parameter lock to base parameters
     * @param base Base parameters (from track defaults)
     * @param lock Parameter lock to apply
     * @return Calculated parameters with lock applied
     */
    CalculatedParameters applyParameterLock(const CalculatedParameters& base,
                                           const ParameterLockPool::ParameterLock& lock) const;
    
    /**
     * Apply note offset with bounds checking
     * @param baseNote Base MIDI note
     * @param offset Note offset (-64 to +63)
     * @return Final MIDI note (0-127)
     */
    uint8_t applyNoteOffset(uint8_t baseNote, int8_t offset) const;
    
    /**
     * Apply velocity scaling
     * @param baseVelocity Base velocity
     * @param lockVelocity Lock velocity (if active)
     * @param hasVelocityLock Whether velocity lock is active
     * @return Final velocity (0-127)
     */
    uint8_t applyVelocity(uint8_t baseVelocity, uint8_t lockVelocity, bool hasVelocityLock) const;
    
    /**
     * Apply gate length modification
     * @param baseLength Base gate length
     * @param lockLength Lock length (if active)
     * @param hasLengthLock Whether length lock is active
     * @return Final gate length in ticks
     */
    uint8_t applyLength(uint8_t baseLength, uint8_t lockLength, bool hasLengthLock) const;
    
    /**
     * Invalidate cached parameters for a step
     * @param track Track index (0-3)
     * @param step Step index (0-7)
     */
    void invalidateStep(uint8_t track, uint8_t step);
    
    /**
     * Invalidate all cached parameters
     */
    void invalidateAll();
    
    /**
     * Invalidate parameters for a track
     * @param track Track index (0-3)
     */
    void invalidateTrack(uint8_t track);
    
    /**
     * Check if parameters are valid (cached and up-to-date)
     * @param track Track index (0-3)
     * @param step Step index (0-7)
     * @return true if parameters are valid
     */
    bool isValid(uint8_t track, uint8_t step) const;
    
    /**
     * Get engine statistics
     * @return Engine statistics structure
     */
    const EngineStats& getStats() const { return stats_; }
    
    /**
     * Reset statistics
     */
    void resetStats();
    
    /**
     * Set cache lifetime
     * @param lifetimeMs Cache lifetime in milliseconds
     */
    void setCacheLifetime(uint32_t lifetimeMs) { cacheLifetimeMs_ = lifetimeMs; }
    
    /**
     * Get cache lifetime
     * @return Cache lifetime in milliseconds
     */
    uint32_t getCacheLifetime() const { return cacheLifetimeMs_; }
    
    /**
     * Validate all cached parameters (for debugging)
     * @return true if all cached parameters are valid
     */
    bool validateCache() const;
    
    /**
     * Get memory usage
     * @return Memory usage in bytes
     */
    size_t getMemoryUsage() const;

private:
    ParameterLockPool* lockPool_;                           // Reference to lock pool
    IClock* clock_;                                         // Clock for timing (may be null)
    CalculatedParameters preCalculated_[MAX_TRACKS][MAX_STEPS]; // Pre-calculated cache
    uint32_t cacheTimestamps_[MAX_TRACKS][MAX_STEPS];      // Cache timestamps
    uint32_t cacheLifetimeMs_;                             // Cache validity duration
    EngineStats stats_;                                    // Performance statistics
    bool ownsClock_;                                       // Whether we own the clock
    
    // Default parameters for invalid requests
    static const CalculatedParameters defaultParameters_;
    
    /**
     * Calculate parameters for a step
     * @param context Calculation context
     * @return Calculated parameters
     */
    CalculatedParameters calculateParameters(const CalculationContext& context);
    
    /**
     * Update statistics with new timing sample
     * @param calcTime Calculation time in microseconds
     */
    void updateStats(uint32_t calcTime);
    
    /**
     * Get current time in milliseconds
     * @return Current time or 0 if no clock
     */
    uint32_t getCurrentTime() const;
    
    /**
     * Check if cache entry is expired
     * @param track Track index
     * @param step Step index
     * @return true if cache entry is expired
     */
    bool isCacheExpired(uint8_t track, uint8_t step) const;
    
    /**
     * Validate track and step indices
     * @param track Track index
     * @param step Step index
     * @return true if indices are valid
     */
    bool isValidPosition(uint8_t track, uint8_t step) const {
        return track < MAX_TRACKS && step < MAX_STEPS;
    }
    
    /**
     * Clamp value to MIDI range
     * @param value Value to clamp
     * @param min Minimum value
     * @param max Maximum value
     * @return Clamped value
     */
    template<typename T>
    T clamp(T value, T min, T max) const {
        return (value < min) ? min : (value > max) ? max : value;
    }
    
    /**
     * Measure calculation time (for performance monitoring)
     * @param startTime Start time
     * @return Elapsed time in microseconds
     */
    uint32_t measureTime(uint32_t startTime) const;
};

#endif // PARAMETER_ENGINE_H