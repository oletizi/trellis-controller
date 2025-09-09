#ifndef TEST_STDINT_COMPAT_H
#define TEST_STDINT_COMPAT_H

/**
 * @file stdint_compat.h
 * @brief Compatibility header for tests to prevent conflicts with custom embedded stdint.h
 * 
 * This header ensures tests use system stdint types instead of the project's
 * custom embedded stdint.h which is designed for 32-bit ARM and conflicts 
 * with 64-bit host system types.
 */

// For test builds, always use the system stdint
#include <cstdint>

// Ensure we have the standard namespace types available
using std::int8_t;
using std::uint8_t;
using std::int16_t;
using std::uint16_t;
using std::int32_t;
using std::uint32_t;
using std::int64_t;
using std::uint64_t;
using std::intptr_t;
using std::uintptr_t;

#endif // TEST_STDINT_COMPAT_H