# Dependency Management Helpers
# This file provides macros to properly manage library dependencies

# Create an interface library for common includes
# This avoids recompiling when headers change
macro(add_common_interface_library NAME)
    add_library(${NAME} INTERFACE)
    target_include_directories(${NAME} INTERFACE
        ${CMAKE_SOURCE_DIR}/include
        ${wxWidgets_INCLUDE_DIRS}
    )
endmacro()

# Helper to add library with proper dependency scoping
# Most internal dependencies should be PRIVATE, not PUBLIC
function(add_optimized_library)
    set(options STATIC SHARED)
    set(oneValueArgs NAME)
    set(multiValueArgs 
        SOURCES 
        HEADERS 
        PUBLIC_DEPS 
        PRIVATE_DEPS 
        INTERFACE_DEPS
        PUBLIC_INCLUDES
        PRIVATE_INCLUDES
    )
    
    cmake_parse_arguments(ARG 
        "${options}" 
        "${oneValueArgs}" 
        "${multiValueArgs}" 
        ${ARGN}
    )
    
    # Determine library type
    if(ARG_STATIC)
        set(LIB_TYPE STATIC)
    elseif(ARG_SHARED)
        set(LIB_TYPE SHARED)
    else()
        set(LIB_TYPE STATIC)  # Default to STATIC
    endif()
    
    # Create library
    add_library(${ARG_NAME} ${LIB_TYPE} ${ARG_SOURCES} ${ARG_HEADERS})
    
    # Set include directories
    if(ARG_PUBLIC_INCLUDES)
        target_include_directories(${ARG_NAME} PUBLIC ${ARG_PUBLIC_INCLUDES})
    endif()
    
    if(ARG_PRIVATE_INCLUDES)
        target_include_directories(${ARG_NAME} PRIVATE ${ARG_PRIVATE_INCLUDES})
    endif()
    
    # Always add common includes as PRIVATE
    target_include_directories(${ARG_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${wxWidgets_INCLUDE_DIRS}
    )
    
    # Link dependencies with proper scoping
    if(ARG_PUBLIC_DEPS)
        target_link_libraries(${ARG_NAME} PUBLIC ${ARG_PUBLIC_DEPS})
    endif()
    
    if(ARG_PRIVATE_DEPS)
        target_link_libraries(${ARG_NAME} PRIVATE ${ARG_PRIVATE_DEPS})
    endif()
    
    if(ARG_INTERFACE_DEPS)
        target_link_libraries(${ARG_NAME} INTERFACE ${ARG_INTERFACE_DEPS})
    endif()
    
    # Set C++ standard
    set_target_properties(${ARG_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )
    
endfunction()

message(STATUS "Dependency helpers loaded")


