# Commands Module - 命令模块
# 包含命令基类、创建命令等命令模式实现

cmake_minimum_required(VERSION 3.20)

set(MODULE_NAME wxcoin_commands)

# Commands module sources
set(COMMANDS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Command.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CreateCommand.cpp
)

# Commands module headers
set(COMMANDS_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Command.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/CreateCommand.h
)

# Create commands static library
add_library(${MODULE_NAME} STATIC ${COMMANDS_SOURCES} ${COMMANDS_HEADERS})

# Set target properties
set_target_properties(${MODULE_NAME} PROPERTIES
    OUTPUT_NAME "commands"
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
    wxcoin_geometry           # Depends on geometry for object creation
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
    COMMANDS_MODULE_EXPORTS
)

message(STATUS "Configured Commands module: ${MODULE_NAME}")
message(STATUS "  Sources: ${COMMANDS_SOURCES}")
message(STATUS "  Dependencies: Core, Geometry") 