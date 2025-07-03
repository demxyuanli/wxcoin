# Core Module - 基础功能模块
# 包含应用程序框架、日志系统、全局配置等核心功能

cmake_minimum_required(VERSION 3.20)

set(MODULE_NAME wxcoin_core)

# Core module sources
set(CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Globals.cpp
)

# Core module headers
set(CORE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Application.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Logger.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Globals.h
)

# Create core static library
add_library(${MODULE_NAME} STATIC ${CORE_SOURCES} ${CORE_HEADERS})

# Set target properties
set_target_properties(${MODULE_NAME} PROPERTIES
    OUTPUT_NAME "core"
    DEBUG_POSTFIX "_d"
    FOLDER "Modules"
)

# Include directories
target_include_directories(${MODULE_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
    ${Coin3D_INCLUDE_DIRS}
)

# Link dependencies
target_link_libraries(${MODULE_NAME} PUBLIC
    ${wxWidgets_LIBRARIES}
)

# Compiler-specific options
if(MSVC)
    target_compile_options(${MODULE_NAME} PRIVATE
        /W4                    # High warning level
        /WX-                   # Warnings not as errors for now
        $<$<CONFIG:Debug>:/ZI> # Debug information
    )
endif()

# Export definitions
target_compile_definitions(${MODULE_NAME} PUBLIC
    $<$<BOOL:${WIN32}>:UNICODE _UNICODE>
    $<$<BOOL:${ENABLE_DEBUG_LOGS}>:ENABLE_DEBUG_LOGS>
)

message(STATUS "Configured Core module: ${MODULE_NAME}")
message(STATUS "  Sources: ${CORE_SOURCES}")
message(STATUS "  Headers: ${CORE_HEADERS}") 