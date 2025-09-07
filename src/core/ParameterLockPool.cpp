#include "ParameterLockPool.h"
#include <cstring>  // For memset

ParameterLockPool::ParameterLockPool() 
    : freeCount_(MAX_LOCKS)
    , nextAlloc_(0)
    , totalAllocations_(0)
    , totalDeallocations_(0)
    , failedAllocations_(0) {
    
    // Initialize all locks to default state
    for (size_t i = 0; i < MAX_LOCKS; ++i) {
        pool_[i] = ParameterLock();  // Use default constructor
    }
    
    // Initialize free list with all indices
    initializeFreeList();
}

ParameterLockPool::~ParameterLockPool() {
    // In debug builds, validate that all locks are properly deallocated
    #ifdef DEBUG
    for (size_t i = 0; i < MAX_LOCKS; ++i) {
        if (pool_[i].inUse) {
            // Log warning about leaked lock
            // In embedded system, this might trigger a diagnostic LED or similar
        }
    }
    #endif
}

uint8_t ParameterLockPool::allocate(uint8_t track, uint8_t step) {
    // Validate input parameters
    if (!isValidPosition(track, step)) {
        ++failedAllocations_;
        return INVALID_INDEX;
    }
    
    // Check if pool is full
    if (freeCount_ == 0) {
        ++failedAllocations_;
        return INVALID_INDEX;
    }
    
    // Check if lock already exists for this track/step
    uint8_t existing = findLock(track, step);
    if (existing != INVALID_INDEX) {
        // Return existing lock index instead of creating duplicate
        return existing;
    }
    
    // Get next free index
    uint8_t index = removeFromFreeList();
    if (index == INVALID_INDEX) {
        ++failedAllocations_;
        return INVALID_INDEX;
    }
    
    // Initialize the lock
    ParameterLock& lock = pool_[index];
    lock = ParameterLock();  // Reset to defaults
    lock.stepIndex = step;
    lock.trackIndex = track;
    lock.inUse = true;
    
    ++totalAllocations_;
    return index;
}

void ParameterLockPool::deallocate(uint8_t index) {
    // Validate index
    if (!isValidIndex(index)) {
        return;
    }
    
    // Get lock reference
    ParameterLock& lock = pool_[index];
    
    // Clear lock data
    lock = ParameterLock();  // Reset to defaults
    lock.inUse = false;
    
    // Add back to free list
    addToFreeList(index);
    
    ++totalDeallocations_;
}

ParameterLockPool::ParameterLock& ParameterLockPool::getLock(uint8_t index) {
    // In debug builds, validate index
    #ifdef DEBUG
    if (index >= MAX_LOCKS || !pool_[index].inUse) {
        // This is undefined behavior - should not happen in correct usage
        // In embedded system, might trigger watchdog reset or diagnostic
    }
    #endif
    
    return pool_[index];
}

const ParameterLockPool::ParameterLock& ParameterLockPool::getLock(uint8_t index) const {
    // In debug builds, validate index
    #ifdef DEBUG
    if (index >= MAX_LOCKS || !pool_[index].inUse) {
        // This is undefined behavior - should not happen in correct usage
    }
    #endif
    
    return pool_[index];
}

bool ParameterLockPool::isValidIndex(uint8_t index) const {
    return index < MAX_LOCKS && pool_[index].inUse;
}

ParameterLockPool::PoolStats ParameterLockPool::getStats() const {
    PoolStats stats;
    stats.totalSlots = MAX_LOCKS;
    stats.usedSlots = getUsedCount();
    stats.freeSlots = freeCount_;
    stats.utilization = getUtilization();
    stats.totalAllocations = totalAllocations_;
    stats.totalDeallocations = totalDeallocations_;
    stats.failedAllocations = failedAllocations_;
    stats.integrityValid = validateIntegrity();
    
    return stats;
}

bool ParameterLockPool::validateIntegrity() const {
    // Check free count consistency
    uint8_t actualFree = 0;
    uint8_t actualUsed = 0;
    
    for (size_t i = 0; i < MAX_LOCKS; ++i) {
        if (pool_[i].inUse) {
            ++actualUsed;
            
            // Validate lock data
            if (!pool_[i].isValid()) {
                return false;
            }
        } else {
            ++actualFree;
        }
    }
    
    if (actualFree != freeCount_ || actualUsed != (MAX_LOCKS - freeCount_)) {
        return false;
    }
    
    // Validate free list doesn't contain duplicates
    for (uint8_t i = 0; i < freeCount_; ++i) {
        uint8_t index = freeList_[i];
        
        // Check bounds
        if (index >= MAX_LOCKS) {
            return false;
        }
        
        // Check that slot is actually free
        if (pool_[index].inUse) {
            return false;
        }
        
        // Check for duplicates in remaining list
        for (uint8_t j = i + 1; j < freeCount_; ++j) {
            if (freeList_[j] == index) {
                return false;
            }
        }
    }
    
    return true;
}

void ParameterLockPool::clearAll() {
    // Reset all locks
    for (size_t i = 0; i < MAX_LOCKS; ++i) {
        pool_[i] = ParameterLock();
        pool_[i].inUse = false;
    }
    
    // Reinitialize free list
    freeCount_ = MAX_LOCKS;
    initializeFreeList();
    nextAlloc_ = 0;
    
    // Note: We don't reset statistics as they provide historical data
}

uint8_t ParameterLockPool::findLock(uint8_t track, uint8_t step) const {
    if (!isValidPosition(track, step)) {
        return INVALID_INDEX;
    }
    
    // Linear search through used locks
    // For 64 locks max, this is acceptable performance
    for (uint8_t i = 0; i < MAX_LOCKS; ++i) {
        const ParameterLock& lock = pool_[i];
        if (lock.inUse && lock.trackIndex == track && lock.stepIndex == step) {
            return i;
        }
    }
    
    return INVALID_INDEX;
}

void ParameterLockPool::initializeFreeList() {
    for (uint8_t i = 0; i < MAX_LOCKS; ++i) {
        freeList_[i] = i;
    }
    freeCount_ = MAX_LOCKS;
}

void ParameterLockPool::addToFreeList(uint8_t index) {
    if (freeCount_ >= MAX_LOCKS) {
        // Free list is full - this should not happen
        return;
    }
    
    freeList_[freeCount_] = index;
    ++freeCount_;
}

uint8_t ParameterLockPool::removeFromFreeList() {
    if (freeCount_ == 0) {
        return INVALID_INDEX;
    }
    
    // Use round-robin allocation to distribute wear evenly
    uint8_t allocIndex = nextAlloc_ % freeCount_;
    uint8_t index = freeList_[allocIndex];
    
    // Remove from free list by swapping with last element
    freeList_[allocIndex] = freeList_[freeCount_ - 1];
    --freeCount_;
    
    // Advance allocation hint
    nextAlloc_ = (nextAlloc_ + 1) % MAX_LOCKS;
    
    return index;
}