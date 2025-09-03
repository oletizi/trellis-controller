cmake_minimum_required(VERSION 3.20)

# Project configuration
project(TrellisStepSequencer 
    VERSION 1.0.0
    LANGUAGES C CXX ASM
)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# Check if we're cross-compiling for ARM
if(NOT CMAKE_CROSSCOMPILING)
    message(FATAL_ERROR "This project requires ARM cross-compilation. Please use: cmake -DCMAKE_TOOLCHAIN_FILE=toolchain-arm-none-eabi.cmake ..")
endif()

# MCU specific flags are now set in the toolchain file
# Additional project-specific flags can be added here
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T${CMAKE_SOURCE_DIR}/samd51.ld")

# Define SAMD51 specific settings
add_definitions(
    -D__SAMD51J19A__
    -DARM_MATH_CM4
    -DF_CPU=120000000L
    -DARDUINO_ARCH_SAMD
)

# Include directories (include our custom headers first)
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib
)

# Source files
file(GLOB_RECURSE SOURCES 
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.c
)

# Create executable
add_executable(${PROJECT_NAME}.elf ${SOURCES})

# Find tools from toolchain
find_program(ARM_SIZE arm-none-eabi-size)
find_program(ARM_OBJCOPY arm-none-eabi-objcopy)

# Generate hex and bin files
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${ARM_OBJCOPY} -O ihex ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
    COMMAND ${ARM_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMAND ${ARM_SIZE} ${PROJECT_NAME}.elf
    COMMENT "Building ${PROJECT_NAME}.hex and ${PROJECT_NAME}.bin"
)

# Flash target (using bossac for SAMD51)
add_custom_target(flash
    COMMAND bossac -e -w -v -R --offset=0x4000 ${PROJECT_NAME}.bin
    DEPENDS ${PROJECT_NAME}.elf
    COMMENT "Flashing ${PROJECT_NAME}.bin to NeoTrellis M4"
)