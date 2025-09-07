#include "ParameterEngine.h"
#include <algorithm>
#include <cstring>

// Default parameters for invalid requests
const CalculatedParameters ParameterEngine::defaultParameters_;

ParameterEngine::ParameterEngine(ParameterLockPool* lockPool, IClock* clock)
    : lockPool_(lockPool)
    , clock_(clock)
    , cacheLifetimeMs_(CACHE_LIFETIME_MS)
    , stats_()
    , ownsClock_(false) {
    
    // Initialize cache
    for (uint8_t track = 0; track < MAX_TRACKS; ++track) {
        for (uint8_t step = 0; step < MAX_STEPS; ++step) {
            preCalculated_[track][step] = CalculatedParameters();
            cacheTimestamps_[track][step] = 0;
        }
    }
    
    // Create default clock if none provided
    if (!clock_) {
        // In embedded environment, would use hardware clock
        // For now, just mark that we don't have a clock
        ownsClock_ = false;
    }
}

ParameterEngine::~ParameterEngine() {
    // Clean up owned clock if any
    if (ownsClock_ && clock_) {
        // In real implementation, would delete clock
    }
}

void ParameterEngine::prepareStep(uint8_t track, uint8_t step, 
                                 const StepData& stepData, 
                                 const TrackDefaults& defaults) {
    if (!isValidPosition(track, step)) {
        return;
    }
    
    // Check if we need to recalculate
    if (isValid(track, step) && !isCacheExpired(track, step)) {
        ++stats_.cacheHits;
        return;
    }
    
    ++stats_.cacheMisses;
    
    // Set up calculation context
    CalculationContext context;
    context.track = track;
    context.step = step;
    context.stepData = &stepData;
    context.defaults = &defaults;
    context.currentTime = getCurrentTime();
    context.forceRecalculate = false;
    
    // Get parameter lock if exists
    if (stepData.hasLock && lockPool_) {
        uint8_t lockIndex = stepData.getLockIndex();
        if (lockPool_->isValidIndex(lockIndex)) {
            context.lock = &lockPool_->getLock(lockIndex);
        }
    }
    
    // Calculate and cache parameters
    uint32_t startTime = getCurrentTime();
    preCalculated_[track][step] = calculateParameters(context);
    cacheTimestamps_[track][step] = context.currentTime;
    
    // Update statistics
    uint32_t calcTime = measureTime(startTime);
    updateStats(calcTime);
}

void ParameterEngine::prepareNextStep(uint8_t nextStep, 
                                     const PatternData& patternData,
                                     const TrackDefaults trackDefaults[MAX_TRACKS]) {
    if (nextStep >= MAX_STEPS) {
        return;
    }
    
    // Pre-calculate all tracks for the next step
    for (uint8_t track = 0; track < MAX_TRACKS; ++track) {
        prepareStep(track, nextStep, patternData[track][nextStep], trackDefaults[track]);
    }
}

const CalculatedParameters& ParameterEngine::getParameters(uint8_t track, uint8_t step) const {
    if (!isValidPosition(track, step)) {
        return defaultParameters_;
    }
    
    // Fast path - return cached parameters
    // This should take <10 microseconds
    return preCalculated_[track][step];
}

CalculatedParameters ParameterEngine::applyParameterLock(const CalculatedParameters& base,
                                                        const ParameterLockPool::ParameterLock& lock) const {
    CalculatedParameters result = base;
    
    // Apply note offset
    if (lock.hasParameter(ParameterLockPool::ParameterType::NOTE)) {
        result.note = applyNoteOffset(base.note, lock.noteOffset);
    }
    
    // Apply velocity
    if (lock.hasParameter(ParameterLockPool::ParameterType::VELOCITY)) {
        result.velocity = applyVelocity(base.velocity, lock.velocity, true);
    }
    
    // Apply length
    if (lock.hasParameter(ParameterLockPool::ParameterType::LENGTH)) {
        result.length = applyLength(base.length, lock.length, true);
    }
    
    // Add more parameter types as needed
    
    result.valid = true;
    result.calculationTime = getCurrentTime();
    
    return result;
}

uint8_t ParameterEngine::applyNoteOffset(uint8_t baseNote, int8_t offset) const {
    // Apply offset with bounds checking
    int16_t finalNote = static_cast<int16_t>(baseNote) + offset;
    
    // Clamp to MIDI range
    return clamp<int16_t>(finalNote, 0, 127);
}

uint8_t ParameterEngine::applyVelocity(uint8_t baseVelocity, uint8_t lockVelocity, bool hasVelocityLock) const {
    if (hasVelocityLock) {
        return clamp<uint8_t>(lockVelocity, 0, 127);
    }
    return clamp<uint8_t>(baseVelocity, 0, 127);
}

uint8_t ParameterEngine::applyLength(uint8_t baseLength, uint8_t lockLength, bool hasLengthLock) const {
    if (hasLengthLock) {
        return lockLength;  // No clamping needed for length (0-255 range)
    }
    return baseLength;
}

void ParameterEngine::invalidateStep(uint8_t track, uint8_t step) {
    if (isValidPosition(track, step)) {
        preCalculated_[track][step].invalidate();
        cacheTimestamps_[track][step] = 0;
        ++stats_.invalidations;
    }
}

void ParameterEngine::invalidateAll() {
    for (uint8_t track = 0; track < MAX_TRACKS; ++track) {
        for (uint8_t step = 0; step < MAX_STEPS; ++step) {
            preCalculated_[track][step].invalidate();
            cacheTimestamps_[track][step] = 0;
        }
    }
    stats_.invalidations += (MAX_TRACKS * MAX_STEPS);
}

void ParameterEngine::invalidateTrack(uint8_t track) {
    if (track < MAX_TRACKS) {
        for (uint8_t step = 0; step < MAX_STEPS; ++step) {
            preCalculated_[track][step].invalidate();
            cacheTimestamps_[track][step] = 0;
        }
        stats_.invalidations += MAX_STEPS;
    }
}

bool ParameterEngine::isValid(uint8_t track, uint8_t step) const {
    if (!isValidPosition(track, step)) {
        return false;
    }
    
    return preCalculated_[track][step].valid && !isCacheExpired(track, step);
}

void ParameterEngine::resetStats() {
    stats_ = EngineStats();
}

bool ParameterEngine::validateCache() const {
    for (uint8_t track = 0; track < MAX_TRACKS; ++track) {
        for (uint8_t step = 0; step < MAX_STEPS; ++step) {
            const CalculatedParameters& params = preCalculated_[track][step];
            
            if (params.valid) {
                // Validate parameter ranges
                if (params.note > 127 || params.velocity > 127 || params.channel > 15) {
                    return false;
                }
                
                // Check cache timestamp consistency
                if (cacheTimestamps_[track][step] == 0) {
                    return false;
                }
            }
        }
    }
    
    return true;
}

size_t ParameterEngine::getMemoryUsage() const {
    size_t usage = 0;
    
    // Pre-calculated parameters
    usage += sizeof(preCalculated_);  // 4 * 8 * sizeof(CalculatedParameters)
    
    // Cache timestamps
    usage += sizeof(cacheTimestamps_);  // 4 * 8 * sizeof(uint32_t)
    
    // Other members
    usage += sizeof(stats_);
    usage += sizeof(cacheLifetimeMs_);
    
    return usage;
}

CalculatedParameters ParameterEngine::calculateParameters(const CalculationContext& context) {
    CalculatedParameters result;
    
    // Start with track defaults
    if (context.defaults) {
        result.note = context.defaults->note;
        result.velocity = context.defaults->velocity;
        result.length = context.defaults->length;
        result.channel = context.defaults->channel;
    }
    
    // Apply parameter lock if present
    if (context.lock && context.lock->inUse) {
        result = applyParameterLock(result, *context.lock);
    }
    
    // Mark as valid
    result.valid = true;
    result.calculationTime = context.currentTime;
    
    ++stats_.totalCalculations;
    
    return result;
}

void ParameterEngine::updateStats(uint32_t calcTime) {
    // Update maximum calculation time
    if (calcTime > stats_.maxCalcTime) {
        stats_.maxCalcTime = calcTime;
    }
    
    // Update average calculation time
    if (stats_.totalCalculations == 1) {
        stats_.avgCalcTime = calcTime;
    } else {
        // Running average
        uint32_t totalSamples = stats_.cacheHits + stats_.cacheMisses;
        stats_.avgCalcTime = (stats_.avgCalcTime * (totalSamples - 1) + calcTime) / totalSamples;
    }
    
    // Check for real-time violation (>10 microseconds)
    if (calcTime > 10) {
        stats_.realTimeViolation = true;
    }
    
    // Update cache hit rate
    uint32_t total = stats_.cacheHits + stats_.cacheMisses;
    if (total > 0) {
        stats_.cacheHitRate = static_cast<float>(stats_.cacheHits) / total;
    }
}

uint32_t ParameterEngine::getCurrentTime() const {
    if (clock_) {
        return clock_->getCurrentTime();
    }
    
    // No clock available - return 0
    return 0;
}

bool ParameterEngine::isCacheExpired(uint8_t track, uint8_t step) const {
    if (!isValidPosition(track, step) || cacheLifetimeMs_ == 0) {
        return false;  // No expiration if lifetime is 0
    }
    
    uint32_t currentTime = getCurrentTime();
    uint32_t cacheTime = cacheTimestamps_[track][step];
    
    if (cacheTime == 0) {
        return true;  // Never cached
    }
    
    return (currentTime - cacheTime) > cacheLifetimeMs_;
}

uint32_t ParameterEngine::measureTime(uint32_t startTime) const {
    if (clock_) {
        // Convert milliseconds to microseconds
        // In real implementation, would use microsecond timer
        uint32_t endTime = clock_->getCurrentTime();
        return (endTime - startTime) * 1000;  // Rough approximation
    }
    
    // No timing available - return 1 microsecond as estimate
    return 1;
}