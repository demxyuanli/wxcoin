# UI Module - 用户界面模块
# 包含主窗口、对象树面板、属性面板、位置对话框等用户界面功能

cmake_minimum_required(VERSION 3.20)

set(MODULE_NAME wxcoin_ui)

# UI module sources
set(UI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MainFrame.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ObjectTreePanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PropertyPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PositionDialog.cpp
)

# UI module headers
set(UI_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/MainFrame.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ObjectTreePanel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/PropertyPanel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/PositionDialog.h
)

# Create UI static library
add_library(${MODULE_NAME} STATIC ${UI_SOURCES} ${UI_HEADERS})

# Set target properties
set_target_properties(${MODULE_NAME} PROPERTIES
    OUTPUT_NAME "ui"
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
    wxcoin_rendering          # Depends on rendering for Canvas
    wxcoin_geometry           # Depends on geometry for object management
    wxcoin_input              # Depends on input for interaction
    wxcoin_navigation         # Depends on navigation for 3D controls
    wxcoin_commands           # Depends on commands for operations
    ${wxWidgets_LIBRARIES}
    Coin::Coin
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
    UI_MODULE_EXPORTS
    $<$<BOOL:${ENABLE_HIGH_DPI}>:ENABLE_HIGH_DPI>
)

message(STATUS "Configured UI module: ${MODULE_NAME}")
message(STATUS "  Sources: ${UI_SOURCES}")
message(STATUS "  Dependencies: Core, Rendering, Geometry, Input, Navigation, Commands") 