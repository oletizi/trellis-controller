#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <string>
#include <sstream>

/**
 * Minimal JSON utilities for state serialization.
 * Hand-written to avoid external dependencies.
 */
class JsonUtils {
public:
    /**
     * Escape string for JSON format
     */
    static std::string escapeString(const std::string& str);
    
    /**
     * Unescape JSON string
     */
    static std::string unescapeString(const std::string& str);
    
    /**
     * Create JSON key-value pair for primitive types
     */
    static std::string keyValue(const std::string& key, bool value, bool isLast = false);
    static std::string keyValue(const std::string& key, int value, bool isLast = false);
    static std::string keyValue(const std::string& key, uint32_t value, bool isLast = false);
    static std::string keyValue(const std::string& key, const std::string& value, bool isLast = false);
    
    /**
     * Parse value from JSON string
     */
    static bool parseBool(const std::string& json, const std::string& key, bool& value);
    static bool parseInt(const std::string& json, const std::string& key, int& value);
    static bool parseUInt(const std::string& json, const std::string& key, uint32_t& value);
    static bool parseString(const std::string& json, const std::string& key, std::string& value);
    
    /**
     * Find JSON object boundaries
     */
    static bool findObject(const std::string& json, const std::string& key, 
                          size_t& start, size_t& end);
    
    /**
     * Find JSON array boundaries  
     */
    static bool findArray(const std::string& json, const std::string& key,
                         size_t& start, size_t& end);
    
    /**
     * Create JSON object wrapper
     */
    static std::string wrapObject(const std::string& content);
    
    /**
     * Create JSON array wrapper
     */
    static std::string wrapArray(const std::string& content);
    
    /**
     * Format JSON with indentation for readability
     */
    static std::string formatJson(const std::string& json, int indent = 2);
    
private:
    /**
     * Find key in JSON and return value start/end positions
     */
    static bool findValue(const std::string& json, const std::string& key,
                         size_t& start, size_t& end);
    
    /**
     * Skip whitespace in JSON
     */
    static size_t skipWhitespace(const std::string& json, size_t pos);
    
    /**
     * Find matching bracket/brace
     */
    static size_t findMatchingBracket(const std::string& json, size_t start, char open, char close);
};

#endif // JSON_UTILS_H