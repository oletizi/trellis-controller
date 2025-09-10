#ifndef FILE_DEBUG_OUTPUT_H
#define FILE_DEBUG_OUTPUT_H

#include "IDebugOutput.h"
#include <fstream>
#include <string>
#include <mutex>

/**
 * @brief File-based debug output implementation with timestamped logging
 * 
 * FileDebugOutput writes debug messages to timestamped log files in ./logs/simulation/
 * directory. Each instance creates a unique log file named with timestamp format:
 * simulation_YYYYMMDD_HHMMSS.log
 * 
 * Features:
 * - Automatic directory creation if not exists
 * - Thread-safe file operations
 * - Automatic file flushing for reliability
 * - Timestamped messages within log files
 * - RAII-based resource management
 * - Descriptive error handling (no fallbacks)
 */
class FileDebugOutput : public IDebugOutput {
public:
    /**
     * @brief Construct file debug output with timestamped filename
     * 
     * Creates log file in ./logs/simulation/ with format:
     * simulation_YYYYMMDD_HHMMSS.log
     * 
     * @throws std::runtime_error if log directory cannot be created or file cannot be opened
     */
    FileDebugOutput();
    
    /**
     * @brief Destructor ensures file is properly closed
     */
    ~FileDebugOutput() override;
    
    /**
     * @brief Log message to file with timestamp
     * 
     * Writes message to log file with ISO 8601 timestamp prefix.
     * Format: [YYYY-MM-DD HH:MM:SS.mmm] message
     * 
     * @param message Debug message to log
     * @throws std::runtime_error if file write fails
     */
    void log(const std::string& message) override;
    
    /**
     * @brief Log formatted message to file with timestamp
     * 
     * Formats message using printf-style formatting then logs with timestamp.
     * 
     * @param format Printf-style format string
     * @param ... Variable arguments for formatting
     * @throws std::invalid_argument if format is null
     * @throws std::runtime_error if formatting or file write fails
     */
    void logf(const char* format, ...) override;
    
    /**
     * @brief Get the full path to the log file
     * 
     * @return Absolute path to the log file
     */
    const std::string& getLogFilePath() const;
    
    /**
     * @brief Check if log file is open and ready for writing
     * 
     * @return true if file is open and ready
     */
    bool isOpen() const;

private:
    std::string logFilePath_;           ///< Full path to log file
    std::ofstream logFile_;             ///< Output file stream
    mutable std::mutex fileMutex_;      ///< Protects file operations
    
    /**
     * @brief Create timestamped filename for log file
     * 
     * @return Filename in format: simulation_YYYYMMDD_HHMMSS.log
     */
    std::string createTimestampedFilename() const;
    
    /**
     * @brief Create log directory if it doesn't exist
     * 
     * Creates ./logs/simulation/ directory structure.
     * 
     * @throws std::runtime_error if directory creation fails
     */
    void ensureLogDirectoryExists() const;
    
    /**
     * @brief Get current timestamp as formatted string
     * 
     * @return Timestamp in format: [YYYY-MM-DD HH:MM:SS.mmm]
     */
    std::string getCurrentTimestamp() const;
};

#endif // FILE_DEBUG_OUTPUT_H