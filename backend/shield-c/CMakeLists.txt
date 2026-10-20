# Interlock Assessment IDE - Kernel Shield Build Configuration
cmake_minimum_required(VERSION 3.20)
project(InterlockShield)

# 1. Kernel-specific Compilation Guards
# These flags are CRITICAL to prevent Kernel Panics due to unsupported C++ features.
set(KERNEL_FLAGS
    -fno-builtin
    -fno-exceptions
    -fno-rtti
    -fno-common
    -fno-stack-protector
    -mkernel
    -nostdinc++
    -DKERNEL
    -DAPPLE
    -DNeXT
)

add_library(InterlockShield OBJECT
    interlock_shield.cpp
    interlock_user_client.cpp
    interlock_log.cpp
)

target_compile_options(InterlockShield PRIVATE ${KERNEL_FLAGS})

# Ensure we don't accidentally link against the standard C library or C++ library
set_target_properties(InterlockShield PROPERTIES
    LINKER_LANGUAGE CXX
    LINK_FLAGS "-Xlinker -kext"
)

# 2. Safety Check: Verify no 'std::' usage
add_custom_command(TARGET InterlockShield POST_BUILD
    COMMAND grep -rv "std::" ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp || (echo "[!] ERROR: Standard Library usage detected in Kernel code! This WILL cause a Kernel Panic." && exit 1)
    COMMENT "Auditing for unsafe Standard Library usage..."
)
