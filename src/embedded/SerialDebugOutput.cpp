#include "SerialDebugOutput.h"
#include <cstdarg>

#ifdef ARDUINO
#include <Arduino.h>
#endif

SerialDebugOutput::SerialDebugOutput() {
}

void SerialDebugOutput::log(const std::string& message) {
#ifdef ARDUINO
    Serial.println(message.c_str());
#endif
}

void SerialDebugOutput::logf(const char* format, ...) {
#ifdef ARDUINO
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Serial.println(buffer);
#endif
}