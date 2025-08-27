# Docking system module
set(DOCKING_SOURCES
    DockManager.cpp
    DockWidget.cpp
    DockArea.cpp
    DockContainerWidget.cpp
    FloatingDockContainer.cpp
    DockOverlay.cpp
    DockingIntegration.cpp
    AutoHideContainer.cpp
    PerspectiveManager.cpp
)

set(DOCKING_HEADERS
    ${CMAKE_SOURCE_DIR}/include/docking/DockManager.h
    ${CMAKE_SOURCE_DIR}/include/docking/DockWidget.h
    ${CMAKE_SOURCE_DIR}/include/docking/DockArea.h
    ${CMAKE_SOURCE_DIR}/include/docking/DockContainerWidget.h
    ${CMAKE_SOURCE_DIR}/include/docking/FloatingDockContainer.h
    ${CMAKE_SOURCE_DIR}/include/docking/DockOverlay.h
    ${CMAKE_SOURCE_DIR}/include/docking/DockingIntegration.h
    ${CMAKE_SOURCE_DIR}/include/docking/AutoHideContainer.h
    ${CMAKE_SOURCE_DIR}/include/docking/PerspectiveManager.h
)

# Create docking library
add_library(docking STATIC ${DOCKING_SOURCES} ${DOCKING_HEADERS})

# Include directories
target_include_directories(docking PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Link with wxWidgets
target_link_libraries(docking PUBLIC
    wx::core
    wx::base
)

# Set properties
set_target_properties(docking PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)