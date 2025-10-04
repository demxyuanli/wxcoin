# Unified Parameter Management System
# CMakeLists.txt for the unified parameter management system

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Existing parameter management sources
set(EXISTING_PARAM_SOURCES
    MeshParameterManager.cpp
    MeshParameterManagerSimple.cpp
    MeshParameterValidator.cpp
    NormalValidator.cpp
)

# New unified parameter management sources
set(UNIFIED_PARAM_SOURCES
    UnifiedParameterTree.cpp
    ParameterRegistry.cpp
    UpdateCoordinator.cpp
    UnifiedParameterIntegration.cpp
    ParameterSystemIntegration.cpp
    UnifiedParameterExample.cpp
    ParameterSystemIntegrationExample.cpp
)

# All parameter management sources
set(ALL_PARAM_SOURCES
    ${EXISTING_PARAM_SOURCES}
    ${UNIFIED_PARAM_SOURCES}
)

# Header files
set(UNIFIED_PARAM_HEADERS
    ${CMAKE_SOURCE_DIR}/include/param/UnifiedParameterTree.h
    ${CMAKE_SOURCE_DIR}/include/param/ParameterRegistry.h
    ${CMAKE_SOURCE_DIR}/include/param/UpdateCoordinator.h
    ${CMAKE_SOURCE_DIR}/include/param/UnifiedParameterIntegration.h
    ${CMAKE_SOURCE_DIR}/include/param/ParameterSystemIntegration.h
)

# Existing parameter management headers
set(EXISTING_PARAM_HEADERS
    ${CMAKE_SOURCE_DIR}/include/MeshParameterManager.h
    ${CMAKE_SOURCE_DIR}/include/MeshParameterManagerSimple.h
    ${CMAKE_SOURCE_DIR}/include/MeshParameterValidator.h
    ${CMAKE_SOURCE_DIR}/include/NormalValidator.h
)

# All parameter management headers
set(ALL_PARAM_HEADERS
    ${EXISTING_PARAM_HEADERS}
    ${UNIFIED_PARAM_HEADERS}
)

# Create unified parameter management library
add_library(UnifiedParameterSystem STATIC ${ALL_PARAM_SOURCES} ${ALL_PARAM_HEADERS})

# Create legacy parameter management library (for backward compatibility)
add_library(LegacyParameterSystem STATIC ${EXISTING_PARAM_SOURCES} ${EXISTING_PARAM_HEADERS})

# Link necessary libraries
target_link_libraries(UnifiedParameterSystem
    ${OpenCASCADE_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${Inventor_LIBRARIES}
)

target_link_libraries(LegacyParameterSystem
    ${OpenCASCADE_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${Inventor_LIBRARIES}
)

# Set compile definitions
target_compile_definitions(UnifiedParameterSystem PRIVATE
    -DUNIFIED_PARAMETER_SYSTEM_VERSION="1.0.0"
    -DUNIFIED_PARAMETER_SYSTEM_BUILD_DATE="${CMAKE_BUILD_DATE}"
    -DUNIFIED_PARAMETER_SYSTEM_ENABLED=1
)

target_compile_definitions(LegacyParameterSystem PRIVATE
    -DLEGACY_PARAMETER_SYSTEM_VERSION="1.0.0"
    -DLEGACY_PARAMETER_SYSTEM_ENABLED=1
)

# Set include directories
target_include_directories(UnifiedParameterSystem PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${OpenCASCADE_INCLUDE_DIRS}
    ${wxWidgets_INCLUDE_DIRS}
    ${Inventor_INCLUDE_DIRS}
)

target_include_directories(LegacyParameterSystem PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${OpenCASCADE_INCLUDE_DIRS}
    ${wxWidgets_INCLUDE_DIRS}
    ${Inventor_INCLUDE_DIRS}
)

# Create example executables
add_executable(UnifiedParameterExample UnifiedParameterExample.cpp)
target_link_libraries(UnifiedParameterExample
    UnifiedParameterSystem
    LegacyParameterSystem
    ${OpenCASCADE_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${Inventor_LIBRARIES}
)

add_executable(ParameterSystemIntegrationExample ParameterSystemIntegrationExample.cpp)
target_link_libraries(ParameterSystemIntegrationExample
    UnifiedParameterSystem
    LegacyParameterSystem
    ${OpenCASCADE_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${Inventor_LIBRARIES}
)

# Set example include directories
target_include_directories(UnifiedParameterExample PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${OpenCASCADE_INCLUDE_DIRS}
    ${wxWidgets_INCLUDE_DIRS}
    ${Inventor_INCLUDE_DIRS}
)

target_include_directories(ParameterSystemIntegrationExample PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${OpenCASCADE_INCLUDE_DIRS}
    ${wxWidgets_INCLUDE_DIRS}
    ${Inventor_INCLUDE_DIRS}
)

# Installation rules
install(TARGETS UnifiedParameterSystem LegacyParameterSystem
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(TARGETS UnifiedParameterExample ParameterSystemIntegrationExample
    RUNTIME DESTINATION bin
)

install(FILES ${ALL_PARAM_HEADERS}
    DESTINATION include/param
)

# Test targets (optional)
if(BUILD_TESTS)
    enable_testing()
    
    # Create test executables
    add_executable(UnifiedParameterTest
        UnifiedParameterTest.cpp
    )
    
    target_link_libraries(UnifiedParameterTest
        UnifiedParameterSystem
        LegacyParameterSystem
        ${OpenCASCADE_LIBRARIES}
        ${wxWidgets_LIBRARIES}
        ${Inventor_LIBRARIES}
    )
    
    # Add tests
    add_test(NAME UnifiedParameterSystemTest
        COMMAND UnifiedParameterTest
    )
    
    add_test(NAME LegacyParameterSystemTest
        COMMAND UnifiedParameterTest --legacy
    )
endif()

# Print build information
message(STATUS "Parameter Management System:")
message(STATUS "  - Unified System Version: 1.0.0")
message(STATUS "  - Legacy System Version: 1.0.0")
message(STATUS "  - Unified Sources: ${UNIFIED_PARAM_SOURCES}")
message(STATUS "  - Legacy Sources: ${EXISTING_PARAM_SOURCES}")
message(STATUS "  - All Headers: ${ALL_PARAM_HEADERS}")
message(STATUS "  - Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  - C++ standard: ${CMAKE_CXX_STANDARD}")

# Set output directories
set_target_properties(UnifiedParameterSystem PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

set_target_properties(LegacyParameterSystem PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

set_target_properties(UnifiedParameterExample PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

set_target_properties(ParameterSystemIntegrationExample PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Add compile options
if(MSVC)
    target_compile_options(UnifiedParameterSystem PRIVATE /W4)
    target_compile_options(LegacyParameterSystem PRIVATE /W4)
    target_compile_options(UnifiedParameterExample PRIVATE /W4)
    target_compile_options(ParameterSystemIntegrationExample PRIVATE /W4)
else()
    target_compile_options(UnifiedParameterSystem PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(LegacyParameterSystem PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(UnifiedParameterExample PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(ParameterSystemIntegrationExample PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Debug information
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(UnifiedParameterSystem PRIVATE DEBUG_UNIFIED_PARAMETER_SYSTEM)
    target_compile_definitions(LegacyParameterSystem PRIVATE DEBUG_LEGACY_PARAMETER_SYSTEM)
    message(STATUS "  - Debug mode enabled")
endif()

# Performance optimization
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(UnifiedParameterSystem PRIVATE RELEASE_UNIFIED_PARAMETER_SYSTEM)
    target_compile_definitions(LegacyParameterSystem PRIVATE RELEASE_LEGACY_PARAMETER_SYSTEM)
    message(STATUS "  - Release mode enabled")
endif()

# Feature flags
option(ENABLE_UNIFIED_PARAMETER_SYSTEM "Enable unified parameter management system" ON)
option(ENABLE_LEGACY_PARAMETER_SYSTEM "Enable legacy parameter management system" ON)
option(ENABLE_PARAMETER_SYSTEM_INTEGRATION "Enable integration between unified and legacy systems" ON)

if(ENABLE_UNIFIED_PARAMETER_SYSTEM)
    target_compile_definitions(UnifiedParameterSystem PRIVATE UNIFIED_PARAMETER_SYSTEM_ENABLED=1)
    message(STATUS "  - Unified parameter system enabled")
endif()

if(ENABLE_LEGACY_PARAMETER_SYSTEM)
    target_compile_definitions(LegacyParameterSystem PRIVATE LEGACY_PARAMETER_SYSTEM_ENABLED=1)
    message(STATUS "  - Legacy parameter system enabled")
endif()

if(ENABLE_PARAMETER_SYSTEM_INTEGRATION)
    target_compile_definitions(UnifiedParameterSystem PRIVATE PARAMETER_SYSTEM_INTEGRATION_ENABLED=1)
    target_compile_definitions(LegacyParameterSystem PRIVATE PARAMETER_SYSTEM_INTEGRATION_ENABLED=1)
    message(STATUS "  - Parameter system integration enabled")
endif()