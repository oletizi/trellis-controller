#ifndef COMPOSITE_DEBUG_OUTPUT_H
#define COMPOSITE_DEBUG_OUTPUT_H

#include "IDebugOutput.h"
#include <vector>
#include <memory>
#include <mutex>

/**
 * @brief Composite pattern implementation for routing debug output to multiple destinations
 * 
 * CompositeDebugOutput allows routing debug messages to multiple IDebugOutput instances
 * simultaneously (e.g., console and file). Provides thread-safe access and proper
 * error handling following project guidelines.
 * 
 * Thread Safety: All methods are thread-safe using mutex protection
 * Error Handling: Continues operation if individual outputs fail, logs errors
 * Memory Management: Uses RAII with smart pointers for automatic cleanup
 */
class CompositeDebugOutput : public IDebugOutput {
public:
    /**
     * @brief Construct empty composite debug output
     */
    CompositeDebugOutput();
    
    /**
     * @brief Destructor ensures proper cleanup of all outputs
     */
    ~CompositeDebugOutput() override = default;
    
    /**
     * @brief Add a debug output destination
     * 
     * Takes ownership of the debug output instance and adds it to the routing list.
     * Thread-safe operation.
     * 
     * @param output Unique pointer to debug output implementation
     * @throws std::invalid_argument if output is nullptr
     */
    void addOutput(std::unique_ptr<IDebugOutput> output);
    
    /**
     * @brief Log message to all registered outputs
     * 
     * Sends message to all registered debug outputs. If any individual output fails,
     * the operation continues with remaining outputs (no fallback behavior per guidelines).
     * 
     * @param message Debug message to log
     */
    void log(const std::string& message) override;
    
    /**
     * @brief Log formatted message to all registered outputs
     * 
     * Formats message using printf-style formatting then sends to all outputs.
     * 
     * @param format Printf-style format string
     * @param ... Variable arguments for formatting
     */
    void logf(const char* format, ...) override;
    
    /**
     * @brief Get count of registered debug outputs
     * 
     * Thread-safe count of currently registered outputs.
     * 
     * @return Number of registered debug outputs
     */
    size_t getOutputCount() const;
    
    /**
     * @brief Remove all registered debug outputs
     * 
     * Thread-safe removal of all debug outputs. Useful for cleanup scenarios.
     */
    void clear();

private:
    std::vector<std::unique_ptr<IDebugOutput>> outputs_;
    mutable std::mutex outputsMutex_;  ///< Protects outputs_ vector
};

#endif // COMPOSITE_DEBUG_OUTPUT_H