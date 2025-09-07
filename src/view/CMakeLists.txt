# CADView: viewport and event orchestration layer

set(CADVIEW_SOURCES
    ${CMAKE_SOURCE_DIR}/src/view/Canvas.cpp
    ${CMAKE_SOURCE_DIR}/src/view/ViewportManager.cpp
    ${CMAKE_SOURCE_DIR}/src/view/MultiViewportManager.cpp
    ${CMAKE_SOURCE_DIR}/src/view/ViewRefreshManager.cpp
    ${CMAKE_SOURCE_DIR}/src/view/DynamicSilhouetteRenderer.cpp
)

set(CADVIEW_HEADERS
    ${CMAKE_SOURCE_DIR}/include/ViewportManager.h
    ${CMAKE_SOURCE_DIR}/include/MultiViewportManager.h
    ${CMAKE_SOURCE_DIR}/include/ViewRefreshManager.h
    ${CMAKE_SOURCE_DIR}/include/EventCoordinator.h
    ${CMAKE_SOURCE_DIR}/include/DynamicSilhouetteRenderer.h
)

add_library(CADView STATIC ${CADVIEW_SOURCES} ${CADVIEW_HEADERS})

target_include_directories(CADView PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
)

target_link_libraries(CADView PUBLIC
    CADCore
    CADLogger
    CADRendering
    CADInput
    ${wxWidgets_LIBRARIES}
)

set_target_properties(CADView PROPERTIES
    FOLDER "View"
    EXPORT_NAME View
)


