# Commands module - application commands and geometry factory
set(COMMANDS_SOURCES
    Command.cpp
    CommandDispatcher.cpp
    CommandErrorHelper.cpp
    CommandListenerManager.cpp
    CoordinateSystemVisibilityListener.cpp
    ReferenceGridToggleListener.cpp
    ChessboardGridToggleListener.cpp
    CreateBoxListener.cpp
    CreateCommand.cpp
    CreateConeListener.cpp
    CreateCylinderListener.cpp
    CreateSphereListener.cpp
    CreateTorusListener.cpp
    CreateTruncatedCylinderListener.cpp
    CreateWrenchListener.cpp
    EdgeSettingsListener.cpp
    FileExitListener.cpp
    FileNewListener.cpp
    FileOpenListener.cpp
    FileSaveAsListener.cpp
    FileSaveListener.cpp
    FixNormalsListener.cpp
    GeometryFactory.cpp
    HelpAboutListener.cpp
    ImportGeometryListener.cpp
    LightingSettingsListener.cpp
    MeshQualityDialogListener.cpp
    NavCubeConfigListener.cpp
    NavigationModeListener.cpp
    NormalFixDialogListener.cpp
    RedoListener.cpp
    RenderingSettingsListener.cpp
    SetTransparencyListener.cpp
    # ShowSilhouetteEdgesListener.cpp  # removed (replaced by Outline toggle)
    ShowFaceNormalsListener.cpp
    ShowFeatureEdgesListener.cpp
    ShowMeshEdgesListener.cpp
    ShowNormalsListener.cpp
    ShowOriginalEdgesListener.cpp
    ShowWireFrameListener.cpp
    TextureModeBlendListener.cpp
    TextureModeDecalListener.cpp
    TextureModeModulateListener.cpp
    TextureModeReplaceListener.cpp
    ToggleOutlineListener.cpp
    UndoListener.cpp
    ViewAllListener.cpp
    ViewFrontListener.cpp
    ViewIsometricListener.cpp
    ViewModeListener.cpp
    ViewRightListener.cpp
    ViewTopListener.cpp
    ZoomSpeedListener.cpp
    ViewBookmarkListener.cpp
    NavigationAnimationListener.cpp
    RenderPreviewSystemListener.cpp
    ShowFlatWidgetsExampleListener.cpp
    ExplodeAssemblyListener.cpp
    FaceQueryListener.cpp
    FaceQueryCommandListener.cpp
    SliceToggleListener.cpp
    DockLayoutConfigListener.cpp
)

set(COMMANDS_HEADERS
    ${CMAKE_SOURCE_DIR}/include/Command.h
    ${CMAKE_SOURCE_DIR}/include/CommandDispatcher.h
    ${CMAKE_SOURCE_DIR}/include/CommandErrorHelper.h
    ${CMAKE_SOURCE_DIR}/include/CommandListener.h
    ${CMAKE_SOURCE_DIR}/include/CommandListenerManager.h
    ${CMAKE_SOURCE_DIR}/include/CommandType.h
    ${CMAKE_SOURCE_DIR}/include/CoordinateSystemVisibilityListener.h
    ${CMAKE_SOURCE_DIR}/include/ReferenceGridToggleListener.h
    ${CMAKE_SOURCE_DIR}/include/ChessboardGridToggleListener.h
    ${CMAKE_SOURCE_DIR}/include/CreateBoxListener.h
    ${CMAKE_SOURCE_DIR}/include/CreateCommand.h
    ${CMAKE_SOURCE_DIR}/include/CreateConeListener.h
    ${CMAKE_SOURCE_DIR}/include/CreateCylinderListener.h
    ${CMAKE_SOURCE_DIR}/include/CreateSphereListener.h
    ${CMAKE_SOURCE_DIR}/include/CreateTorusListener.h
    ${CMAKE_SOURCE_DIR}/include/CreateTruncatedCylinderListener.h
    ${CMAKE_SOURCE_DIR}/include/CreateWrenchListener.h
    ${CMAKE_SOURCE_DIR}/include/EdgeSettingsListener.h
    ${CMAKE_SOURCE_DIR}/include/FileExitListener.h
    ${CMAKE_SOURCE_DIR}/include/FileNewListener.h
    ${CMAKE_SOURCE_DIR}/include/FileOpenListener.h
    ${CMAKE_SOURCE_DIR}/include/FileSaveAsListener.h
    ${CMAKE_SOURCE_DIR}/include/FileSaveListener.h
    ${CMAKE_SOURCE_DIR}/include/FixNormalsListener.h
    ${CMAKE_SOURCE_DIR}/include/GeometryFactory.h
    ${CMAKE_SOURCE_DIR}/include/HelpAboutListener.h
    ${CMAKE_SOURCE_DIR}/include/ImportGeometryListener.h
    ${CMAKE_SOURCE_DIR}/include/LightingSettingsListener.h
    ${CMAKE_SOURCE_DIR}/include/MeshQualityDialogListener.h
    ${CMAKE_SOURCE_DIR}/include/NavCubeConfigListener.h
    ${CMAKE_SOURCE_DIR}/include/NavigationModeListener.h
    ${CMAKE_SOURCE_DIR}/include/NormalFixDialogListener.h
    ${CMAKE_SOURCE_DIR}/include/RedoListener.h
    ${CMAKE_SOURCE_DIR}/include/RenderingSettingsListener.h
    ${CMAKE_SOURCE_DIR}/include/SetTransparencyListener.h
    # ${CMAKE_SOURCE_DIR}/include/ShowSilhouetteEdgesListener.h  # removed
    ${CMAKE_SOURCE_DIR}/include/ShowFaceNormalsListener.h
    ${CMAKE_SOURCE_DIR}/include/ShowFeatureEdgesListener.h
    ${CMAKE_SOURCE_DIR}/include/ShowMeshEdgesListener.h
    ${CMAKE_SOURCE_DIR}/include/ShowNormalsListener.h
    ${CMAKE_SOURCE_DIR}/include/ShowOriginalEdgesListener.h
    ${CMAKE_SOURCE_DIR}/include/ShowWireFrameListener.h
    ${CMAKE_SOURCE_DIR}/include/TextureModeBlendListener.h
    ${CMAKE_SOURCE_DIR}/include/TextureModeDecalListener.h
    ${CMAKE_SOURCE_DIR}/include/TextureModeModulateListener.h
    ${CMAKE_SOURCE_DIR}/include/TextureModeReplaceListener.h
    ${CMAKE_SOURCE_DIR}/include/UndoListener.h
    ${CMAKE_SOURCE_DIR}/include/ViewAllListener.h
    ${CMAKE_SOURCE_DIR}/include/ViewFrontListener.h
    ${CMAKE_SOURCE_DIR}/include/ViewIsometricListener.h
    ${CMAKE_SOURCE_DIR}/include/ViewModeListener.h
    ${CMAKE_SOURCE_DIR}/include/ViewRightListener.h
    ${CMAKE_SOURCE_DIR}/include/ViewTopListener.h
    ${CMAKE_SOURCE_DIR}/include/ZoomSpeedListener.h
    ${CMAKE_SOURCE_DIR}/include/ViewBookmarkListener.h
    ${CMAKE_SOURCE_DIR}/include/NavigationAnimationListener.h
    ${CMAKE_SOURCE_DIR}/include/RenderPreviewSystemListener.h
    ${CMAKE_SOURCE_DIR}/include/ShowFlatWidgetsExampleListener.h
    ${CMAKE_SOURCE_DIR}/include/ExplodeAssemblyListener.h
    ${CMAKE_SOURCE_DIR}/include/FaceQueryListener.h
    ${CMAKE_SOURCE_DIR}/include/FaceQueryCommandListener.h
    ${CMAKE_SOURCE_DIR}/include/SliceToggleListener.h
    ${CMAKE_SOURCE_DIR}/include/ExplodeConfigListener.h
    ${CMAKE_SOURCE_DIR}/include/ToggleOutlineListener.h
    ${CMAKE_SOURCE_DIR}/include/DockLayoutConfigListener.h
)

# Create command static library
add_library(CADCommands STATIC ${COMMANDS_SOURCES} ${COMMANDS_HEADERS})

target_include_directories(CADCommands PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${wxWidgets_INCLUDE_DIRS}
    ${Coin3D_INCLUDE_DIRS}
    ${OpenCASCADE_INCLUDE_DIR}
)

target_link_libraries(CADCommands PUBLIC
    CADCore
    CADLogger
    CADOCC
    CADGeometry
    UIDialogMesh
    ${wxWidgets_LIBRARIES}
    Coin::Coin
    ${OpenCASCADE_LIBRARIES}
)

# Enable precompiled headers for faster compilation
if(MSVC)
    target_precompile_headers(CADCommands PRIVATE
        <wx/wx.h>
        <vector>
        <string>
        <memory>
        <Inventor/nodes/SoSeparator.h>
    )
endif()

set_target_properties(CADCommands PROPERTIES
    FOLDER "Commands"
    EXPORT_NAME Commands
) 