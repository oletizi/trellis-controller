#include "FileDebugOutput.h"
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <cstdarg>
#include <cstdio>

FileDebugOutput::FileDebugOutput() {
    try {
        // Ensure log directory exists
        ensureLogDirectoryExists();
        
        // Create timestamped filename
        std::string filename = createTimestampedFilename();
        logFilePath_ = "./logs/simulation/" + filename;
        
        // Open log file for writing
        logFile_.open(logFilePath_, std::ios::out | std::ios::trunc);
        if (!logFile_.is_open()) {
            throw std::runtime_error("FileDebugOutput: Failed to open log file: " + logFilePath_ + 
                                   " - check directory permissions");
        }
        
        // Write header to log file
        std::string timestamp = getCurrentTimestamp();
        logFile_ << timestamp << " === Trellis Controller Simulation Debug Log ===" << std::endl;
        logFile_ << timestamp << " Log file: " << logFilePath_ << std::endl;
        logFile_.flush();
        
    } catch (const std::exception& e) {
        // Clean up and re-throw with context
        if (logFile_.is_open()) {
            logFile_.close();
        }
        throw std::runtime_error(std::string("FileDebugOutput initialization failed: ") + e.what());
    }
}

FileDebugOutput::~FileDebugOutput() {
    std::lock_guard<std::mutex> lock(fileMutex_);
    
    if (logFile_.is_open()) {
        try {
            std::string timestamp = getCurrentTimestamp();
            logFile_ << timestamp << " === Debug log session ended ===" << std::endl;
            logFile_.flush();
        } catch (const std::exception&) {
            // Ignore errors during destruction
        }
        logFile_.close();
    }
}

void FileDebugOutput::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(fileMutex_);
    
    if (!logFile_.is_open()) {
        throw std::runtime_error("FileDebugOutput: Log file is not open for writing");
    }
    
    try {
        std::string timestamp = getCurrentTimestamp();
        logFile_ << timestamp << " " << message << std::endl;
        logFile_.flush();  // Ensure immediate write for reliability
        
    } catch (const std::exception& e) {
        throw std::runtime_error("FileDebugOutput: Failed to write to log file: " + 
                               std::string(e.what()));
    }
}

void FileDebugOutput::logf(const char* format, ...) {
    if (!format) {
        throw std::invalid_argument("FileDebugOutput: Format string cannot be null");
    }
    
    // Format the message using vsnprintf for thread safety
    char buffer[512];  // Reasonable buffer size for debug messages
    va_list args;
    va_start(args, format);
    
    int result = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (result < 0) {
        throw std::runtime_error("FileDebugOutput: Failed to format debug message");
    }
    
    if (result >= static_cast<int>(sizeof(buffer))) {
        // Message was truncated - this is acceptable for debug output
        // Continue with truncated message rather than failing
        buffer[sizeof(buffer) - 1] = '\0';
    }
    
    // Use the string-based log method
    log(std::string(buffer));
}

const std::string& FileDebugOutput::getLogFilePath() const {
    return logFilePath_;
}

bool FileDebugOutput::isOpen() const {
    std::lock_guard<std::mutex> lock(fileMutex_);
    return logFile_.is_open();
}

std::string FileDebugOutput::createTimestampedFilename() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << "simulation_" 
       << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") 
       << ".log";
    
    return ss.str();
}

void FileDebugOutput::ensureLogDirectoryExists() const {
    const std::string logDir = "./logs/simulation";
    
    try {
        // Create directory structure if it doesn't exist
        std::filesystem::create_directories(logDir);
        
        // Verify directory was created and is writable
        if (!std::filesystem::exists(logDir)) {
            throw std::runtime_error("Failed to create log directory: " + logDir);
        }
        
        if (!std::filesystem::is_directory(logDir)) {
            throw std::runtime_error("Log path exists but is not a directory: " + logDir);
        }
        
    } catch (const std::filesystem::filesystem_error& e) {
        throw std::runtime_error("FileDebugOutput: Failed to create log directory '" + logDir + 
                               "': " + e.what());
    }
}

std::string FileDebugOutput::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count() << "]";
    
    return ss.str();
}