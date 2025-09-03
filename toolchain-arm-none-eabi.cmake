# Toolchain file for ARM Cortex-M cross-compilation
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=toolchain-arm-none-eabi.cmake ..

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Find the ARM toolchain
find_program(ARM_GCC_COMPILER arm-none-eabi-gcc
    PATHS 
    /opt/homebrew/bin
    /usr/local/bin
    /usr/bin
)

if(NOT ARM_GCC_COMPILER)
    message(FATAL_ERROR "arm-none-eabi-gcc not found! Please install the ARM embedded toolchain.")
endif()

# Get the toolchain path
get_filename_component(ARM_TOOLCHAIN_DIR ${ARM_GCC_COMPILER} DIRECTORY)

# Set the compilers
set(CMAKE_C_COMPILER ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-gcc)
set(CMAKE_AR ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-ar)
set(CMAKE_OBJCOPY ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-objdump)
set(SIZE ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-size)

# Set compiler flags for Cortex-M4 (bare metal)
set(CMAKE_C_FLAGS_INIT "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -ffunction-sections -fdata-sections -Wall -ffreestanding -nostdlib")
set(CMAKE_CXX_FLAGS_INIT "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-threadsafe-statics -Wall -ffreestanding -nostdlib")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wl,--gc-sections -nostdlib -nostartfiles")

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# For libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)