# Navigation module - navigation control and navigation cube
set(NAVIGATION_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/NavigationController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NavigationStyle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NavigationCube.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NavigationCubeManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CuteNavCube.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NavigationCubeConfigDialog.cpp
)

set(NAVIGATION_HEADERS
    ${CMAKE_SOURCE_DIR}/include/NavigationController.h
    ${CMAKE_SOURCE_DIR}/include/NavigationStyle.h
    ${CMAKE_SOURCE_DIR}/include/NavigationCube.h
    ${CMAKE_SOURCE_DIR}/include/NavigationCubeManager.h
    ${CMAKE_SOURCE_DIR}/include/CuteNavCube.h
    ${CMAKE_SOURCE_DIR}/include/NavigationCubeConfigDialog.h
)

# Create navigation static library
add_library(NavCubeLib STATIC ${NAVIGATION_SOURCES} ${NAVIGATION_HEADERS})

# Set header directories
target_include_directories(NavCubeLib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
    ${Coin3D_INCLUDE_DIRS}
)

# Link dependencies
target_link_libraries(NavCubeLib PUBLIC
    CADCore
    CADLogger
    CADRendering
    CADOCC
    ${wxWidgets_LIBRARIES}
    Coin::Coin
)

# Set library export properties
set_target_properties(NavCubeLib PROPERTIES
    FOLDER "Navigation"
    EXPORT_NAME Navigation
) 