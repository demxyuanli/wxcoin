# Commands module - application commands and geometry factory
set(COMMANDS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Command.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CreateCommand.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CommandDispatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ViewAllListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ViewTopListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ViewFrontListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ViewRightListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ViewIsometricListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ShowNormalsListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FixNormalsListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ShowEdgesListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SetTransparencyListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ViewModeListener.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/FileNewListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportStepListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FileOpenListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FileSaveListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FileSaveAsListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CreateBoxListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CreateSphereListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CreateCylinderListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CreateConeListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CreateTorusListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CreateTruncatedCylinderListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CreateWrenchListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UndoListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RedoListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/HelpAboutListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NavCubeConfigListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ZoomSpeedListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FileExitListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CommandListenerManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MeshQualityDialogListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RenderingSettingsListener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RefreshCommand.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RefreshCommandListener.cpp
)

set(COMMANDS_HEADERS
    ${CMAKE_SOURCE_DIR}/include/Command.h
    ${CMAKE_SOURCE_DIR}/include/CreateCommand.h
    ${CMAKE_SOURCE_DIR}/include/CommandDispatcher.h
    ${CMAKE_SOURCE_DIR}/include/CommandListener.h
    ${CMAKE_SOURCE_DIR}/include/FileNewListener.h
    ${CMAKE_SOURCE_DIR}/include/ImportStepListener.h
    ${CMAKE_SOURCE_DIR}/include/FileOpenListener.h
    ${CMAKE_SOURCE_DIR}/include/FileSaveListener.h
    ${CMAKE_SOURCE_DIR}/include/FileSaveAsListener.h
    ${CMAKE_SOURCE_DIR}/include/CreateBoxListener.h
    ${CMAKE_SOURCE_DIR}/include/CreateSphereListener.h
    ${CMAKE_SOURCE_DIR}/include/CreateCylinderListener.h
    ${CMAKE_SOURCE_DIR}/include/CreateConeListener.h
    ${CMAKE_SOURCE_DIR}/include/CreateTorusListener.h
    ${CMAKE_SOURCE_DIR}/include/CreateTruncatedCylinderListener.h
    ${CMAKE_SOURCE_DIR}/include/CreateWrenchListener.h
    ${CMAKE_SOURCE_DIR}/include/ViewAllListener.h
    ${CMAKE_SOURCE_DIR}/include/ViewTopListener.h
    ${CMAKE_SOURCE_DIR}/include/ViewFrontListener.h
    ${CMAKE_SOURCE_DIR}/include/ViewRightListener.h
    ${CMAKE_SOURCE_DIR}/include/ViewIsometricListener.h
    ${CMAKE_SOURCE_DIR}/include/ShowNormalsListener.h
    ${CMAKE_SOURCE_DIR}/include/FixNormalsListener.h
    ${CMAKE_SOURCE_DIR}/include/ShowEdgesListener.h
    ${CMAKE_SOURCE_DIR}/include/SetTransparencyListener.h
    ${CMAKE_SOURCE_DIR}/include/ViewModeListener.h

    ${CMAKE_SOURCE_DIR}/include/UndoListener.h
    ${CMAKE_SOURCE_DIR}/include/RedoListener.h
    ${CMAKE_SOURCE_DIR}/include/HelpAboutListener.h
    ${CMAKE_SOURCE_DIR}/include/NavCubeConfigListener.h
    ${CMAKE_SOURCE_DIR}/include/ZoomSpeedListener.h
    ${CMAKE_SOURCE_DIR}/include/FileExitListener.h
    ${CMAKE_SOURCE_DIR}/include/CommandListenerManager.h
    ${CMAKE_SOURCE_DIR}/include/MeshQualityDialogListener.h
    ${CMAKE_SOURCE_DIR}/include/RenderingSettingsListener.h
    ${CMAKE_SOURCE_DIR}/include/RefreshCommand.h
    ${CMAKE_SOURCE_DIR}/include/RefreshCommandListener.h
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
    optimizer
    ${wxWidgets_LIBRARIES}
    Coin::Coin
    ${OpenCASCADE_LIBRARIES}
)

set_target_properties(CADCommands PROPERTIES
    FOLDER "Commands"
    EXPORT_NAME Commands
) 