#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <stdint.h>
#include <memory>
#include <vector>
#include <chrono>

// Forward declarations
class AdaptiveButtonTracker;
class SequencerStateManager;
class ParameterLockPool;
class ControlGrid;

/**
 * Test timing utilities for deterministic testing
 */
class TestTimer {
private:
    uint32_t startTime_;
    uint32_t currentTime_;
    
public:
    TestTimer(uint32_t startTime = 1000) : startTime_(startTime), currentTime_(startTime) {}
    
    uint32_t getCurrentTime() const { return currentTime_; }
    uint32_t getElapsedTime() const { return currentTime_ - startTime_; }
    
    void advance(uint32_t ms) { currentTime_ += ms; }
    void reset(uint32_t newStartTime = 1000) { 
        startTime_ = newStartTime; 
        currentTime_ = newStartTime; 
    }
    
    // Convenience methods for common delays
    void waitForHoldThreshold() { advance(500); }
    void shortDelay() { advance(10); }
    void mediumDelay() { advance(100); }
    void longDelay() { advance(1000); }
};

/**
 * Test result tracking and validation
 */
struct TestMetrics {
    uint32_t totalTestTime;
    uint32_t setupTime;
    uint32_t executionTime;
    uint32_t cleanupTime;
    
    uint32_t buttonsPressed;
    uint32_t holdsDetected;
    uint32_t parameterLocksCreated;
    uint32_t parameterAdjustments;
    
    bool allTestsPassed;
    uint32_t failureCount;
    std::vector<std::string> failureReasons;
    
    TestMetrics() : totalTestTime(0), setupTime(0), executionTime(0), cleanupTime(0),
                   buttonsPressed(0), holdsDetected(0), parameterLocksCreated(0), 
                   parameterAdjustments(0), allTestsPassed(true), failureCount(0) {}
    
    void recordFailure(const std::string& reason) {
        allTestsPassed = false;
        failureCount++;
        failureReasons.push_back(reason);
    }
    
    double getSuccessRate() const {
        if (failureCount == 0) return 100.0;
        uint32_t totalOperations = buttonsPressed + holdsDetected + parameterLocksCreated + parameterAdjustments;
        if (totalOperations == 0) return 0.0;
        return ((double)(totalOperations - failureCount) / totalOperations) * 100.0;
    }
};

/**
 * Common test patterns and sequences
 */
class ParameterLockTestPatterns {
public:
    // Test button combinations that cover all functionality
    struct TestCase {
        uint8_t track;
        uint8_t step;
        uint8_t expectedButton;
        uint8_t expectedControlStart;
        const char* description;
    };
    
    static const TestCase ALL_POSITIONS[];
    static const size_t ALL_POSITIONS_COUNT;
    
    // Common parameter lock scenarios
    struct Scenario {
        const char* name;
        const char* description;
        std::vector<TestCase> steps;
        uint32_t expectedDuration;
        bool shouldSucceed;
    };
    
    static const Scenario BASIC_SCENARIOS[];
    static const size_t BASIC_SCENARIOS_COUNT;
    
    // Edge case patterns
    struct EdgeCase {
        const char* name;
        const char* description;
        std::function<bool()> testFunction;
    };
    
    // Timing patterns for hold detection testing
    struct TimingPattern {
        const char* name;
        uint32_t pressDuration;
        bool shouldTriggerHold;
        const char* description;
    };
    
    static const TimingPattern TIMING_PATTERNS[];
    static const size_t TIMING_PATTERNS_COUNT;
};

/**
 * Test data validation utilities
 */
class TestDataValidator {
public:
    // Validate parameter lock data integrity
    static bool validateParameterLock(const ParameterLockPool& pool, uint8_t index);
    
    // Validate state consistency across components
    static bool validateSystemState(const AdaptiveButtonTracker& tracker,
                                  const SequencerStateManager& stateManager,
                                  const ParameterLockPool& pool);
    
    // Validate control grid mapping consistency
    static bool validateControlGridMapping(const ControlGrid& grid, 
                                         uint8_t heldStep, uint8_t heldTrack);
    
    // Check for memory leaks and resource cleanup
    static bool validateResourceCleanup(const ParameterLockPool& pool);
    
    // Validate timing precision
    static bool validateTimingPrecision(const AdaptiveButtonTracker& tracker,
                                       uint32_t expectedThreshold,
                                       uint32_t tolerance = 1);
};

/**
 * Performance benchmarking utilities
 */
class PerformanceBenchmark {
private:
    std::chrono::high_resolution_clock::time_point startTime_;
    std::vector<uint32_t> samples_;
    
public:
    void startBenchmark();
    void recordSample();
    void endBenchmark();
    
    uint32_t getAverageTime() const;
    uint32_t getMinTime() const;
    uint32_t getMaxTime() const;
    double getStandardDeviation() const;
    
    void reset();
    
    // Benchmark specific operations
    static uint32_t benchmarkHoldDetection(AdaptiveButtonTracker& tracker, 
                                          uint32_t iterations = 100);
    static uint32_t benchmarkStateTransitions(SequencerStateManager& stateManager,
                                            uint32_t iterations = 100);
    static uint32_t benchmarkPoolOperations(ParameterLockPool& pool,
                                          uint32_t iterations = 100);
};

/**
 * Mock implementations for testing
 */
class MockTimeSource {
private:
    uint32_t currentTime_;
    bool realTimeMode_;
    
public:
    MockTimeSource(uint32_t startTime = 1000, bool realTime = false) 
        : currentTime_(startTime), realTimeMode_(realTime) {}
    
    uint32_t getCurrentTime() const {
        if (realTimeMode_) {
            auto now = std::chrono::steady_clock::now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count();
        }
        return currentTime_;
    }
    
    void advance(uint32_t ms) { 
        if (!realTimeMode_) {
            currentTime_ += ms; 
        }
    }
    
    void setRealTimeMode(bool enabled) { realTimeMode_ = enabled; }
    void setTime(uint32_t time) { 
        if (!realTimeMode_) {
            currentTime_ = time; 
        }
    }
};

/**
 * Test report generation
 */
class TestReportGenerator {
private:
    std::vector<TestMetrics> testResults_;
    
public:
    void addTestResult(const TestMetrics& metrics);
    
    void generateSummaryReport(std::ostream& out) const;
    void generateDetailedReport(std::ostream& out) const;
    void generatePerformanceReport(std::ostream& out) const;
    
    // Export to common formats
    void exportToCsv(const std::string& filename) const;
    void exportToJson(const std::string& filename) const;
    
    // Statistics
    double getOverallSuccessRate() const;
    uint32_t getTotalTestTime() const;
    uint32_t getAverageTestTime() const;
};

/**
 * Common assertion helpers for parameter lock testing
 */
#define REQUIRE_PARAMETER_LOCK_VALID(pool, index) \
    do { \
        REQUIRE(pool.isValidIndex(index)); \
        REQUIRE(pool.getLock(index).isValid()); \
        REQUIRE(TestDataValidator::validateParameterLock(pool, index)); \
    } while(0)

#define REQUIRE_SYSTEM_STATE_CONSISTENT(tracker, stateManager, pool) \
    do { \
        REQUIRE(tracker.validateState()); \
        REQUIRE(stateManager.validateState()); \
        REQUIRE(pool.validateIntegrity()); \
        REQUIRE(TestDataValidator::validateSystemState(tracker, stateManager, pool)); \
    } while(0)

#define REQUIRE_HOLD_DETECTION_PRECISE(tracker, button, expectedTime, tolerance) \
    do { \
        REQUIRE(tracker.isHeld(button)); \
        REQUIRE(TestDataValidator::validateTimingPrecision(tracker, expectedTime, tolerance)); \
    } while(0)

#define REQUIRE_CONTROL_GRID_VALID(grid, step, track) \
    do { \
        auto mapping = grid.getMapping(step, track); \
        REQUIRE(mapping.isValid); \
        REQUIRE(TestDataValidator::validateControlGridMapping(grid, step, track)); \
    } while(0)

// Performance testing macros
#define BENCHMARK_OPERATION(operation, iterations, maxTimeMs) \
    do { \
        PerformanceBenchmark benchmark; \
        benchmark.startBenchmark(); \
        for (uint32_t i = 0; i < iterations; ++i) { \
            operation; \
        } \
        benchmark.endBenchmark(); \
        uint32_t avgTime = benchmark.getAverageTime(); \
        REQUIRE(avgTime <= maxTimeMs); \
        INFO("Average time per operation: " << avgTime << "ms"); \
    } while(0)

#endif // TEST_UTILS_H