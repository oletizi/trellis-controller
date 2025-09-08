#ifndef DEBUG_LOGGER_H
#define DEBUG_LOGGER_H

#include <string>
#include <fstream>
#include <cstdio>

class DebugLogger {
public:
    static DebugLogger& getInstance() {
        static DebugLogger instance;
        return instance;
    }
    
    void init(const std::string& filename = "debug.log") {
        filename_ = filename;
        // Clear the log file
        logFile_.open(filename_, std::ios::out | std::ios::trunc);
        if (logFile_.is_open()) {
            logFile_ << "=== Debug Log Started ===" << std::endl;
            logFile_.flush();
            logFile_.close();
        }
    }
    
    void log(const std::string& category, const std::string& message) {
        #ifdef ARDUINO
        Serial.print("[");
        Serial.print(category.c_str());
        Serial.print("] ");
        Serial.println(message.c_str());
        #else
        logFile_.open(filename_, std::ios::out | std::ios::app);
        if (logFile_.is_open()) {
            logFile_ << "[" << category << "] " << message << std::endl;
            logFile_.flush();
            logFile_.close();
        }
        #endif
    }
    
    void logf(const std::string& category, const char* format, ...) {
        char buffer[512];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        log(category, std::string(buffer));
    }

private:
    DebugLogger() = default;
    std::string filename_;
    std::ofstream logFile_;
};

// Convenience macros
#define DEBUG_LOG(category, message) DebugLogger::getInstance().log(category, message)
#define DEBUG_LOGF(category, format, ...) DebugLogger::getInstance().logf(category, format, __VA_ARGS__)

#endif // DEBUG_LOGGER_H