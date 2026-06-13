cmake_minimum_required(VERSION 3.16)
project(dashboard_app C ASM)

set(CMAKE_C_STANDARD 99)

# Include UCRT64 toolchain paths if on Windows
if(WIN32)
    list(APPEND CMAKE_PREFIX_PATH "C:/msys64/ucrt64")
endif()

# Find SDL2 (Windows only)
if(WIN32)
    find_package(SDL2 REQUIRED)
endif()

# Setup LVGL
set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE STRING "LV_CONF_PATH")
add_subdirectory(lvgl)

# Include paths
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)
if(WIN32)
    include_directories(${SDL2_INCLUDE_DIRS})
endif()

# Sources
file(GLOB_RECURSE SRC_FILES "src/*.c")

# App target
add_executable(dashboard_app main.c ${SRC_FILES})

# Link libraries
if(WIN32)
    target_link_libraries(dashboard_app PRIVATE lvgl::lvgl SDL2::SDL2 winhttp)
endif()
if(UNIX)
    target_link_libraries(dashboard_app PRIVATE lvgl::lvgl m pthread rt)
endif()

# Copy SDL2.dll to build output (Windows only)
if(WIN32)
    add_custom_command(TARGET dashboard_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/msys64/ucrt64/bin/SDL2.dll"
        $<TARGET_FILE_DIR:dashboard_app>
    )
endif()
