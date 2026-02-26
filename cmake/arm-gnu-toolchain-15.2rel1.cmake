# Arm GNU Toolchain (Homebrew cask: gcc-arm-embedded)
# Uses the pkg-installed toolchain under /Applications.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Allow override for nonstandard installs.
set(ARM_GNU_TOOLCHAIN_BIN_DIR "/Applications/ArmGNUToolchain/15.2.rel1/arm-none-eabi/bin" CACHE PATH "Arm GNU toolchain bin dir")

find_program(CMAKE_C_COMPILER arm-none-eabi-gcc-15.2.1 HINTS "${ARM_GNU_TOOLCHAIN_BIN_DIR}" REQUIRED)
set(CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}" CACHE FILEPATH "" FORCE)

# LTO-enabled builds need the gcc wrappers here, otherwise ar/ranlib can't
# properly handle LTO objects and symbols may go missing at link time.
find_program(CMAKE_AR arm-none-eabi-gcc-ar HINTS "${ARM_GNU_TOOLCHAIN_BIN_DIR}" REQUIRED)
find_program(CMAKE_RANLIB arm-none-eabi-gcc-ranlib HINTS "${ARM_GNU_TOOLCHAIN_BIN_DIR}" REQUIRED)
find_program(CMAKE_OBJCOPY arm-none-eabi-objcopy HINTS "${ARM_GNU_TOOLCHAIN_BIN_DIR}" REQUIRED)

# Avoid CMake trying to execute target binaries during configure.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
