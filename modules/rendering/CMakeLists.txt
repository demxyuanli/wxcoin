# Rendering Module - 渲染模块
# 包含3D画布、场景管理器、坐标系渲染、DPI管理等渲染功能

cmake_minimum_required(VERSION 3.20)

set(MODULE_NAME wxcoin_rendering)

# Rendering module sources
set(RENDERING_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Canvas.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CoordinateSystemRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DPIAwareRendering.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DPIManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PickingAidManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SceneManager.cpp
)

# Rendering module headers
set(RENDERING_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Canvas.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/CoordinateSystemRenderer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/DPIAwareRendering.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/DPIManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/PickingAidManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/SceneManager.h
)

# Create rendering static library
add_library(${MODULE_NAME} STATIC ${RENDERING_SOURCES} ${RENDERING_HEADERS})

# Set target properties
set_target_properties(${MODULE_NAME} PROPERTIES
    OUTPUT_NAME "rendering"
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
    RENDERING_MODULE_EXPORTS
    $<$<BOOL:${ENABLE_HIGH_DPI}>:ENABLE_HIGH_DPI>
)

message(STATUS "Configured Rendering module: ${MODULE_NAME}")
message(STATUS "  Sources: ${RENDERING_SOURCES}")
message(STATUS "  Dependencies: Core") 