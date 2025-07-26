# UI module - interface components and dialogs
set(UI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/ObjectTreePanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PropertyPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PositionBasicDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VisualSettingsDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MeshQualityDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TransparencyDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RenderingSettingsDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/EdgeSettingsDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LightingSettingsDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatFrame.cpp
)

set(UI_HEADERS
    ${CMAKE_SOURCE_DIR}/include/ObjectTreePanel.h
    ${CMAKE_SOURCE_DIR}/include/PropertyPanel.h
    ${CMAKE_SOURCE_DIR}/include/PositionBasicDialog.h
    ${CMAKE_SOURCE_DIR}/include/VisualSettingsDialog.h
    ${CMAKE_SOURCE_DIR}/include/GeometryDialogTypes.h
    ${CMAKE_SOURCE_DIR}/include/MeshQualityDialog.h
    ${CMAKE_SOURCE_DIR}/include/TransparencyDialog.h
    ${CMAKE_SOURCE_DIR}/include/RenderingSettingsDialog.h
    ${CMAKE_SOURCE_DIR}/include/EdgeSettingsDialog.h
    ${CMAKE_SOURCE_DIR}/include/LightingSettingsDialog.h
    ${CMAKE_SOURCE_DIR}/include/FlatFrame.h
)

# Create UI static library
add_library(CADUI STATIC ${UI_SOURCES} ${UI_HEADERS})

# Set header directories
target_include_directories(CADUI PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
    ${Coin3D_INCLUDE_DIRS}
)

# Link dependencies
target_link_libraries(CADUI PUBLIC
    CADCore
    CADConfig
    CADLogger
    CADRendering
    CADInput
    NavCubeLib
    CADOCC
    CADCommands
    CADFlatUI
    ${wxWidgets_LIBRARIES}
    Coin::Coin
    ${OpenCASCADE_LIBRARIES}
)

# Set library export properties
set_target_properties(CADUI PROPERTIES
    FOLDER "UI"
    EXPORT_NAME UI
) 