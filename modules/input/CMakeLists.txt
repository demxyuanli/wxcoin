# Input Module - 输入模块
# 包含输入管理器、鼠标处理器等输入处理功能

cmake_minimum_required(VERSION 3.20)

set(MODULE_NAME wxcoin_input)

# Input module sources
set(INPUT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/InputManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MouseHandler.cpp
)

# Input module headers
set(INPUT_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/InputManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/MouseHandler.h
)

# Create input static library
add_library(${MODULE_NAME} STATIC ${INPUT_SOURCES} ${INPUT_HEADERS})

# Set target properties
set_target_properties(${MODULE_NAME} PROPERTIES
    OUTPUT_NAME "input"
    DEBUG_POSTFIX "_d"
    FOLDER "Modules"
)

# Include directories
target_include_directories(${MODULE_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
    ${Coin3D_INCLUDE_DIRS}
)

# Dependencies
target_link_libraries(${MODULE_NAME} PUBLIC
    wxcoin_core               # Depends on core module
    wxcoin_rendering          # Depends on rendering for canvas interaction
    ${wxWidgets_LIBRARIES}
)

# Compiler-specific options
if(MSVC)
    target_compile_options(${MODULE_NAME} PRIVATE
        /W4
        $<$<CONFIG:Debug>:/ZI>
    )
endif()

# Export definitions
target_compile_definitions(${MODULE_NAME} PUBLIC
    INPUT_MODULE_EXPORTS
)

message(STATUS "Configured Input module: ${MODULE_NAME}")
message(STATUS "  Sources: ${INPUT_SOURCES}")
message(STATUS "  Dependencies: Core, Rendering") 