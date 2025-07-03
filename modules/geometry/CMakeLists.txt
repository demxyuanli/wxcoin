# Geometry Module - 几何模块
# 包含几何对象创建、几何工厂、几何数据管理等功能

cmake_minimum_required(VERSION 3.20)

set(MODULE_NAME wxcoin_geometry)

# Geometry module sources
set(GEOMETRY_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GeometryFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GeometryObject.cpp
)

# Geometry module headers
set(GEOMETRY_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/GeometryFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/GeometryObject.h
)

# Create geometry static library
add_library(${MODULE_NAME} STATIC ${GEOMETRY_SOURCES} ${GEOMETRY_HEADERS})

# Set target properties
set_target_properties(${MODULE_NAME} PROPERTIES
    OUTPUT_NAME "geometry"
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
    wxcoin_rendering          # Depends on rendering for scene management
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
    GEOMETRY_MODULE_EXPORTS
)

message(STATUS "Configured Geometry module: ${MODULE_NAME}")
message(STATUS "  Sources: ${GEOMETRY_SOURCES}")
message(STATUS "  Dependencies: Core, Rendering") 