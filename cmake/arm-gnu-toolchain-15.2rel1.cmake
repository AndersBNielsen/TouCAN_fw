# Arm GNU Toolchain 15.2rel1.
#
# This file supports both:
# - Arm installer under /Applications/ArmGNUToolchain/...
# - Homebrew-installed toolchains (typically under /opt/homebrew/bin)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Allow override for nonstandard installs.
set(ARM_GNU_TOOLCHAIN_BIN_DIR "/Applications/ArmGNUToolchain/15.2.rel1/arm-none-eabi/bin" CACHE PATH "Arm GNU toolchain bin dir")

set(_ARM_GNU_TOOLCHAIN_SEARCH_DIRS
	"${ARM_GNU_TOOLCHAIN_BIN_DIR}"
	"/opt/homebrew/bin"
	"/usr/local/bin"
)

find_program(CMAKE_C_COMPILER
	NAMES arm-none-eabi-gcc-15.2.1 arm-none-eabi-gcc-15 arm-none-eabi-gcc
	HINTS ${_ARM_GNU_TOOLCHAIN_SEARCH_DIRS}
	REQUIRED
)
set(CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}" CACHE FILEPATH "" FORCE)

# LTO-enabled builds need the gcc wrappers here, otherwise ar/ranlib can't
# properly handle LTO objects and symbols may go missing at link time.
find_program(CMAKE_AR arm-none-eabi-gcc-ar HINTS ${_ARM_GNU_TOOLCHAIN_SEARCH_DIRS} REQUIRED)
find_program(CMAKE_RANLIB arm-none-eabi-gcc-ranlib HINTS ${_ARM_GNU_TOOLCHAIN_SEARCH_DIRS} REQUIRED)
find_program(CMAKE_OBJCOPY arm-none-eabi-objcopy HINTS ${_ARM_GNU_TOOLCHAIN_SEARCH_DIRS} REQUIRED)

# Avoid CMake trying to execute target binaries during configure.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
