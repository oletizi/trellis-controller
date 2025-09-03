// Minimal C++ runtime stubs for embedded environment
// These functions are required by the C++ compiler but not available in bare metal

#include <stddef.h>
#include <stdint.h>

extern "C" {

// Minimal memory allocation stubs - should never be called in embedded code
void* operator new(size_t size) {
    // In embedded systems, we should never use dynamic allocation
    // This stub prevents linking errors but will halt if called
    (void)size;
    while(1) {
        // Infinite loop to indicate programming error
    }
    return nullptr;
}

void* operator new[](size_t size) {
    (void)size;
    while(1) {
        // Infinite loop to indicate programming error
    }
    return nullptr;
}

void operator delete(void* ptr) {
    // No-op in embedded system - memory is statically allocated
    (void)ptr;
}

void operator delete[](void* ptr) {
    // No-op in embedded system - memory is statically allocated
    (void)ptr;
}

void operator delete(void* ptr, size_t size) {
    // No-op in embedded system - memory is statically allocated
    (void)ptr;
    (void)size;
}

void operator delete[](void* ptr, size_t size) {
    // No-op in embedded system - memory is statically allocated
    (void)ptr;
    (void)size;
}

// C++ runtime support functions
int atexit(void (*func)(void)) {
    // In embedded systems, we don't need atexit functionality
    // Global destructors should be avoided
    (void)func;
    return 0; // Success
}

// Pure virtual function call handler
void __cxa_pure_virtual() {
    // This should never be called if interfaces are properly implemented
    while(1) {
        // Infinite loop to indicate programming error
    }
}

// Exception handling stubs (should never be used with -fno-exceptions)
void __cxa_throw_bad_array_new_length() {
    while(1) {}
}

void _Unwind_Resume() {
    while(1) {}
}

// Global constructor/destructor support
typedef void (*func_ptr)(void);

// These are provided by the linker script and startup code
extern func_ptr __init_array_start[];
extern func_ptr __init_array_end[];

void __libc_init_array() {
    // Call global constructors
    for (func_ptr* func = __init_array_start; func != __init_array_end; func++) {
        if (*func) {
            (*func)();
        }
    }
}

} // extern "C"