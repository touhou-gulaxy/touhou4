cmake_minimum_required(VERSION 3.16)
project(scrnn_upscale LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

find_package(PkgConfig REQUIRED)
pkg_check_modules(EGL REQUIRED IMPORTED_TARGET egl)
pkg_check_modules(EPOXY REQUIRED IMPORTED_TARGET epoxy)
find_package(PNG REQUIRED)

add_executable(scrnn-upscale src/main.cpp)
target_compile_definitions(
    scrnn-upscale
    PRIVATE SCRNN_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)
target_compile_options(scrnn-upscale PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(
    scrnn-upscale
    PRIVATE
    PkgConfig::EGL
    PkgConfig::EPOXY
    PNG::PNG
)
