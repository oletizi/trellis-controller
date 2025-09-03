#include "DebugSerial.h"

#ifdef __SAMD51J19A__

// No C++ standard library in embedded

// Static member initialization
bool DebugSerial::initialized_ = false;
uint32_t DebugSerial::startTime_ = 0;
char DebugSerial::buffer_[256];

// External system timer functions
extern volatile uint32_t g_tickCount;

void DebugSerial::init() {
    if (initialized_) return;
    
    // TODO: Initialize USB CDC for serial communication
    // For now, this is a stub implementation
    // Real implementation would:
    // 1. Configure USB peripheral
    // 2. Set up CDC interface
    // 3. Enable USB interrupts
    
    startTime_ = g_tickCount;
    initialized_ = true;
    
    // Send startup message
    println("=== Trellis Controller Debug Started ===");
    printTimestamp("INIT");
    printSystemStatus();
}

void DebugSerial::print(const char* str) {
    if (!initialized_ || !str) return;
    writeString(str);
}

void DebugSerial::println(const char* str) {
    if (!initialized_) return;
    if (str) writeString(str);
    writeString("\r\n");
}

void DebugSerial::printf(const char* format, ...) {
    if (!initialized_ || !format) return;
    
    va_list args;
    va_start(args, format);
    
    // Simple printf implementation for embedded
    // Supports %s, %d, %x, %c, %%
    char* out = buffer_;
    const char* in = format;
    
    while (*in && (out - buffer_) < (sizeof(buffer_) - 1)) {
        if (*in == '%' && *(in + 1)) {
            in++; // Skip %
            switch (*in) {
                case 's': {
                    const char* arg = va_arg(args, const char*);
                    if (arg) {
                        while (*arg && (out - buffer_) < (sizeof(buffer_) - 1)) {
                            *out++ = *arg++;
                        }
                    }
                    break;
                }
                case 'd': {
                    int arg = va_arg(args, int);
                    // Simple integer to string conversion
                    if (arg == 0) {
                        *out++ = '0';
                    } else {
                        if (arg < 0) {
                            *out++ = '-';
                            arg = -arg;
                        }
                        char temp[12]; // Max digits for 32-bit int
                        int pos = 0;
                        while (arg > 0) {
                            temp[pos++] = '0' + (arg % 10);
                            arg /= 10;
                        }
                        while (pos > 0 && (out - buffer_) < (sizeof(buffer_) - 1)) {
                            *out++ = temp[--pos];
                        }
                    }
                    break;
                }
                case 'x': {
                    uint32_t arg = va_arg(args, uint32_t);
                    const char* hex = "0123456789ABCDEF";
                    for (int i = 7; i >= 0; i--) {
                        if ((out - buffer_) < (sizeof(buffer_) - 1)) {
                            *out++ = hex[(arg >> (i * 4)) & 0xF];
                        }
                    }
                    break;
                }
                case 'c': {
                    char arg = va_arg(args, int);
                    *out++ = arg;
                    break;
                }
                case '%': {
                    *out++ = '%';
                    break;
                }
                default:
                    *out++ = '%';
                    *out++ = *in;
                    break;
            }
        } else {
            *out++ = *in;
        }
        in++;
    }
    
    *out = '\0';
    va_end(args);
    
    writeString(buffer_);
}

void DebugSerial::printHex(uint32_t value, uint8_t digits) {
    if (!initialized_) return;
    
    const char* hex = "0123456789ABCDEF";
    char temp[9]; // Max 8 digits + null terminator
    
    digits = (digits > 8) ? 8 : digits;
    temp[digits] = '\0';
    
    for (int i = digits - 1; i >= 0; i--) {
        temp[i] = hex[value & 0xF];
        value >>= 4;
    }
    
    writeString("0x");
    writeString(temp);
}

void DebugSerial::printBinary(uint32_t value, uint8_t bits) {
    if (!initialized_) return;
    
    char temp[33]; // Max 32 bits + null terminator
    bits = (bits > 32) ? 32 : bits;
    temp[bits] = '\0';
    
    for (int i = bits - 1; i >= 0; i--) {
        temp[i] = (value & 1) ? '1' : '0';
        value >>= 1;
    }
    
    writeString("0b");
    writeString(temp);
}

void DebugSerial::printTimestamp(const char* label) {
    if (!initialized_) return;
    
    uint32_t elapsed = g_tickCount - startTime_;
    printf("[%d ms]", elapsed);
    
    if (label) {
        printf(" %s", label);
    }
    println();
}

void DebugSerial::printMemoryUsage() {
    if (!initialized_) return;
    
    // TODO: Implement actual memory usage monitoring
    // This would require:
    // 1. Stack pointer monitoring
    // 2. Heap usage tracking (should be zero)
    // 3. Static memory calculation
    
    println("=== Memory Usage ===");
    printf("Stack: ~%d bytes used\n", 1024); // Placeholder
    printf("Heap:  %d bytes (should be 0)\n", 0);
    printf("Static: ~%d bytes\n", 8600); // From linker output
    println("===================");
}

void DebugSerial::printSystemStatus() {
    if (!initialized_) return;
    
    println("=== System Status ===");
    printf("Uptime: %d ms\n", g_tickCount);
    printf("System Clock: 48 MHz\n");
    printf("Firmware: Trellis Step Sequencer v1.0\n");
    printf("Build: %s %s\n", __DATE__, __TIME__);
    println("====================");
}

void DebugSerial::writeChar(char c) {
    // TODO: Implement actual USB CDC character output
    // For now, this is a stub
    // Real implementation would write to USB CDC buffer
    (void)c; // Suppress unused parameter warning
}

void DebugSerial::writeString(const char* str) {
    if (!str) return;
    while (*str) {
        writeChar(*str++);
    }
}

#endif // __SAMD51J19A__