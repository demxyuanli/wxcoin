# ModuleUtils.cmake - CMake utility functions for module management
# Provides common functions for building and linking modules

# Function to print module information
function(print_module_info MODULE_NAME)
    message(STATUS "")
    message(STATUS "=== Module: ${MODULE_NAME} ===")
    get_target_property(SOURCES ${MODULE_NAME} SOURCES)
    get_target_property(LINK_LIBS ${MODULE_NAME} LINK_LIBRARIES)
    get_target_property(INCLUDE_DIRS ${MODULE_NAME} INCLUDE_DIRECTORIES)
    
    message(STATUS "  Sources: ${SOURCES}")
    message(STATUS "  Link Libraries: ${LINK_LIBS}")
    message(STATUS "  Include Directories: ${INCLUDE_DIRS}")
    message(STATUS "========================")
    message(STATUS "")
endfunction()

# Function to add a module with common settings
function(add_wxcoin_module MODULE_NAME)
    cmake_parse_arguments(MODULE "" "" "SOURCES;HEADERS;DEPENDENCIES" ${ARGN})
    
    # Create the library
    add_library(${MODULE_NAME} STATIC ${MODULE_SOURCES} ${MODULE_HEADERS})
    
    # Set common properties
    set_target_properties(${MODULE_NAME} PROPERTIES
        OUTPUT_NAME "${MODULE_NAME}"
        DEBUG_POSTFIX "_d"
        FOLDER "Modules"
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )
    
    # Common include directories
    target_include_directories(${MODULE_NAME} PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${wxWidgets_INCLUDE_DIRS}
        ${Coin3D_INCLUDE_DIRS}
    )
    
    # Common link libraries
    target_link_libraries(${MODULE_NAME} PUBLIC
        ${wxWidgets_LIBRARIES}
        Coin::Coin
    )
    
    # Add module dependencies
    if(MODULE_DEPENDENCIES)
        target_link_libraries(${MODULE_NAME} PUBLIC ${MODULE_DEPENDENCIES})
    endif()
    
    # MSVC specific settings
    if(MSVC)
        target_compile_options(${MODULE_NAME} PRIVATE
            /W4                     # Warning level 4
            /permissive-            # Disable non-conforming code
            $<$<CONFIG:Debug>:/ZI>  # Edit and continue debug info
            $<$<CONFIG:Release>:/O2> # Optimize for speed
        )
    endif()
    
    # Add common compile definitions
    target_compile_definitions(${MODULE_NAME} PUBLIC
        $<$<BOOL:${WIN32}>:UNICODE _UNICODE>
        $<$<BOOL:${ENABLE_HIGH_DPI}>:ENABLE_HIGH_DPI>
        $<$<BOOL:${ENABLE_DEBUG_LOGS}>:ENABLE_DEBUG_LOGS>
    )
    
    message(STATUS "Added module: ${MODULE_NAME}")
endfunction()

# Function to verify module dependencies
function(verify_module_dependencies)
    message(STATUS "Verifying module dependencies...")
    
    # Check if all required modules exist
    set(REQUIRED_MODULES 
        wxcoin_core 
        wxcoin_rendering 
        wxcoin_geometry 
        wxcoin_input 
        wxcoin_navigation 
        wxcoin_commands 
        wxcoin_ui
    )
    
    foreach(MODULE ${REQUIRED_MODULES})
        if(TARGET ${MODULE})
            message(STATUS "  ✓ ${MODULE}")
        else()
            message(WARNING "  ✗ ${MODULE} - Not found!")
        endif()
    endforeach()
endfunction()

# Function to generate dependency graph
function(generate_dependency_graph)
    message(STATUS "Module Dependency Graph:")
    message(STATUS "")
    message(STATUS "  Core")
    message(STATUS "    └── Rendering (depends on Core)")
    message(STATUS "    └── Geometry (depends on Core, Rendering)")
    message(STATUS "    └── Input (depends on Core, Rendering)")
    message(STATUS "    └── Navigation (depends on Core, Rendering, Input)")
    message(STATUS "    └── Commands (depends on Core, Geometry)")
    message(STATUS "    └── UI (depends on all modules)")
    message(STATUS "")
endfunction()

# Function to validate build configuration
function(validate_build_config)
    message(STATUS "Validating build configuration...")
    
    # Check wxWidgets
    if(NOT wxWidgets_FOUND)
        message(FATAL_ERROR "wxWidgets not found!")
    endif()
    
    # Check Coin3D
    if(NOT TARGET Coin::Coin)
        message(FATAL_ERROR "Coin3D not found!")
    endif()
    
    # Check OpenGL for rendering module
    find_package(OpenGL QUIET)
    if(NOT OPENGL_FOUND)
        message(WARNING "OpenGL not found - rendering may be limited")
    endif()
    
    message(STATUS "Build configuration validated successfully")
endfunction() 