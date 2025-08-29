# Viewer module - Real-time outline and silhouette rendering
set(VIEWER_SOURCES
    ImageOutlinePass.cpp
    OutlineDisplayManager.cpp
    HoverSilhouetteManager.cpp
    DynamicSilhouetteRenderer.cpp
    PickingService.cpp
)

# Add viewer sources to the parent target
target_sources(${PROJECT_NAME} PRIVATE ${VIEWER_SOURCES})

# Ensure headers are found
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}/include/viewer
)