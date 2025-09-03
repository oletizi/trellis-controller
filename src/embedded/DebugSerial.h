#ifndef DEBUG_SERIAL_H
#define DEBUG_SERIAL_H

#include <stdint.h>
#include <stdarg.h>

#ifdef __SAMD51J19A__

// Debug serial interface for SAMD51
// Uses USB CDC for serial communication over native USB port
class DebugSerial {
public:
    static void init();
    static void print(const char* str);
    static void println(const char* str = "");
    static void printf(const char* format, ...);
    static void printHex(uint32_t value, uint8_t digits = 8);
    static void printBinary(uint32_t value, uint8_t bits = 32);
    
    // Performance monitoring
    static void printTimestamp(const char* label = nullptr);
    static void printMemoryUsage();
    static void printSystemStatus();
    
private:
    static bool initialized_;
    static uint32_t startTime_;
    static char buffer_[256];
    
    static void writeChar(char c);
    static void writeString(const char* str);
};

// Convenience macros for debug output
#define DEBUG_INIT()           DebugSerial::init()
#define DEBUG_PRINT(str)       DebugSerial::print(str)
#define DEBUG_PRINTLN(str)     DebugSerial::println(str)
#define DEBUG_PRINTF(...)      DebugSerial::printf(__VA_ARGS__)
#define DEBUG_HEX(val, digits) DebugSerial::printHex(val, digits)
#define DEBUG_BIN(val, bits)   DebugSerial::printBinary(val, bits)
#define DEBUG_TIMESTAMP(label) DebugSerial::printTimestamp(label)
#define DEBUG_MEMORY()         DebugSerial::printMemoryUsage()
#define DEBUG_STATUS()         DebugSerial::printSystemStatus()

#else
// Stubs for non-embedded builds
#define DEBUG_INIT()           
#define DEBUG_PRINT(str)       
#define DEBUG_PRINTLN(str)     
#define DEBUG_PRINTF(...)      
#define DEBUG_HEX(val, digits) 
#define DEBUG_BIN(val, bits)   
#define DEBUG_TIMESTAMP(label) 
#define DEBUG_MEMORY()         
#define DEBUG_STATUS()         
#endif

#endif