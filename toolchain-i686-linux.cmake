# CMake toolchain file for cross-compiling to Linux i686
# using the Ubuntu apt package gcc-i686-linux-gnu
#
# Usage:
#   cmake -B build_linux -S . -DCMAKE_TOOLCHAIN_FILE=toolchain-i686-linux.cmake
#
set(CMAKE_SYSTEM_NAME      Linux)
set(CMAKE_SYSTEM_PROCESSOR i686)

# Cross-compilers installed via apt: gcc-i686-linux-gnu g++-i686-linux-gnu
set(CMAKE_C_COMPILER   i686-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER i686-linux-gnu-g++)

# Use the cross GCC as the preprocessor for .S assembly files so that
# C headers included inside them are properly preprocessed.
set(CMAKE_ASM_COMPILER i686-linux-gnu-gcc)

# During cross-compilation detection, only compile test programs,
# don't try to link them (linking requires target libs that may not
# be in the host's standard locations).
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Tell CMake to look for headers/libs in the cross-sysroot only,
# not in the host's /usr/include or /usr/lib.
set(CMAKE_FIND_ROOT_PATH        /usr/i686-linux-gnu)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
