#ifndef MOCK_DEBUG_OUTPUT_H
#define MOCK_DEBUG_OUTPUT_H

#include "IDebugOutput.h"
#include <string>
#include <vector>
#include <cstdarg>
#include <cstdio>

/**
 * @brief Mock implementation of IDebugOutput for testing
 * 
 * Captures all debug messages for verification in tests.
 * Provides access to logged messages for assertions.
 */
class MockDebugOutput : public IDebugOutput {
public:
    MockDebugOutput() = default;
    
    void log(const std::string& message) override {
        messages.push_back(message);
    }
    
    void logf(const char* format, ...) override {
        va_list args;
        va_start(args, format);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        log(std::string(buffer));
    }
    
    // Test helpers
    const std::vector<std::string>& getMessages() const {
        return messages;
    }
    
    void clearMessages() {
        messages.clear();
    }
    
    size_t getMessageCount() const {
        return messages.size();
    }
    
    bool hasMessage(const std::string& message) const {
        for (const auto& msg : messages) {
            if (msg.find(message) != std::string::npos) {
                return true;
            }
        }
        return false;
    }
    
    std::string getLastMessage() const {
        return messages.empty() ? "" : messages.back();
    }
    
private:
    std::vector<std::string> messages;
};

#endif // MOCK_DEBUG_OUTPUT_H