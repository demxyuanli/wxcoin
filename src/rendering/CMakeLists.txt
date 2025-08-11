# Rendering module - rendering engine and scene management
set(RENDERING_SOURCES
    # moved to CADView: ${CMAKE_CURRENT_SOURCE_DIR}/Canvas.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RenderingEngine.cpp
    # moved to CADView: ${CMAKE_CURRENT_SOURCE_DIR}/ViewportManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SceneManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CoordinateSystemRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PickingAidManager.cpp
    # moved to CADView: ${CMAKE_CURRENT_SOURCE_DIR}/MultiViewportManager.cpp
    # moved to CADView: ${CMAKE_CURRENT_SOURCE_DIR}/ViewRefreshManager.cpp
    # moved to CADView: ${CMAKE_CURRENT_SOURCE_DIR}/DynamicSilhouetteRenderer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/ServiceLocator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/DefaultSubsystemFactory.cpp
)

set(RENDERING_HEADERS
    ${CMAKE_SOURCE_DIR}/include/Canvas.h
    ${CMAKE_SOURCE_DIR}/include/RenderingEngine.h
    # moved to CADView: ${CMAKE_SOURCE_DIR}/include/ViewportManager.h
    ${CMAKE_SOURCE_DIR}/include/SceneManager.h
    ${CMAKE_SOURCE_DIR}/include/CoordinateSystemRenderer.h
    ${CMAKE_SOURCE_DIR}/include/PickingAidManager.h
    # moved to CADView: ${CMAKE_SOURCE_DIR}/include/MultiViewportManager.h
    # moved to CADView: ${CMAKE_SOURCE_DIR}/include/ViewRefreshManager.h
    # moved to CADView: ${CMAKE_SOURCE_DIR}/include/DynamicSilhouetteRenderer.h
)

# Create rendering static library
add_library(CADRendering STATIC ${RENDERING_SOURCES} ${RENDERING_HEADERS})

# Set header directories
target_include_directories(CADRendering PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
    ${Coin3D_INCLUDE_DIRS}
)

# Link dependencies
target_link_libraries(CADRendering PUBLIC
    CADCore
    CADOCC
    CADLogger
    ${wxWidgets_LIBRARIES}
    Coin::Coin
)

# Set library export properties
set_target_properties(CADRendering PROPERTIES
    FOLDER "Rendering"
    EXPORT_NAME Rendering
) 