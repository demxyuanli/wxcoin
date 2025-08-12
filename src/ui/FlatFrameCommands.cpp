#include "FlatFrame.h"
#include "Command.h"
#include "CommandDispatcher.h"
#include "CommandListenerManager.h"
#include "CommandType.h"
#include "Canvas.h"
#include "InputManager.h"
#include "logger/Logger.h"
#include <unordered_map>
#include "FileNewListener.h"
#include "FileOpenListener.h"
#include "FileSaveListener.h"
#include "FileSaveAsListener.h"
#include "ImportStepListener.h"
#include "CreateBoxListener.h"
#include "CreateSphereListener.h"
#include "CreateCylinderListener.h"
#include "CreateConeListener.h"
#include "CreateTorusListener.h"
#include "CreateTruncatedCylinderListener.h"
#include "CreateWrenchListener.h"
#include "ViewAllListener.h"
#include "ViewTopListener.h"
#include "ViewFrontListener.h"
#include "ViewRightListener.h"
#include "ViewIsometricListener.h"
#include "ShowNormalsListener.h"
#include "FixNormalsListener.h"
// #include "ShowSilhouetteEdgesListener.h" // removed
#include "SetTransparencyListener.h"
#include "TextureModeDecalListener.h"
#include "TextureModeModulateListener.h"
#include "TextureModeReplaceListener.h"
#include "TextureModeBlendListener.h"
#include "ViewModeListener.h"
#include "EdgeSettingsListener.h"
#include "LightingSettingsListener.h"
#include "RenderingSettingsListener.h"
#include "MeshQualityDialogListener.h"
#include "CoordinateSystemVisibilityListener.h"
#include "ShowOriginalEdgesListener.h"
#include "ShowFeatureEdgesListener.h"
#include "ShowMeshEdgesListener.h"
#include "ShowWireFrameListener.h"
#include "ShowFaceNormalsListener.h"
#include "RenderPreviewSystemListener.h"
#include "ShowFlatWidgetsExampleListener.h"
#include "ReferenceGridToggleListener.h"
#include "ChessboardGridToggleListener.h"
#include "ExplodeAssemblyListener.h"
#include "ExplodeConfigListener.h"
#include "SliceToggleListener.h"
#include "ToggleOutlineListener.h"
#include "UndoListener.h"
#include "RedoListener.h"
#include "HelpAboutListener.h"
#include "NavCubeConfigListener.h"
#include "ZoomSpeedListener.h"
#include "FileExitListener.h"

void FlatFrame::setupCommandSystem() {
    LOG_INF_S("Setting up command system");
    m_commandDispatcher = std::make_unique<CommandDispatcher>();

    auto createBoxListener = std::make_shared<CreateBoxListener>(m_mouseHandler);
    auto createSphereListener = std::make_shared<CreateSphereListener>(m_mouseHandler);
    auto createCylinderListener = std::make_shared<CreateCylinderListener>(m_mouseHandler);
    auto createConeListener = std::make_shared<CreateConeListener>(m_mouseHandler);
    auto createTorusListener = std::make_shared<CreateTorusListener>(m_mouseHandler);
    auto createTruncatedCylinderListener = std::make_shared<CreateTruncatedCylinderListener>(m_mouseHandler);
    auto createWrenchListener = std::make_shared<CreateWrenchListener>(m_mouseHandler, m_geometryFactory);

    m_listenerManager = std::make_unique<CommandListenerManager>();
    m_listenerManager->registerListener(cmd::CommandType::CreateBox, createBoxListener);
    m_listenerManager->registerListener(cmd::CommandType::CreateSphere, createSphereListener);
    m_listenerManager->registerListener(cmd::CommandType::CreateCylinder, createCylinderListener);
    m_listenerManager->registerListener(cmd::CommandType::CreateCone, createConeListener);
    m_listenerManager->registerListener(cmd::CommandType::CreateTorus, createTorusListener);
    m_listenerManager->registerListener(cmd::CommandType::CreateTruncatedCylinder, createTruncatedCylinderListener);
    m_listenerManager->registerListener(cmd::CommandType::CreateWrench, createWrenchListener);

    auto viewAllListener = std::make_shared<ViewAllListener>(m_canvas->getInputManager()->getNavigationController());
    auto viewTopListener = std::make_shared<ViewTopListener>(m_canvas->getInputManager()->getNavigationController());
    auto viewFrontListener = std::make_shared<ViewFrontListener>(m_canvas->getInputManager()->getNavigationController());
    auto viewRightListener = std::make_shared<ViewRightListener>(m_canvas->getInputManager()->getNavigationController());
    auto viewIsoListener = std::make_shared<ViewIsometricListener>(m_canvas->getInputManager()->getNavigationController());
    auto showNormalsListener = std::make_shared<ShowNormalsListener>(m_occViewer);
    auto fixNormalsListener = std::make_shared<FixNormalsListener>(m_occViewer);
    auto setTransparencyListener = std::make_shared<SetTransparencyListener>(this, m_occViewer);
    auto viewModeListener = std::make_shared<ViewModeListener>(m_occViewer);
    auto showOriginalEdgesListener = std::make_shared<ShowOriginalEdgesListener>(m_occViewer);
    auto showMeshEdgesListener = std::make_shared<ShowMeshEdgesListener>(m_occViewer);
    auto showWireFrameListener = std::make_shared<ShowWireFrameListener>(m_occViewer);
    auto showFaceNormalsListener = std::make_shared<ShowFaceNormalsListener>(m_occViewer);
    auto showFeatureEdgesListener = std::make_shared<ShowFeatureEdgesListener>(m_occViewer);

    m_listenerManager->registerListener(cmd::CommandType::ViewAll, viewAllListener);
    m_listenerManager->registerListener(cmd::CommandType::ViewTop, viewTopListener);
    m_listenerManager->registerListener(cmd::CommandType::ViewFront, viewFrontListener);
    m_listenerManager->registerListener(cmd::CommandType::ViewRight, viewRightListener);
    m_listenerManager->registerListener(cmd::CommandType::ViewIsometric, viewIsoListener);
    m_listenerManager->registerListener(cmd::CommandType::ShowNormals, showNormalsListener);
    m_listenerManager->registerListener(cmd::CommandType::FixNormals, fixNormalsListener);
    m_listenerManager->registerListener(cmd::CommandType::SetTransparency, setTransparencyListener);
    m_listenerManager->registerListener(cmd::CommandType::ToggleEdges, viewModeListener);
    m_listenerManager->registerListener(cmd::CommandType::ShowOriginalEdges, showOriginalEdgesListener);
    m_listenerManager->registerListener(cmd::CommandType::ShowFeatureEdges, showFeatureEdgesListener);
    m_listenerManager->registerListener(cmd::CommandType::ShowMeshEdges, showMeshEdgesListener);
    m_listenerManager->registerListener(cmd::CommandType::ShowFaceNormals, showFaceNormalsListener);
    m_listenerManager->registerListener(cmd::CommandType::ToggleWireframe, showWireFrameListener);

    auto textureModeDecalListener = std::make_shared<TextureModeDecalListener>(this, m_occViewer);
    m_listenerManager->registerListener(cmd::CommandType::TextureModeDecal, textureModeDecalListener);
    auto textureModeModulateListener = std::make_shared<TextureModeModulateListener>(this, m_occViewer);
    m_listenerManager->registerListener(cmd::CommandType::TextureModeModulate, textureModeModulateListener);
    auto textureModeReplaceListener = std::make_shared<TextureModeReplaceListener>(this, m_occViewer);
    m_listenerManager->registerListener(cmd::CommandType::TextureModeReplace, textureModeReplaceListener);
    auto textureModeBlendListener = std::make_shared<TextureModeBlendListener>(this, m_occViewer);
    m_listenerManager->registerListener(cmd::CommandType::TextureModeBlend, textureModeBlendListener);

    auto fileNewListener = std::make_shared<FileNewListener>(m_canvas, m_commandManager);
    auto fileOpenListener = std::make_shared<FileOpenListener>(this);
    auto fileSaveListener = std::make_shared<FileSaveListener>(this);
    auto fileSaveAsListener = std::make_shared<FileSaveAsListener>(this);
    auto importStepListener = std::make_shared<ImportStepListener>(this, m_canvas, m_occViewer);
    m_listenerManager->registerListener(cmd::CommandType::FileNew, fileNewListener);
    m_listenerManager->registerListener(cmd::CommandType::FileOpen, fileOpenListener);
    m_listenerManager->registerListener(cmd::CommandType::FileSave, fileSaveListener);
    m_listenerManager->registerListener(cmd::CommandType::FileSaveAs, fileSaveAsListener);
    m_listenerManager->registerListener(cmd::CommandType::ImportSTEP, importStepListener);

    auto undoListener = std::make_shared<UndoListener>(m_commandManager, m_canvas);
    auto redoListener = std::make_shared<RedoListener>(m_commandManager, m_canvas);
    auto helpAboutListener = std::make_shared<HelpAboutListener>(this);
    auto navCubeConfigListener = std::make_shared<NavCubeConfigListener>(m_canvas);
    auto zoomSpeedListener = std::make_shared<ZoomSpeedListener>(this, m_canvas);
    auto fileExitListener = std::make_shared<FileExitListener>(this);
    auto meshQualityDialogListener = std::make_shared<MeshQualityDialogListener>(this, m_occViewer);
    auto renderingSettingsListener = std::make_shared<RenderingSettingsListener>(m_occViewer, m_canvas->getRenderingEngine());
    auto edgeSettingsListener = std::make_shared<EdgeSettingsListener>(this, m_occViewer);
    auto lightingSettingsListener = std::make_shared<LightingSettingsListener>(this);
    auto coordinateSystemVisibilityListener = std::make_shared<CoordinateSystemVisibilityListener>(this, m_canvas->getSceneManager());
    auto referenceGridToggleListener = std::make_shared<ReferenceGridToggleListener>(m_canvas->getSceneManager());
    auto chessboardGridToggleListener = std::make_shared<ChessboardGridToggleListener>(m_canvas->getSceneManager());
    auto renderPreviewSystemListener = std::make_shared<RenderPreviewSystemListener>(this);
    auto showFlatWidgetsExampleListener = std::make_shared<ShowFlatWidgetsExampleListener>(this);
    auto explodeAssemblyListener = std::make_shared<ExplodeAssemblyListener>(this, m_occViewer);
    auto sliceToggleListener = std::make_shared<SliceToggleListener>(m_occViewer);

    m_listenerManager->registerListener(cmd::CommandType::Undo, undoListener);
    m_listenerManager->registerListener(cmd::CommandType::Redo, redoListener);
    m_listenerManager->registerListener(cmd::CommandType::HelpAbout, helpAboutListener);
    m_listenerManager->registerListener(cmd::CommandType::NavCubeConfig, navCubeConfigListener);
    m_listenerManager->registerListener(cmd::CommandType::ZoomSpeed, zoomSpeedListener);
    m_listenerManager->registerListener(cmd::CommandType::FileExit, fileExitListener);
    m_listenerManager->registerListener(cmd::CommandType::MeshQualityDialog, meshQualityDialogListener);
    m_listenerManager->registerListener(cmd::CommandType::RenderingSettings, renderingSettingsListener);
    m_listenerManager->registerListener(cmd::CommandType::EdgeSettings, edgeSettingsListener);
    m_listenerManager->registerListener(cmd::CommandType::LightingSettings, lightingSettingsListener);
    m_listenerManager->registerListener(cmd::CommandType::ToggleCoordinateSystem, coordinateSystemVisibilityListener);
    m_listenerManager->registerListener(cmd::CommandType::ToggleReferenceGrid, referenceGridToggleListener);
    m_listenerManager->registerListener(cmd::CommandType::ToggleChessboardGrid, chessboardGridToggleListener);
    m_listenerManager->registerListener(cmd::CommandType::RenderPreviewSystem, renderPreviewSystemListener);
    m_listenerManager->registerListener(cmd::CommandType::ShowFlatWidgetsExample, showFlatWidgetsExampleListener);
    m_listenerManager->registerListener(cmd::CommandType::ExplodeAssembly, explodeAssemblyListener);
    m_listenerManager->registerListener(cmd::CommandType::SliceToggle, sliceToggleListener);
    auto toggleOutlineListener = std::make_shared<ToggleOutlineListener>(m_occViewer);
    m_listenerManager->registerListener(cmd::CommandType::ToggleOutline, toggleOutlineListener);

    m_commandDispatcher->setUIFeedbackHandler([this](const CommandResult& result) { this->onCommandFeedback(result); });
    LOG_INF_S("Command system setup completed");
}


