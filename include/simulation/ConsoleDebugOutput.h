#ifndef CONSOLE_DEBUG_OUTPUT_H
#define CONSOLE_DEBUG_OUTPUT_H

#include "IDebugOutput.h"

class CursesDisplay;

class ConsoleDebugOutput : public IDebugOutput {
public:
    explicit ConsoleDebugOutput(CursesDisplay* display);
    
    void log(const std::string& message) override;
    void logf(const char* format, ...) override;
    
private:
    CursesDisplay* display_;
};

#endif