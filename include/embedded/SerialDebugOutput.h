#ifndef SERIAL_DEBUG_OUTPUT_H
#define SERIAL_DEBUG_OUTPUT_H

#include "IDebugOutput.h"

class SerialDebugOutput : public IDebugOutput {
public:
    SerialDebugOutput();
    
    void log(const std::string& message) override;
    void logf(const char* format, ...) override;
};

#endif