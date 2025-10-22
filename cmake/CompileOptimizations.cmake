# CMake Compilation Optimization Settings
# This file contains settings to improve incremental build performance

# Enable ccache if available (Windows: sccache, Linux: ccache)
find_program(CCACHE_PROGRAM ccache)
find_program(SCCACHE_PROGRAM sccache)

if(SCCACHE_PROGRAM AND WIN32)
    message(STATUS "Found sccache: ${SCCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER ${SCCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE_PROGRAM})
    message(STATUS "Using sccache for compilation caching")
elseif(CCACHE_PROGRAM)
    message(STATUS "Found ccache: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    message(STATUS "Using ccache for compilation caching")
else()
    message(STATUS "No compilation cache tool found (consider installing sccache or ccache)")
endif()

# MSVC-specific optimizations
if(MSVC)
    # Use /MP for multi-processor compilation
    add_compile_options(/MP)
    message(STATUS "Enabled multi-processor compilation (/MP)")
    
    # Disable Edit and Continue to speed up compilation
    string(REPLACE "/ZI" "/Zi" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/ZI" "/Zi" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    
    # Disable minimal rebuild (it's slower for incremental builds)
    string(REPLACE "/Gm" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Gm" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    
    # Use /Zc:inline to remove unreferenced COMDAT functions
    add_compile_options(/Zc:inline)
    
    # Enable incremental linking in Debug
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /INCREMENTAL")
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /INCREMENTAL")
    
    # Disable full PDB in favor of faster /Zi
    add_compile_options($<$<CONFIG:Debug>:/Zi>)
    add_compile_options($<$<CONFIG:Release>:/Zi>)
    
    # Parallel linking
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /CGTHREADS:8")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /CGTHREADS:8")
    
    # Function-level linking for better incremental linking
    add_compile_options(/Gy)
    
    message(STATUS "Enabled MSVC incremental build optimizations")
endif()

# Set output directories to avoid conflicts
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Per-configuration output directories
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/${OUTPUTCONFIG})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/${OUTPUTCONFIG})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/${OUTPUTCONFIG})
endforeach()

# Reduce unnecessary file generation
set(CMAKE_EXPORT_COMPILE_COMMANDS OFF CACHE BOOL "Disable compile commands export for faster builds")

# Helper function to set optimal link dependencies
# Use PRIVATE for implementation dependencies, INTERFACE for header-only requirements
function(target_link_libraries_optimized TARGET_NAME)
    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs PUBLIC PRIVATE INTERFACE)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    if(ARG_PUBLIC)
        target_link_libraries(${TARGET_NAME} PUBLIC ${ARG_PUBLIC})
    endif()
    
    if(ARG_PRIVATE)
        target_link_libraries(${TARGET_NAME} PRIVATE ${ARG_PRIVATE})
    endif()
    
    if(ARG_INTERFACE)
        target_link_libraries(${TARGET_NAME} INTERFACE ${ARG_INTERFACE})
    endif()
endfunction()

message(STATUS "Compilation optimizations configured")


