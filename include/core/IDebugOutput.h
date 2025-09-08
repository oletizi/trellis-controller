#ifndef IDEBUG_OUTPUT_H
#define IDEBUG_OUTPUT_H

#include <string>

class IDebugOutput {
public:
    virtual ~IDebugOutput() = default;
    
    virtual void log(const std::string& message) = 0;
    virtual void logf(const char* format, ...) = 0;
};

#endif