#include "CompositeDebugOutput.h"
#include <cstdarg>
#include <cstdio>
#include <stdexcept>
#include <iostream>

CompositeDebugOutput::CompositeDebugOutput() {
    // Initialize empty - outputs added via addOutput()
}

void CompositeDebugOutput::addOutput(std::unique_ptr<IDebugOutput> output) {
    if (!output) {
        throw std::invalid_argument("CompositeDebugOutput: Cannot add null debug output");
    }
    
    std::lock_guard<std::mutex> lock(outputsMutex_);
    outputs_.push_back(std::move(output));
}

void CompositeDebugOutput::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(outputsMutex_);
    
    // Send message to all registered outputs
    // Continue operation even if individual outputs fail (per project guidelines)
    for (auto& output : outputs_) {
        try {
            if (output) {
                output->log(message);
            }
        } catch (const std::exception& e) {
            // Log error to stderr but continue with other outputs
            // Don't use fallbacks - just report the error
            std::cerr << "CompositeDebugOutput: Output failed - " << e.what() << std::endl;
        }
    }
}

void CompositeDebugOutput::logf(const char* format, ...) {
    if (!format) {
        throw std::invalid_argument("CompositeDebugOutput: Format string cannot be null");
    }
    
    // Format the message using vsnprintf for thread safety
    char buffer[512];  // Reasonable buffer size for debug messages
    va_list args;
    va_start(args, format);
    
    int result = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (result < 0) {
        throw std::runtime_error("CompositeDebugOutput: Failed to format debug message");
    }
    
    if (result >= static_cast<int>(sizeof(buffer))) {
        // Message was truncated - this is acceptable for debug output
        // Continue with truncated message rather than failing
        buffer[sizeof(buffer) - 1] = '\0';
    }
    
    // Use the string-based log method
    log(std::string(buffer));
}

size_t CompositeDebugOutput::getOutputCount() const {
    std::lock_guard<std::mutex> lock(outputsMutex_);
    return outputs_.size();
}

void CompositeDebugOutput::clear() {
    std::lock_guard<std::mutex> lock(outputsMutex_);
    outputs_.clear();
}