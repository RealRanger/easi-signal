cmake_minimum_required(VERSION 3.15)

project(
    EasiSignal
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "A lightweight header-only C++ event system library"
    HOMEPAGE_URL "https://github.com/EasiSoft/EasiSignal"
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Header-only library
add_library(easi_signal INTERFACE)

# Add include directories
target_include_directories(
    easi_signal
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Alias for convenience
add_library(easi::signal ALIAS easi_signal)
add_library(easi::events ALIAS easi_signal)

# Installation rules
include(CMakePackageConfigHelpers)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    TARGETS easi_signal
    EXPORT easi_signalTargets
)

install(
    EXPORT easi_signalTargets
    FILE easi_signalTargets.cmake
    NAMESPACE easi::
    DESTINATION lib/cmake/easi_signal
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/easi_signalConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/easi_signalConfig.cmake"
    INSTALL_DESTINATION lib/cmake/easi_signal
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/easi_signalConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
    "${CMAKE_CURRENT_BINARY_DIR}/easi_signalConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/easi_signalConfigVersion.cmake"
    DESTINATION lib/cmake/easi_signal
)

# Optional: Enable examples
option(EASI_SIGNAL_BUILD_EXAMPLES "Build example code" OFF)

if(EASI_SIGNAL_BUILD_EXAMPLES)
    add_executable(easi_signal_example examples/main.cpp)
    target_link_libraries(easi_signal_example PRIVATE easi::signal)
endif()
