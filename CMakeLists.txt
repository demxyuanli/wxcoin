# wxCoin 3D Application - Modular CMake Build System
# Main CMakeLists.txt for modular project structure

cmake_minimum_required(VERSION 3.20)
project(FreeCADNavigation)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set Vcpkg toolchain file
set(CMAKE_TOOLCHAIN_FILE "D:/repos/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# Include module utilities
include(cmake/ModuleUtils.cmake)

# Find required dependencies
find_package(wxWidgets REQUIRED COMPONENTS core base gl aui propgrid)
find_package(Coin REQUIRED)

# Include wxWidgets
include(${wxWidgets_USE_FILE})

# Validate build configuration
validate_build_config()

# Build configuration options
option(ENABLE_HIGH_DPI "Enable high DPI support" ON)
option(ENABLE_DEBUG_LOGS "Enable debug logging" ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add module subdirectories in dependency order
message(STATUS "")
message(STATUS "=== Building Modules ===")

# Core module (no dependencies)
add_subdirectory(modules/core)

# Rendering module (depends on core)
add_subdirectory(modules/rendering)

# Geometry module (depends on core, rendering)
add_subdirectory(modules/geometry)

# Input module (depends on core, rendering)
add_subdirectory(modules/input)

# Navigation module (depends on core, rendering, input)
add_subdirectory(modules/navigation)

# Commands module (depends on core, geometry)
add_subdirectory(modules/commands)

# UI module (depends on all other modules)
add_subdirectory(modules/ui)

message(STATUS "=== Modules Built ===")
message(STATUS "")

# Verify all modules were built correctly
verify_module_dependencies()

# Generate dependency graph
generate_dependency_graph()

# Main executable
add_executable(${PROJECT_NAME} WIN32 main.cpp)

# Set target properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "wxcoin"
    DEBUG_POSTFIX "_d"
)

# Link all modules to the main executable
target_link_libraries(${PROJECT_NAME} PRIVATE
    wxcoin_ui                 # UI module includes all other dependencies
    ${wxWidgets_LIBRARIES}
    Coin::Coin
)

# Include directories for main executable
target_include_directories(${PROJECT_NAME} PRIVATE
    modules/core/include
    modules/rendering/include
    modules/geometry/include
    modules/input/include
    modules/navigation/include
    modules/commands/include
    modules/ui/include
    ${wxWidgets_INCLUDE_DIRS}
    ${Coin3D_INCLUDE_DIRS}
)

# Windows-specific settings
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)
endif()

# MSVC-specific settings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:/ZI /Od>)
    target_compile_options(${PROJECT_NAME} PRIVATE /std:c++17)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    )
endif()

# Build summary
message(STATUS "")
message(STATUS "=== Build Configuration Summary ===")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "High DPI Support: ${ENABLE_HIGH_DPI}")
message(STATUS "Debug Logs: ${ENABLE_DEBUG_LOGS}")
message(STATUS "Output Directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "=====================================")
message(STATUS "") 