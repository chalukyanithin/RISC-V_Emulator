cmake_minimum_required(VERSION 3.20)
project(RV64Emulator VERSION 1.0.0 LANGUAGES CXX)

# Only include the source files we have actually created
add_executable(rvemu
    src/main.cpp
    src/cpu/cpu.cpp
)

set_target_properties(rvemu PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_include_directories(rvemu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# MinGW/GCC compiler flags
target_compile_options(rvemu PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Release>:-O3;-march=native;-flto>
    $<$<CONFIG:Debug>:-O0;-g>
)

target_link_options(rvemu PRIVATE
    $<$<CONFIG:Release>:-flto>
)