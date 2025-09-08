cmake_minimum_required(VERSION 3.20)

# Project configuration
project(TrellisController 
    VERSION 1.0.0
    LANGUAGES C CXX ASM
)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# Build options (mutually exclusive)
option(BUILD_SIMULATION "Build simulation for host platform instead of embedded target" OFF)
option(BUILD_TESTS "Build unit tests for host platform" OFF)

# Core business logic sources (platform-agnostic)
set(CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/core/StepSequencer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/ShiftControls.cpp
    ${CMAKE_SOURCE_DIR}/src/core/ParameterLockPool.cpp
    ${CMAKE_SOURCE_DIR}/src/core/SequencerStateManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/ParameterEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/core/ControlGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AdaptiveButtonTracker.cpp
    ${CMAKE_SOURCE_DIR}/src/core/ControlMessage.cpp
    ${CMAKE_SOURCE_DIR}/src/core/JsonState.cpp
    ${CMAKE_SOURCE_DIR}/src/core/JsonUtils.cpp
)

if(BUILD_TESTS)
    # ==================== TEST BUILD ====================
    message(STATUS "Building unit tests for host platform")
    
    # Enable testing
    enable_testing()
    
    # Compiler flags for coverage and debugging
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g --coverage")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g --coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    
    # Include directories
    include_directories(
        ${CMAKE_SOURCE_DIR}/include/core
        ${CMAKE_SOURCE_DIR}/test
    )
    
    # Test sources (exclude the parameter lock test harness for general tests)
    file(GLOB_RECURSE ALL_TEST_SOURCES 
        ${CMAKE_SOURCE_DIR}/test/*.cpp
    )
    
    # Separate parameter lock test from other tests
    list(REMOVE_ITEM ALL_TEST_SOURCES "${CMAKE_SOURCE_DIR}/test/test_parameter_locks.cpp")
    set(TEST_SOURCES ${ALL_TEST_SOURCES})
    
    # Find Catch2
    find_package(Catch2 3 REQUIRED)
    
    # Create main test executable (existing tests)
    add_executable(run_tests ${TEST_SOURCES} ${CORE_SOURCES})
    target_link_libraries(run_tests PRIVATE Catch2::Catch2WithMain)
    add_test(NAME UnitTests COMMAND run_tests)
    
    # Create dedicated Parameter Lock test executable
    add_executable(test_parameter_locks 
        ${CMAKE_SOURCE_DIR}/test/test_parameter_locks.cpp 
        ${CORE_SOURCES}
        ${CMAKE_SOURCE_DIR}/src/core/NonRealtimeSequencer.cpp
    )
    target_link_libraries(test_parameter_locks PRIVATE Catch2::Catch2)
    add_test(NAME ParameterLockTests COMMAND test_parameter_locks)
    
    # Add standalone target for parameter lock tests (runs without Catch2)
    add_custom_target(test-parameter-locks-standalone
        COMMAND test_parameter_locks --standalone
        DEPENDS test_parameter_locks
        COMMENT "Running parameter lock tests in standalone mode"
    )
    
    # Coverage targets
    add_custom_target(coverage
        COMMAND lcov --directory . --capture --output-file coverage.info
        COMMAND lcov --remove coverage.info '/usr/*' '${CMAKE_SOURCE_DIR}/test/*' --output-file coverage.info
        COMMAND genhtml coverage.info --output-directory coverage_html
        DEPENDS run_tests test_parameter_locks
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating coverage report"
    )
    
    add_custom_target(coverage-clean
        COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -delete
        COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcno" -delete
        COMMAND rm -rf ${CMAKE_BINARY_DIR}/coverage.info ${CMAKE_BINARY_DIR}/coverage_html
        COMMENT "Cleaning coverage data"
    )

elseif(BUILD_SIMULATION)
    # ==================== SIMULATION BUILD ====================
    message(STATUS "Building simulation for host platform")
    
    # Compiler flags for debugging
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")
    
    # Find ncurses
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(NCURSES REQUIRED ncurses)
    
    # Include directories for simulation
    include_directories(
        ${CMAKE_SOURCE_DIR}/include/core
        ${CMAKE_SOURCE_DIR}/include/simulation
    )
    
    # Simulation sources (includes testing-only files)
    set(SIMULATION_SOURCES
        ${CMAKE_SOURCE_DIR}/src/simulation/CursesDisplay.cpp
        ${CMAKE_SOURCE_DIR}/src/simulation/CursesInput.cpp
        ${CMAKE_SOURCE_DIR}/src/simulation/ConsoleDebugOutput.cpp
        ${CMAKE_SOURCE_DIR}/src/simulation/main.cpp
        ${CMAKE_SOURCE_DIR}/src/core/NonRealtimeSequencer.cpp
    )
    
    # Create simulation executable
    add_executable(trellis_simulation ${SIMULATION_SOURCES} ${CORE_SOURCES})
    
    # Link with ncurses
    target_link_libraries(trellis_simulation ${NCURSES_LIBRARIES})
    target_include_directories(trellis_simulation PRIVATE ${NCURSES_INCLUDE_DIRS})
    target_compile_options(trellis_simulation PRIVATE ${NCURSES_CFLAGS_OTHER})
    
    # Install target
    install(TARGETS trellis_simulation DESTINATION bin)
    
else()
    # ==================== EMBEDDED BUILD ====================
    message(STATUS "Building for embedded ARM target (SAMD51)")
    
    # Check if we're cross-compiling for ARM
    if(NOT CMAKE_CROSSCOMPILING)
        message(FATAL_ERROR "Embedded build requires ARM cross-compilation. Please use: cmake -DCMAKE_TOOLCHAIN_FILE=toolchain-arm-none-eabi.cmake ..")
    endif()
    
    # MCU specific flags are now set in the toolchain file
    # Additional project-specific flags for embedded C++
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti -fno-use-cxa-atexit")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-threadsafe-statics -fno-stack-protector")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T${CMAKE_SOURCE_DIR}/samd51.ld")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections -Wl,--print-memory-usage")
    
    # Define SAMD51 specific settings
    add_definitions(
        -D__SAMD51J19A__
        -DARM_MATH_CM4
        -DF_CPU=120000000L
        -DARDUINO_ARCH_SAMD
    )
    
    # Include directories (include core and embedded headers first)
    include_directories(
        ${CMAKE_SOURCE_DIR}/include/core
        ${CMAKE_SOURCE_DIR}/include/embedded
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/lib
    )
    
    # Embedded platform sources
    set(EMBEDDED_SOURCES
        ${CMAKE_SOURCE_DIR}/src/embedded/main.cpp
        ${CMAKE_SOURCE_DIR}/src/embedded/NeoTrellisDisplay.cpp
        ${CMAKE_SOURCE_DIR}/src/embedded/NeoTrellisInput.cpp
        ${CMAKE_SOURCE_DIR}/src/embedded/SeesawI2C.cpp
        ${CMAKE_SOURCE_DIR}/src/embedded/SAMD51_I2C.cpp
        ${CMAKE_SOURCE_DIR}/src/embedded/DebugSerial.cpp
        ${CMAKE_SOURCE_DIR}/src/embedded/runtime_stubs.cpp
    )
    
    # Legacy embedded sources (keep existing hardware drivers)
    file(GLOB LEGACY_SOURCES 
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/src/*.c
    )
    # Add system_init.c explicitly
    list(APPEND LEGACY_SOURCES ${CMAKE_SOURCE_DIR}/src/system_init.c)
    # Remove old main.cpp to avoid conflicts
    list(FILTER LEGACY_SOURCES EXCLUDE REGEX ".*/main.cpp$")
    
    # All sources for embedded build
    set(SOURCES ${CORE_SOURCES} ${EMBEDDED_SOURCES} ${LEGACY_SOURCES})
    
    # Create executable
    add_executable(TrellisStepSequencer.elf ${SOURCES})
    
    # Find tools from toolchain
    find_program(ARM_SIZE arm-none-eabi-size)
    find_program(ARM_OBJCOPY arm-none-eabi-objcopy)
    
    # Generate hex and bin files
    add_custom_command(TARGET TrellisStepSequencer.elf POST_BUILD
        COMMAND ${ARM_OBJCOPY} -O ihex TrellisStepSequencer.elf TrellisStepSequencer.hex
        COMMAND ${ARM_OBJCOPY} -O binary TrellisStepSequencer.elf TrellisStepSequencer.bin
        COMMAND ${ARM_SIZE} TrellisStepSequencer.elf
        COMMENT "Building TrellisStepSequencer.hex and TrellisStepSequencer.bin"
    )
    
    # Flash target (using bossac for SAMD51)
    add_custom_target(flash
        COMMAND bossac -e -w -v -R --offset=0x4000 TrellisStepSequencer.bin
        DEPENDS TrellisStepSequencer.elf
        COMMENT "Flashing TrellisStepSequencer.bin to NeoTrellis M4"
    )
endif()