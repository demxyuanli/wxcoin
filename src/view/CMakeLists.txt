# CADView: viewport and event orchestration layer

set(CADVIEW_SOURCES
    ${CMAKE_SOURCE_DIR}/src/view/Canvas.cpp
    ${CMAKE_SOURCE_DIR}/src/view/CanvasOptimized.cpp
    ${CMAKE_SOURCE_DIR}/src/view/ViewportManager.cpp
    ${CMAKE_SOURCE_DIR}/src/view/MultiViewportManager.cpp
    ${CMAKE_SOURCE_DIR}/src/view/SplitViewportManager.cpp
    ${CMAKE_SOURCE_DIR}/src/view/CoordinateTransformer.cpp
    ${CMAKE_SOURCE_DIR}/src/view/ViewBookmark.cpp
    ${CMAKE_SOURCE_DIR}/src/view/CameraAnimation.cpp
    ${CMAKE_SOURCE_DIR}/src/view/ZoomController.cpp
    ${CMAKE_SOURCE_DIR}/src/view/ViewRefreshManager.cpp
    ${CMAKE_SOURCE_DIR}/src/view/DynamicSilhouetteRenderer.cpp
    ${CMAKE_SOURCE_DIR}/src/view/FaceInfoOverlay.cpp
    ${CMAKE_SOURCE_DIR}/src/view/SoFCBackgroundGradient.cpp
    ${CMAKE_SOURCE_DIR}/src/view/SoFCBackgroundImage.cpp
    # ${CMAKE_SOURCE_DIR}/src/view/NavigationFeaturesTest.cpp  # Uncomment to enable tests
)

set(CADVIEW_HEADERS
    ${CMAKE_SOURCE_DIR}/include/Canvas.h
    ${CMAKE_SOURCE_DIR}/include/view/CanvasOptimized.h
    ${CMAKE_SOURCE_DIR}/include/ViewportManager.h
    ${CMAKE_SOURCE_DIR}/include/MultiViewportManager.h
    ${CMAKE_SOURCE_DIR}/include/SplitViewportManager.h
    ${CMAKE_SOURCE_DIR}/include/CoordinateTransformer.h
    ${CMAKE_SOURCE_DIR}/include/ViewportConfig.h
    ${CMAKE_SOURCE_DIR}/include/ViewBookmark.h
    ${CMAKE_SOURCE_DIR}/include/CameraAnimation.h
    ${CMAKE_SOURCE_DIR}/include/ZoomController.h
    ${CMAKE_SOURCE_DIR}/include/ViewRefreshManager.h
    ${CMAKE_SOURCE_DIR}/include/EventCoordinator.h
    ${CMAKE_SOURCE_DIR}/include/DynamicSilhouetteRenderer.h
    ${CMAKE_SOURCE_DIR}/include/FaceInfoOverlay.h
    ${CMAKE_SOURCE_DIR}/include/SoFCBackgroundGradient.h
    ${CMAKE_SOURCE_DIR}/include/SoFCBackgroundImage.h
    ${CMAKE_SOURCE_DIR}/include/interfaces/ISplitViewportManager.h
)

add_library(CADView STATIC ${CADVIEW_SOURCES} ${CADVIEW_HEADERS})

target_include_directories(CADView PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
)

target_link_libraries(CADView PUBLIC
    CADCore
    CADLogger
    CADConfig
    CADRendering
    CADInput
    CADOCC
    ${wxWidgets_LIBRARIES}
)

set_target_properties(CADView PROPERTIES
    FOLDER "View"
    EXPORT_NAME View
)


