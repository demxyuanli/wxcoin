# UI module split into multiple libraries (no underscores in target names)

# Panels split
add_library(UIPanelTree STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ObjectTreePanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatBarNotebook.cpp
    ${CMAKE_SOURCE_DIR}/include/ObjectTreePanel.h
    ${CMAKE_SOURCE_DIR}/include/ui/FlatBarNotebook.h
)
target_include_directories(UIPanelTree PUBLIC ${CMAKE_SOURCE_DIR}/include ${wxWidgets_INCLUDE_DIRS})
target_link_libraries(UIPanelTree PUBLIC CADCore CADConfig CADLogger CADFlatUI widgets ${wxWidgets_LIBRARIES})
set_target_properties(UIPanelTree PROPERTIES FOLDER "UI")

add_library(UIPanelProperty STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/PropertyPanel.cpp
    ${CMAKE_SOURCE_DIR}/include/PropertyPanel.h
)
target_include_directories(UIPanelProperty PUBLIC ${CMAKE_SOURCE_DIR}/include ${wxWidgets_INCLUDE_DIRS})
target_link_libraries(UIPanelProperty PUBLIC CADCore CADConfig CADLogger CADFlatUI widgets ${wxWidgets_LIBRARIES})
set_target_properties(UIPanelProperty PROPERTIES FOLDER "UI")

# Aggregator for panels
add_library(UIPanels INTERFACE)
target_link_libraries(UIPanels INTERFACE UIPanelTree UIPanelProperty)
set_target_properties(UIPanels PROPERTIES FOLDER "UI")

# Dialogs split
add_library(UIDialogGeometry STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/PositionBasicDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VisualSettingsDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ExplodeConfigDialog.cpp
    ${CMAKE_SOURCE_DIR}/include/PositionBasicDialog.h
    ${CMAKE_SOURCE_DIR}/include/VisualSettingsDialog.h
    ${CMAKE_SOURCE_DIR}/include/ExplodeConfigDialog.h
    ${CMAKE_SOURCE_DIR}/include/GeometryDialogTypes.h
)
target_include_directories(UIDialogGeometry PUBLIC ${CMAKE_SOURCE_DIR}/include ${wxWidgets_INCLUDE_DIRS})
target_link_libraries(UIDialogGeometry PUBLIC CADCore CADConfig CADLogger CADFlatUI widgets ${wxWidgets_LIBRARIES})
set_target_properties(UIDialogGeometry PROPERTIES FOLDER "UI")

add_library(UIDialogRendering STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/RenderingSettingsDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LightingSettingsDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PointViewDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SelectionHighlightConfigDialog.cpp
    ${CMAKE_SOURCE_DIR}/include/RenderingSettingsDialog.h
    ${CMAKE_SOURCE_DIR}/include/LightingSettingsDialog.h
    ${CMAKE_SOURCE_DIR}/include/PointViewDialog.h
    ${CMAKE_SOURCE_DIR}/include/SelectionHighlightConfigDialog.h
)
target_include_directories(UIDialogRendering PUBLIC ${CMAKE_SOURCE_DIR}/include ${wxWidgets_INCLUDE_DIRS})
target_link_libraries(UIDialogRendering PUBLIC CADCore CADConfig CADLogger CADRendering CADFlatUI widgets CADCommands ${wxWidgets_LIBRARIES})
set_target_properties(UIDialogRendering PROPERTIES FOLDER "UI")

add_library(UIDialogEdges STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/EdgeSettingsDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FeatureEdgesParamDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OriginalEdgesParamDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/WireframeParamDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MeshEdgesParamDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OutlineSettingsDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OutlinePreviewCanvas.cpp

    ${CMAKE_SOURCE_DIR}/include/EdgeSettingsDialog.h
    ${CMAKE_SOURCE_DIR}/include/FeatureEdgesParamDialog.h
    ${CMAKE_SOURCE_DIR}/include/OriginalEdgesParamDialog.h
    ${CMAKE_SOURCE_DIR}/include/WireframeParamDialog.h
    ${CMAKE_SOURCE_DIR}/include/MeshEdgesParamDialog.h
    ${CMAKE_SOURCE_DIR}/include/ui/OutlineSettingsDialog.h
    ${CMAKE_SOURCE_DIR}/include/ui/OutlinePreviewCanvas.h


)


target_include_directories(UIDialogEdges PUBLIC ${CMAKE_SOURCE_DIR}/include ${wxWidgets_INCLUDE_DIRS})
target_link_libraries(UIDialogEdges PUBLIC CADCore CADConfig CADLogger CADRendering CADOCC CADFlatUI widgets ${wxWidgets_LIBRARIES})
set_target_properties(UIDialogEdges PROPERTIES FOLDER "UI")

set(UIDialogMesh_SOURCES
    MeshQualityDialog.cpp
    TransparencyDialog.cpp
    ImportSettingsDialog.cpp
    ImportStatisticsDialog.cpp
    NormalFixDialog.cpp
    GeometryDecompositionDialog.cpp
    SliceParamDialog.cpp
    ${CMAKE_SOURCE_DIR}/include/MeshQualityDialog.h
    ${CMAKE_SOURCE_DIR}/include/TransparencyDialog.h
    ${CMAKE_SOURCE_DIR}/include/ImportSettingsDialog.h
    ${CMAKE_SOURCE_DIR}/include/ImportStatisticsDialog.h
    ${CMAKE_SOURCE_DIR}/include/NormalFixDialog.h
    ${CMAKE_SOURCE_DIR}/include/GeometryDecompositionDialog.h
    ${CMAKE_SOURCE_DIR}/include/SliceParamDialog.h
)

add_library(UIDialogMesh STATIC ${UIDialogMesh_SOURCES})
target_include_directories(UIDialogMesh PUBLIC ${CMAKE_SOURCE_DIR}/include ${wxWidgets_INCLUDE_DIRS})
target_link_libraries(UIDialogMesh PUBLIC CADCore CADConfig CADLogger CADParam CADRendering CADFlatUI widgets ${wxWidgets_LIBRARIES})
set_target_properties(UIDialogMesh PROPERTIES FOLDER "UI")

# Aggregator for dialogs
add_library(UIDialogs INTERFACE)
target_link_libraries(UIDialogs INTERFACE UIDialogGeometry UIDialogRendering UIDialogEdges UIDialogMesh)
set_target_properties(UIDialogs PROPERTIES FOLDER "UI")

# Frame library
set(UIFrame_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatFrame.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatFrameInit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatFrameEvents.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatFrameCommands.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatFrameStatus.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PerformancePanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AsyncEnginePanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SplashScreen.cpp
)
set(UIFrame_HEADERS
    ${CMAKE_SOURCE_DIR}/include/FlatFrame.h
    ${CMAKE_SOURCE_DIR}/include/ui/PerformancePanel.h
    ${CMAKE_SOURCE_DIR}/include/ui/AsyncEnginePanel.h
    ${CMAKE_SOURCE_DIR}/include/SplashScreen.h
)
add_library(UIFrame STATIC ${UIFrame_SOURCES} ${UIFrame_HEADERS})
target_include_directories(UIFrame PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
)
target_link_libraries(UIFrame PUBLIC
    UIPanels
    UIDialogs
    CADCore
    CADConfig
    CADLogger
    CADRendering
    CADInput
    NavCubeLib
    CADOCC
    CADGeometry
    AsyncEngine
    CADCommands
    RenderPreview
    CADFlatUI
    widgets
    ${wxWidgets_LIBRARIES}
    Coin::Coin
    ${OpenCASCADE_LIBRARIES}
)

# Enable precompiled headers for faster compilation
if(MSVC)
    target_precompile_headers(UIFrame PRIVATE
        <wx/wx.h>
        <wx/panel.h>
        <wx/sizer.h>
        <vector>
        <string>
        <memory>
    )
endif()

set_target_properties(UIFrame PROPERTIES FOLDER "UI")

# Add FlatFrameDocking to UIFrame
target_sources(UIFrame PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/FlatFrameDocking.cpp
    ${CMAKE_SOURCE_DIR}/include/FlatFrameDocking.h
)

# Add docking library dependency
target_link_libraries(UIFrame PUBLIC docking)

# SimpleDockingFrame executable
add_executable(SimpleDockingFrame WIN32
    ${CMAKE_CURRENT_SOURCE_DIR}/SimpleDockingFrame.cpp
)
target_include_directories(SimpleDockingFrame PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
)
target_link_libraries(SimpleDockingFrame PUBLIC
    UIPanels
    UIDialogs
    docking
    CADCore
    CADConfig
    CADLogger
    CADFlatUI
    widgets
    CADOCC
    CADGeometry
    CADRendering
    CADInput
    NavCubeLib
    CADCommands
    ${wxWidgets_LIBRARIES}
    Coin::Coin
    ${OpenCASCADE_LIBRARIES}
)
set_target_properties(SimpleDockingFrame PROPERTIES FOLDER "UI")

# Aggregator target to keep existing references working
add_library(CADUI INTERFACE)
target_link_libraries(CADUI INTERFACE UIFrame UIPanels UIDialogs)
set_target_properties(CADUI PROPERTIES FOLDER "UI" EXPORT_NAME UI)