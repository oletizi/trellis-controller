#include "JsonUtils.h"
#include <sstream>
#include <algorithm>

std::string JsonUtils::escapeString(const std::string& str) {
    std::string result;
    result.reserve(str.length() + str.length() / 8); // Estimate for escaped chars
    
    for (char c : str) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (c < 32) {
                    result += "\\u";
                    char hex[5];
                    snprintf(hex, sizeof(hex), "%04x", static_cast<unsigned char>(c));
                    result += hex;
                } else {
                    result += c;
                }
                break;
        }
    }
    return result;
}

std::string JsonUtils::unescapeString(const std::string& str) {
    std::string result;
    result.reserve(str.length());
    
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            switch (str[i + 1]) {
                case '"':  result += '"'; ++i; break;
                case '\\': result += '\\'; ++i; break;
                case 'b':  result += '\b'; ++i; break;
                case 'f':  result += '\f'; ++i; break;
                case 'n':  result += '\n'; ++i; break;
                case 'r':  result += '\r'; ++i; break;
                case 't':  result += '\t'; ++i; break;
                case 'u':
                    if (i + 5 < str.length()) {
                        // Parse \uXXXX
                        std::string hex = str.substr(i + 2, 4);
                        char* end;
                        long val = strtol(hex.c_str(), &end, 16);
                        if (end - hex.c_str() == 4) {
                            result += static_cast<char>(val);
                            i += 5;
                        } else {
                            result += str[i]; // Invalid escape, keep as-is
                        }
                    } else {
                        result += str[i]; // Invalid escape
                    }
                    break;
                default:
                    result += str[i]; // Invalid escape, keep backslash
                    break;
            }
        } else {
            result += str[i];
        }
    }
    return result;
}

std::string JsonUtils::keyValue(const std::string& key, bool value, bool isLast) {
    std::string result = "\"" + escapeString(key) + "\": " + (value ? "true" : "false");
    if (!isLast) result += ",";
    return result;
}

std::string JsonUtils::keyValue(const std::string& key, int value, bool isLast) {
    std::string result = "\"" + escapeString(key) + "\": " + std::to_string(value);
    if (!isLast) result += ",";
    return result;
}

std::string JsonUtils::keyValue(const std::string& key, uint32_t value, bool isLast) {
    std::string result = "\"" + escapeString(key) + "\": " + std::to_string(value);
    if (!isLast) result += ",";
    return result;
}

std::string JsonUtils::keyValue(const std::string& key, const std::string& value, bool isLast) {
    std::string result = "\"" + escapeString(key) + "\": \"" + escapeString(value) + "\"";
    if (!isLast) result += ",";
    return result;
}

bool JsonUtils::parseBool(const std::string& json, const std::string& key, bool& value) {
    size_t start, end;
    if (!findValue(json, key, start, end)) {
        return false;
    }
    
    std::string val = json.substr(start, end - start);
    if (val == "true") {
        value = true;
        return true;
    } else if (val == "false") {
        value = false;
        return true;
    }
    return false;
}

bool JsonUtils::parseInt(const std::string& json, const std::string& key, int& value) {
    size_t start, end;
    if (!findValue(json, key, start, end)) {
        return false;
    }
    
    std::string val = json.substr(start, end - start);
    try {
        value = std::stoi(val);
        return true;
    } catch (...) {
        return false;
    }
}

bool JsonUtils::parseUInt(const std::string& json, const std::string& key, uint32_t& value) {
    size_t start, end;
    if (!findValue(json, key, start, end)) {
        return false;
    }
    
    std::string val = json.substr(start, end - start);
    try {
        unsigned long ul = std::stoul(val);
        value = static_cast<uint32_t>(ul);
        return true;
    } catch (...) {
        return false;
    }
}

bool JsonUtils::parseString(const std::string& json, const std::string& key, std::string& value) {
    size_t start, end;
    if (!findValue(json, key, start, end)) {
        return false;
    }
    
    std::string val = json.substr(start, end - start);
    // Remove quotes if present
    if (val.length() >= 2 && val[0] == '"' && val[val.length()-1] == '"') {
        value = unescapeString(val.substr(1, val.length()-2));
    } else {
        value = val;
    }
    return true;
}

std::string JsonUtils::wrapObject(const std::string& content) {
    return "{\n" + content + "\n}";
}

std::string JsonUtils::wrapArray(const std::string& content) {
    return "[\n" + content + "\n]";
}

bool JsonUtils::findValue(const std::string& json, const std::string& key, size_t& start, size_t& end) {
    // Find "key":
    std::string searchKey = "\"" + escapeString(key) + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) {
        return false;
    }
    
    // Find the colon after the key
    size_t colonPos = json.find(':', keyPos + searchKey.length());
    if (colonPos == std::string::npos) {
        return false;
    }
    
    // Skip whitespace after colon
    start = skipWhitespace(json, colonPos + 1);
    if (start >= json.length()) {
        return false;
    }
    
    // Find end of value
    if (json[start] == '"') {
        // String value
        end = start + 1;
        while (end < json.length() && (json[end] != '"' || json[end-1] == '\\')) {
            ++end;
        }
        if (end < json.length()) ++end; // Include closing quote
    } else if (json[start] == '{') {
        // Object value
        end = findMatchingBracket(json, start, '{', '}');
        if (end < json.length()) ++end;
    } else if (json[start] == '[') {
        // Array value
        end = findMatchingBracket(json, start, '[', ']');
        if (end < json.length()) ++end;
    } else {
        // Number, boolean, or null
        end = start;
        while (end < json.length() && json[end] != ',' && json[end] != '}' && 
               json[end] != ']' && json[end] != '\n' && json[end] != '\r') {
            ++end;
        }
        // Trim whitespace from end
        while (end > start && (json[end-1] == ' ' || json[end-1] == '\t')) {
            --end;
        }
    }
    
    return end > start;
}

size_t JsonUtils::skipWhitespace(const std::string& json, size_t pos) {
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || 
           json[pos] == '\n' || json[pos] == '\r')) {
        ++pos;
    }
    return pos;
}

size_t JsonUtils::findMatchingBracket(const std::string& json, size_t start, char open, char close) {
    int depth = 0;
    for (size_t i = start; i < json.length(); ++i) {
        if (json[i] == open) {
            ++depth;
        } else if (json[i] == close) {
            --depth;
            if (depth == 0) {
                return i;
            }
        }
    }
    return json.length(); // Not found
}

std::string JsonUtils::formatJson(const std::string& json, int indent) {
    std::string result;
    std::string indentStr(indent, ' ');
    int currentIndent = 0;
    bool inString = false;
    bool escape = false;
    
    for (size_t i = 0; i < json.length(); ++i) {
        char c = json[i];
        
        if (!inString) {
            if (c == '"') {
                inString = true;
                result += c;
            } else if (c == '{' || c == '[') {
                result += c;
                result += '\n';
                currentIndent += indent;
                for (int j = 0; j < currentIndent; ++j) result += ' ';
            } else if (c == '}' || c == ']') {
                result += '\n';
                currentIndent -= indent;
                for (int j = 0; j < currentIndent; ++j) result += ' ';
                result += c;
            } else if (c == ',') {
                result += c;
                result += '\n';
                for (int j = 0; j < currentIndent; ++j) result += ' ';
            } else if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                result += c;
            }
        } else {
            result += c;
            if (escape) {
                escape = false;
            } else if (c == '\\') {
                escape = true;
            } else if (c == '"') {
                inString = false;
            }
        }
    }
    
    return result;
}