#include "ConsoleDebugOutput.h"
#include "CursesDisplay.h"
#include <cstdarg>
#include <cstdio>

ConsoleDebugOutput::ConsoleDebugOutput(CursesDisplay* display) 
    : display_(display) {
}

void ConsoleDebugOutput::log(const std::string& message) {
    if (display_) {
        display_->addConsoleMessage(message);
    }
}

void ConsoleDebugOutput::logf(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    log(std::string(buffer));
}