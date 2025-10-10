# Rendering Toolkit module - new modular rendering system
set(RENDERING_TOOLKIT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/RenderingToolkitAPI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RenderConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RenderManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RenderPluginManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OpenCASCADEProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Coin3DBackendImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FrustumCuller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OcclusionCuller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/GPUEdgeRenderer.cpp
)

set(RENDERING_TOOLKIT_HEADERS
    ${CMAKE_SOURCE_DIR}/include/rendering/RenderingToolkit.h
    ${CMAKE_SOURCE_DIR}/include/rendering/GeometryProcessor.h
    ${CMAKE_SOURCE_DIR}/include/rendering/OpenCASCADEProcessor.h
    ${CMAKE_SOURCE_DIR}/include/rendering/RenderBackend.h
    ${CMAKE_SOURCE_DIR}/include/rendering/Coin3DBackend.h
    ${CMAKE_SOURCE_DIR}/include/rendering/RenderConfig.h
    ${CMAKE_SOURCE_DIR}/include/rendering/RenderManager.h
    ${CMAKE_SOURCE_DIR}/include/rendering/RenderPlugin.h
    ${CMAKE_SOURCE_DIR}/include/rendering/RenderingToolkitAPI.h
    ${CMAKE_SOURCE_DIR}/include/rendering/FrustumCuller.h
    ${CMAKE_SOURCE_DIR}/include/rendering/OcclusionCuller.h
    ${CMAKE_SOURCE_DIR}/include/rendering/GPUEdgeRenderer.h
)

# Create Rendering Toolkit static library
add_library(CADRenderingToolkit STATIC ${RENDERING_TOOLKIT_SOURCES} ${RENDERING_TOOLKIT_HEADERS})

# Set header directories
target_include_directories(CADRenderingToolkit PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
    ${Coin3D_INCLUDE_DIRS}
    ${OpenCASCADE_INCLUDE_DIR}
)

# Link dependencies
target_link_libraries(CADRenderingToolkit PUBLIC
    CADCore
    CADLogger
    CADConfig
    ${wxWidgets_LIBRARIES}
    Coin::Coin
    ${OpenCASCADE_LIBRARIES}
)

# Set library export properties
set_target_properties(CADRenderingToolkit PROPERTIES
    FOLDER "Rendering"
    EXPORT_NAME RenderingToolkit
) 