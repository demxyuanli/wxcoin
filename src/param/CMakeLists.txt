# Parameter Management System CMakeLists.txt

# Set minimum CMake version
cmake_minimum_required(VERSION 3.16)

# Find required packages
find_package(OpenCASCADE REQUIRED)
find_package(jsoncpp REQUIRED)
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${OpenCASCADE_INCLUDE_DIRS})

# Source files
set(PARAM_SOURCES
    ParameterTree.cpp
    ParameterTreeBuilder.cpp
    ParameterUpdateManager.cpp
    ParameterSynchronizer.cpp
    UnifiedParameterManager.cpp
)

# Header files
set(PARAM_HEADERS
    ${CMAKE_SOURCE_DIR}/include/param/ParameterTree.h
    ${CMAKE_SOURCE_DIR}/include/param/ParameterUpdateManager.h
    ${CMAKE_SOURCE_DIR}/include/param/ParameterSynchronizer.h
    ${CMAKE_SOURCE_DIR}/include/param/UnifiedParameterManager.h
)

# Create parameter management library
add_library(param_management STATIC ${PARAM_SOURCES} ${PARAM_HEADERS})

# Link libraries
target_link_libraries(param_management
    ${OpenCASCADE_LIBRARIES}
    jsoncpp_lib
    Threads::Threads
)

# Set compile definitions
target_compile_definitions(param_management PRIVATE
    PARAM_MANAGEMENT_EXPORTS
)

# Set C++ standard
set_target_properties(param_management PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Include directories for the library
target_include_directories(param_management PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${OpenCASCADE_INCLUDE_DIRS}
)

# Compiler-specific options
if(MSVC)
    target_compile_options(param_management PRIVATE /W4)
else()
    target_compile_options(param_management PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Installation rules
install(TARGETS param_management
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${PARAM_HEADERS}
    DESTINATION include/param
)