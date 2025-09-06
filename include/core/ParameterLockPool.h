#ifndef PARAMETER_LOCK_POOL_H
#define PARAMETER_LOCK_POOL_H

#include <stdint.h>

/**
 * Memory pool for parameter locks with O(1) allocation/deallocation.
 * Uses fixed-size allocation to avoid dynamic memory allocation in real-time paths.
 * Thread-safe for single producer/consumer scenarios typical in embedded systems.
 */
class ParameterLockPool {
public:
    static constexpr size_t MAX_LOCKS = 64;
    static constexpr uint8_t INVALID_INDEX = 0xFF;
    
    /**
     * Parameter type bitmask - allows multiple parameters per lock
     */
    enum ParameterType : uint16_t {
        NONE = 0x0000,
        NOTE = 0x0001,
        VELOCITY = 0x0002,
        LENGTH = 0x0004,
        PROBABILITY = 0x0008,
        MICRO_TIMING = 0x0010,
        CC_VALUE = 0x0020,
        RATCHET = 0x0040,
        SLIDE = 0x0080,
        // 8 more available in 16-bit field
    };
    
    /**
     * Individual parameter lock storage.
     * Total size: 8 bytes (aligned for efficient access)
     */
    struct ParameterLock {
        uint16_t activeLocks;      // Bitmask of active parameters
        int8_t noteOffset;         // -64 to +63 semitones
        uint8_t velocity;          // 0-127 MIDI velocity
        uint8_t length;            // Gate time in ticks (0-255)
        uint8_t stepIndex;         // Back-reference for validation
        uint8_t trackIndex;        // Track ownership
        bool inUse;                // Pool management flag
        
        // Default constructor
        ParameterLock() : activeLocks(NONE), noteOffset(0), velocity(100), 
                         length(12), stepIndex(0xFF), trackIndex(0xFF), inUse(false) {}
        
        // Check if a specific parameter is active
        bool hasParameter(ParameterType param) const {
            return (activeLocks & param) != 0;
        }
        
        // Set parameter active
        void setParameter(ParameterType param, bool active = true) {
            if (active) {
                activeLocks |= param;
            } else {
                activeLocks &= ~param;
            }
        }
        
        // Validate lock data integrity
        bool isValid() const {
            return inUse && stepIndex < 8 && trackIndex < 4 && 
                   noteOffset >= -64 && noteOffset <= 63 &&
                   velocity <= 127;
        }
    };
    
    /**
     * Pool statistics for monitoring and debugging
     */
    struct PoolStats {
        size_t totalSlots;
        size_t usedSlots;
        size_t freeSlots;
        float utilization;
        uint32_t totalAllocations;
        uint32_t totalDeallocations;
        uint32_t failedAllocations;
        bool integrityValid;
    };
    
public:
    /**
     * Constructor - initializes empty pool
     */
    ParameterLockPool();
    
    /**
     * Destructor - validates no locks are leaked
     */
    ~ParameterLockPool();
    
    /**
     * Allocate a parameter lock for given track/step.
     * @param track Track index (0-3)
     * @param step Step index (0-7)
     * @return Lock index or INVALID_INDEX if allocation failed
     */
    uint8_t allocate(uint8_t track, uint8_t step);
    
    /**
     * Deallocate a parameter lock by index.
     * @param index Lock index to deallocate
     */
    void deallocate(uint8_t index);
    
    /**
     * Get mutable reference to parameter lock.
     * @param index Lock index
     * @return Reference to parameter lock (undefined behavior if invalid index)
     */
    ParameterLock& getLock(uint8_t index);
    
    /**
     * Get const reference to parameter lock.
     * @param index Lock index
     * @return Const reference to parameter lock (undefined behavior if invalid index)
     */
    const ParameterLock& getLock(uint8_t index) const;
    
    /**
     * Check if index is valid and in use.
     * @param index Lock index to validate
     * @return true if index is valid and lock is in use
     */
    bool isValidIndex(uint8_t index) const;
    
    /**
     * Get number of locks currently in use.
     * @return Number of allocated locks
     */
    uint8_t getUsedCount() const { return MAX_LOCKS - freeCount_; }
    
    /**
     * Get pool utilization as percentage.
     * @return Utilization from 0.0 to 1.0
     */
    float getUtilization() const { 
        return static_cast<float>(getUsedCount()) / MAX_LOCKS; 
    }
    
    /**
     * Check if pool is full.
     * @return true if no more locks can be allocated
     */
    bool isFull() const { return freeCount_ == 0; }
    
    /**
     * Check if pool is empty.
     * @return true if no locks are allocated
     */
    bool isEmpty() const { return freeCount_ == MAX_LOCKS; }
    
    /**
     * Get comprehensive pool statistics.
     * @return PoolStats structure with current statistics
     */
    PoolStats getStats() const;
    
    /**
     * Validate internal data structure integrity.
     * Used for debugging and testing.
     * @return true if pool integrity is valid
     */
    bool validateIntegrity() const;
    
    /**
     * Clear all locks (for emergency cleanup or testing).
     * WARNING: This invalidates all existing lock indices!
     */
    void clearAll();
    
    /**
     * Find lock index for given track/step.
     * @param track Track index
     * @param step Step index  
     * @return Lock index or INVALID_INDEX if not found
     */
    uint8_t findLock(uint8_t track, uint8_t step) const;

private:
    ParameterLock pool_[MAX_LOCKS];        // Fixed-size lock storage
    uint8_t freeList_[MAX_LOCKS];          // Free slot indices
    uint8_t freeCount_;                    // Number of free slots
    uint8_t nextAlloc_;                    // Hint for next allocation (round-robin)
    
    // Statistics
    uint32_t totalAllocations_;
    uint32_t totalDeallocations_;
    uint32_t failedAllocations_;
    
    /**
     * Initialize free list with all indices.
     */
    void initializeFreeList();
    
    /**
     * Add index to free list.
     * @param index Index to add to free list
     */
    void addToFreeList(uint8_t index);
    
    /**
     * Remove index from free list.
     * @return Next free index or INVALID_INDEX if none available
     */
    uint8_t removeFromFreeList();
    
    /**
     * Check if track/step combination is valid.
     * @param track Track index
     * @param step Step index
     * @return true if valid
     */
    bool isValidPosition(uint8_t track, uint8_t step) const {
        return track < 4 && step < 8;
    }
};

#endif // PARAMETER_LOCK_POOL_H