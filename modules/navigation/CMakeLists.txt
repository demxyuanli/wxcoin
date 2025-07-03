# Navigation Module - 导航模块
# 包含导航控制器、导航立方体、导航样式等3D导航功能

cmake_minimum_required(VERSION 3.20)

set(MODULE_NAME wxcoin_navigation)

# Navigation module sources
set(NAVIGATION_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/NavigationController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/NavigationCube.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/NavigationCubeConfigDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/NavigationStyle.cpp
)

# Navigation module headers
set(NAVIGATION_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/NavigationController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/NavigationCube.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/NavigationCubeConfigDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/NavigationStyle.h
)

# Create navigation static library
add_library(${MODULE_NAME} STATIC ${NAVIGATION_SOURCES} ${NAVIGATION_HEADERS})

# Set target properties
set_target_properties(${MODULE_NAME} PROPERTIES
    OUTPUT_NAME "navigation"
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
    wxcoin_rendering          # Depends on rendering for scene interaction
    wxcoin_input              # Depends on input for user interaction
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
    NAVIGATION_MODULE_EXPORTS
)

message(STATUS "Configured Navigation module: ${MODULE_NAME}")
message(STATUS "  Sources: ${NAVIGATION_SOURCES}")
message(STATUS "  Dependencies: Core, Rendering, Input") 