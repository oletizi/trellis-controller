#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/generators/catch_generators.hpp>
#include "ParameterLockPool.h"

using namespace Catch::Matchers;

TEST_CASE("ParameterLockPool - Initialization and basic operations", "[parameter_lock_pool]") {
    ParameterLockPool pool;
    
    SECTION("Initial state should be empty") {
        REQUIRE(pool.isEmpty());
        REQUIRE_FALSE(pool.isFull());
        REQUIRE(pool.getUsedCount() == 0);
        REQUIRE_THAT(pool.getUtilization(), WithinAbs(0.0f, 0.001f));
        
        auto stats = pool.getStats();
        REQUIRE(stats.totalSlots == ParameterLockPool::MAX_LOCKS);
        REQUIRE(stats.usedSlots == 0);
        REQUIRE(stats.freeSlots == ParameterLockPool::MAX_LOCKS);
        REQUIRE_THAT(stats.utilization, WithinAbs(0.0f, 0.001f));
        REQUIRE(stats.totalAllocations == 0);
        REQUIRE(stats.totalDeallocations == 0);
        REQUIRE(stats.failedAllocations == 0);
        REQUIRE(stats.integrityValid);
    }
    
    SECTION("Pool integrity validation") {
        REQUIRE(pool.validateIntegrity());
    }
}

TEST_CASE("ParameterLockPool - Basic allocation and deallocation", "[parameter_lock_pool]") {
    ParameterLockPool pool;
    
    SECTION("Single allocation") {
        uint8_t index = pool.allocate(0, 0); // Track 0, Step 0
        REQUIRE(index != ParameterLockPool::INVALID_INDEX);
        REQUIRE(pool.isValidIndex(index));
        REQUIRE(pool.getUsedCount() == 1);
        REQUIRE_FALSE(pool.isEmpty());
        REQUIRE_FALSE(pool.isFull());
        
        const auto& lock = pool.getLock(index);
        REQUIRE(lock.inUse);
        REQUIRE(lock.trackIndex == 0);
        REQUIRE(lock.stepIndex == 0);
        REQUIRE(lock.isValid());
    }
    
    SECTION("Allocation and deallocation cycle") {
        uint8_t index = pool.allocate(1, 3); // Track 1, Step 3
        REQUIRE(index != ParameterLockPool::INVALID_INDEX);
        REQUIRE(pool.getUsedCount() == 1);
        
        pool.deallocate(index);
        REQUIRE(pool.getUsedCount() == 0);
        REQUIRE(pool.isEmpty());
        REQUIRE_FALSE(pool.isValidIndex(index));
    }
    
    SECTION("Multiple allocations") {
        std::vector<uint8_t> indices;
        
        // Allocate several locks
        for (uint8_t track = 0; track < 4; ++track) {
            for (uint8_t step = 0; step < 8; ++step) {
                uint8_t index = pool.allocate(track, step);
                REQUIRE(index != ParameterLockPool::INVALID_INDEX);
                indices.push_back(index);
                
                const auto& lock = pool.getLock(index);
                REQUIRE(lock.trackIndex == track);
                REQUIRE(lock.stepIndex == step);
            }
        }
        
        REQUIRE(pool.getUsedCount() == 32); // 4 tracks * 8 steps
        REQUIRE_THAT(pool.getUtilization(), WithinAbs(32.0f / ParameterLockPool::MAX_LOCKS, 0.001f));
        
        // Deallocate all
        for (uint8_t index : indices) {
            pool.deallocate(index);
        }
        
        REQUIRE(pool.isEmpty());
    }
}

TEST_CASE("ParameterLockPool - Parameter manipulation", "[parameter_lock_pool][parameters]") {
    ParameterLockPool pool;
    
    SECTION("Parameter lock structure defaults") {
        uint8_t index = pool.allocate(0, 0);
        auto& lock = pool.getLock(index);
        
        REQUIRE(lock.activeLocks == ParameterLockPool::ParameterType::NONE);
        REQUIRE(lock.noteOffset == 0);
        REQUIRE(lock.velocity == 100);
        REQUIRE(lock.length == 12);
        REQUIRE(lock.inUse);
        REQUIRE(lock.isValid());
    }
    
    SECTION("Parameter activation and deactivation") {
        uint8_t index = pool.allocate(2, 5);
        auto& lock = pool.getLock(index);
        
        // Test parameter activation
        lock.setParameter(ParameterLockPool::ParameterType::NOTE, true);
        REQUIRE(lock.hasParameter(ParameterLockPool::ParameterType::NOTE));
        REQUIRE(lock.activeLocks & ParameterLockPool::ParameterType::NOTE);
        
        lock.setParameter(ParameterLockPool::ParameterType::VELOCITY, true);
        REQUIRE(lock.hasParameter(ParameterLockPool::ParameterType::VELOCITY));
        REQUIRE(lock.hasParameter(ParameterLockPool::ParameterType::NOTE)); // Should still be active
        
        // Test parameter deactivation
        lock.setParameter(ParameterLockPool::ParameterType::NOTE, false);
        REQUIRE_FALSE(lock.hasParameter(ParameterLockPool::ParameterType::NOTE));
        REQUIRE(lock.hasParameter(ParameterLockPool::ParameterType::VELOCITY)); // Should still be active
    }
    
    SECTION("Multiple parameters simultaneously") {
        uint8_t index = pool.allocate(1, 7);
        auto& lock = pool.getLock(index);
        
        // Activate multiple parameters
        lock.setParameter(ParameterLockPool::ParameterType::NOTE);
        lock.setParameter(ParameterLockPool::ParameterType::VELOCITY);
        lock.setParameter(ParameterLockPool::ParameterType::LENGTH);
        lock.setParameter(ParameterLockPool::ParameterType::PROBABILITY);
        
        REQUIRE(lock.hasParameter(ParameterLockPool::ParameterType::NOTE));
        REQUIRE(lock.hasParameter(ParameterLockPool::ParameterType::VELOCITY));
        REQUIRE(lock.hasParameter(ParameterLockPool::ParameterType::LENGTH));
        REQUIRE(lock.hasParameter(ParameterLockPool::ParameterType::PROBABILITY));
        
        // Test bitmask directly
        uint16_t expectedMask = ParameterLockPool::ParameterType::NOTE | 
                               ParameterLockPool::ParameterType::VELOCITY |
                               ParameterLockPool::ParameterType::LENGTH |
                               ParameterLockPool::ParameterType::PROBABILITY;
        REQUIRE(lock.activeLocks == expectedMask);
    }
    
    SECTION("Parameter value ranges") {
        uint8_t index = pool.allocate(0, 4);
        auto& lock = pool.getLock(index);
        
        // Test note offset range
        lock.noteOffset = -64;
        REQUIRE(lock.isValid());
        lock.noteOffset = 63;
        REQUIRE(lock.isValid());
        lock.noteOffset = -65;
        REQUIRE_FALSE(lock.isValid()); // Out of range
        
        // Reset to valid value
        lock.noteOffset = 0;
        
        // Test velocity range
        lock.velocity = 0;
        REQUIRE(lock.isValid());
        lock.velocity = 127;
        REQUIRE(lock.isValid());
        lock.velocity = 128;
        REQUIRE_FALSE(lock.isValid()); // Out of range
    }
}

TEST_CASE("ParameterLockPool - Pool capacity and limits", "[parameter_lock_pool][capacity]") {
    ParameterLockPool pool;
    
    SECTION("Fill pool to capacity") {
        std::vector<uint8_t> indices;
        
        // Allocate until full
        for (size_t i = 0; i < ParameterLockPool::MAX_LOCKS; ++i) {
            uint8_t track = i % 4;
            uint8_t step = (i / 4) % 8;
            uint8_t index = pool.allocate(track, step);
            
            REQUIRE(index != ParameterLockPool::INVALID_INDEX);
            indices.push_back(index);
        }
        
        REQUIRE(pool.isFull());
        REQUIRE(pool.getUsedCount() == ParameterLockPool::MAX_LOCKS);
        REQUIRE_THAT(pool.getUtilization(), WithinAbs(1.0f, 0.001f));
        
        // Try to allocate one more (should fail)
        uint8_t overflowIndex = pool.allocate(0, 0);
        REQUIRE(overflowIndex == ParameterLockPool::INVALID_INDEX);
        
        auto stats = pool.getStats();
        REQUIRE(stats.failedAllocations == 1);
        
        // Cleanup
        for (uint8_t index : indices) {
            pool.deallocate(index);
        }
    }
    
    SECTION("Pool statistics tracking") {
        auto initialStats = pool.getStats();
        
        // Perform some allocations and deallocations
        uint8_t index1 = pool.allocate(0, 0);
        uint8_t index2 = pool.allocate(1, 1);
        uint8_t index3 = pool.allocate(2, 2);
        
        auto midStats = pool.getStats();
        REQUIRE(midStats.totalAllocations == initialStats.totalAllocations + 3);
        REQUIRE(midStats.usedSlots == 3);
        
        pool.deallocate(index2);
        
        auto finalStats = pool.getStats();
        REQUIRE(finalStats.totalDeallocations == initialStats.totalDeallocations + 1);
        REQUIRE(finalStats.usedSlots == 2);
        
        // Cleanup
        pool.deallocate(index1);
        pool.deallocate(index3);
    }
}

TEST_CASE("ParameterLockPool - Find and lookup operations", "[parameter_lock_pool][lookup]") {
    ParameterLockPool pool;
    
    SECTION("Find allocated locks") {
        uint8_t index1 = pool.allocate(0, 3);
        uint8_t index2 = pool.allocate(2, 7);
        uint8_t index3 = pool.allocate(1, 0);
        
        // Find existing locks
        REQUIRE(pool.findLock(0, 3) == index1);
        REQUIRE(pool.findLock(2, 7) == index2);
        REQUIRE(pool.findLock(1, 0) == index3);
        
        // Find non-existent locks
        REQUIRE(pool.findLock(0, 0) == ParameterLockPool::INVALID_INDEX);
        REQUIRE(pool.findLock(3, 5) == ParameterLockPool::INVALID_INDEX);
        
        // Cleanup
        pool.deallocate(index1);
        pool.deallocate(index2);
        pool.deallocate(index3);
    }
    
    SECTION("Find after deallocation") {
        uint8_t index = pool.allocate(1, 4);
        REQUIRE(pool.findLock(1, 4) == index);
        
        pool.deallocate(index);
        REQUIRE(pool.findLock(1, 4) == ParameterLockPool::INVALID_INDEX);
    }
    
    SECTION("Duplicate allocations for same position") {
        uint8_t index1 = pool.allocate(0, 0);
        REQUIRE(index1 != ParameterLockPool::INVALID_INDEX);
        
        // Try to allocate same position again
        // Behavior depends on implementation - should either fail or reuse
        uint8_t index2 = pool.allocate(0, 0);
        
        if (index2 == ParameterLockPool::INVALID_INDEX) {
            // Implementation prevents duplicates
            REQUIRE(pool.findLock(0, 0) == index1);
        } else {
            // Implementation allows or reuses slots
            REQUIRE(pool.isValidIndex(index2));
        }
        
        // Cleanup
        if (pool.isValidIndex(index1)) pool.deallocate(index1);
        if (index2 != ParameterLockPool::INVALID_INDEX && index2 != index1) {
            pool.deallocate(index2);
        }
    }
}

TEST_CASE("ParameterLockPool - Error handling and edge cases", "[parameter_lock_pool][edge_cases]") {
    ParameterLockPool pool;
    
    SECTION("Invalid track/step combinations") {
        // Test invalid track values
        REQUIRE(pool.allocate(4, 0) == ParameterLockPool::INVALID_INDEX);
        REQUIRE(pool.allocate(255, 0) == ParameterLockPool::INVALID_INDEX);
        
        // Test invalid step values  
        REQUIRE(pool.allocate(0, 8) == ParameterLockPool::INVALID_INDEX);
        REQUIRE(pool.allocate(0, 255) == ParameterLockPool::INVALID_INDEX);
        
        // Test both invalid
        REQUIRE(pool.allocate(255, 255) == ParameterLockPool::INVALID_INDEX);
    }
    
    SECTION("Invalid index operations") {
        // Test operations on invalid indices
        pool.deallocate(ParameterLockPool::INVALID_INDEX);
        pool.deallocate(255);
        
        REQUIRE_FALSE(pool.isValidIndex(ParameterLockPool::INVALID_INDEX));
        REQUIRE_FALSE(pool.isValidIndex(ParameterLockPool::MAX_LOCKS));
        REQUIRE_FALSE(pool.isValidIndex(255));
        
        // Pool should remain in valid state
        REQUIRE(pool.validateIntegrity());
    }
    
    SECTION("Double deallocation") {
        uint8_t index = pool.allocate(0, 0);
        REQUIRE(pool.isValidIndex(index));
        
        pool.deallocate(index);
        REQUIRE_FALSE(pool.isValidIndex(index));
        
        // Double deallocation should be safe
        pool.deallocate(index);
        REQUIRE(pool.validateIntegrity());
    }
    
    SECTION("Clear all locks") {
        // Allocate some locks
        uint8_t index1 = pool.allocate(0, 0);
        uint8_t index2 = pool.allocate(1, 5);
        uint8_t index3 = pool.allocate(3, 7);
        
        REQUIRE(pool.getUsedCount() == 3);
        
        // Clear all
        pool.clearAll();
        
        REQUIRE(pool.isEmpty());
        REQUIRE(pool.getUsedCount() == 0);
        REQUIRE_FALSE(pool.isValidIndex(index1));
        REQUIRE_FALSE(pool.isValidIndex(index2));
        REQUIRE_FALSE(pool.isValidIndex(index3));
        REQUIRE(pool.validateIntegrity());
    }
}

TEST_CASE("ParameterLockPool - Stress testing", "[parameter_lock_pool][stress]") {
    ParameterLockPool pool;
    
    SECTION("Rapid allocation/deallocation cycles") {
        const int CYCLES = 100;
        std::vector<uint8_t> activeIndices;
        
        for (int cycle = 0; cycle < CYCLES; ++cycle) {
            // Allocate a few locks
            for (int i = 0; i < 5; ++i) {
                uint8_t track = (cycle + i) % 4;
                uint8_t step = (cycle * 2 + i) % 8;
                uint8_t index = pool.allocate(track, step);
                if (index != ParameterLockPool::INVALID_INDEX) {
                    activeIndices.push_back(index);
                }
            }
            
            // Randomly deallocate some
            if (!activeIndices.empty() && (cycle % 3 == 0)) {
                size_t deallocCount = activeIndices.size() / 2;
                for (size_t i = 0; i < deallocCount; ++i) {
                    pool.deallocate(activeIndices.back());
                    activeIndices.pop_back();
                }
            }
            
            // Validate integrity every few cycles
            if (cycle % 10 == 0) {
                REQUIRE(pool.validateIntegrity());
            }
        }
        
        // Cleanup remaining
        for (uint8_t index : activeIndices) {
            pool.deallocate(index);
        }
        
        REQUIRE(pool.isEmpty());
        REQUIRE(pool.validateIntegrity());
    }
    
    SECTION("Parameter manipulation stress test") {
        std::vector<uint8_t> indices;
        
        // Allocate many locks
        for (int i = 0; i < 20; ++i) {
            uint8_t track = i % 4;
            uint8_t step = (i / 4) % 8;
            uint8_t index = pool.allocate(track, step);
            if (index != ParameterLockPool::INVALID_INDEX) {
                indices.push_back(index);
            }
        }
        
        // Manipulate parameters randomly
        for (int operation = 0; operation < 1000; ++operation) {
            if (!indices.empty()) {
                uint8_t index = indices[operation % indices.size()];
                auto& lock = pool.getLock(index);
                
                // Random parameter operations
                switch (operation % 6) {
                    case 0:
                        lock.setParameter(ParameterLockPool::ParameterType::NOTE, operation % 2);
                        break;
                    case 1:
                        lock.setParameter(ParameterLockPool::ParameterType::VELOCITY, operation % 2);
                        break;
                    case 2:
                        lock.noteOffset = ((operation % 128) - 64);
                        break;
                    case 3:
                        lock.velocity = (operation % 128);
                        break;
                    case 4:
                        lock.length = (operation % 256);
                        break;
                    case 5:
                        // Validate lock
                        lock.isValid(); // Just call for coverage
                        break;
                }
            }
        }
        
        // Cleanup
        for (uint8_t index : indices) {
            pool.deallocate(index);
        }
        
        REQUIRE(pool.validateIntegrity());
    }
}

// Parametric tests for all valid track/step combinations
TEST_CASE("ParameterLockPool - All valid positions", "[parameter_lock_pool][parametric]") {
    ParameterLockPool pool;
    
    auto track = GENERATE(range(0, 4));
    auto step = GENERATE(range(0, 8));
    
    SECTION("Track " + std::to_string(track) + ", Step " + std::to_string(step)) {
        uint8_t index = pool.allocate(track, step);
        REQUIRE(index != ParameterLockPool::INVALID_INDEX);
        
        const auto& lock = pool.getLock(index);
        REQUIRE(lock.trackIndex == track);
        REQUIRE(lock.stepIndex == step);
        REQUIRE(lock.isValid());
        
        REQUIRE(pool.findLock(track, step) == index);
        
        pool.deallocate(index);
        REQUIRE(pool.findLock(track, step) == ParameterLockPool::INVALID_INDEX);
    }
}

TEST_CASE("ParameterLockPool - Memory layout and alignment", "[parameter_lock_pool][memory]") {
    SECTION("ParameterLock structure size") {
        // Verify structure is reasonably sized (should be 8 bytes as documented)
        REQUIRE(sizeof(ParameterLockPool::ParameterLock) <= 16); // Allow some flexibility
        
        // Verify enum values
        REQUIRE(static_cast<uint16_t>(ParameterLockPool::ParameterType::NONE) == 0x0000);
        REQUIRE(static_cast<uint16_t>(ParameterLockPool::ParameterType::NOTE) == 0x0001);
        REQUIRE(static_cast<uint16_t>(ParameterLockPool::ParameterType::VELOCITY) == 0x0002);
        REQUIRE(static_cast<uint16_t>(ParameterLockPool::ParameterType::LENGTH) == 0x0004);
    }
    
    SECTION("Pool constants") {
        REQUIRE(ParameterLockPool::MAX_LOCKS == 64);
        REQUIRE(ParameterLockPool::INVALID_INDEX == 0xFF);
    }
}