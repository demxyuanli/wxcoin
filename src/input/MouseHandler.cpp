#include "MouseHandler.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "PickingAidManager.h"
#include "NavigationController.h"
#include "GeometryFactory.h"
#include "PositionBasicDialog.h"
#include "OCCViewer.h"
#include "OCCGeometry.h"
#include "ObjectTreePanel.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoSelection.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/SoPickedPoint.h>

MouseHandler::MouseHandler(Canvas* canvas, ObjectTreePanel* objectTree, PropertyPanel* propertyPanel, CommandManager* commandManager)
    : m_canvas(canvas)
    , m_objectTree(objectTree)
    , m_propertyPanel(propertyPanel)
    , m_commandManager(commandManager)
    , m_navigationController(nullptr)
    , m_operationMode(OperationMode::VIEW)
    , m_isDragging(false)
    , m_currentPositionBasicDialog(nullptr)
{
    LOG_INF_S("MouseHandler initializing");
    if (!m_canvas) LOG_ERR_S("MouseHandler: Canvas is null");
    if (!m_objectTree) LOG_ERR_S("MouseHandler: ObjectTree is null");
    if (!m_propertyPanel) LOG_ERR_S("MouseHandler: PropertyPanel is null");
    if (!m_commandManager) LOG_ERR_S("MouseHandler: CommandManager is null");
}

MouseHandler::~MouseHandler() {
    LOG_INF_S("MouseHandler destroying");
    if (m_currentPositionBasicDialog) {
        m_currentPositionBasicDialog->Destroy();
        m_currentPositionBasicDialog = nullptr;
    }
}

void MouseHandler::setOperationMode(OperationMode mode) {
    m_operationMode = mode;
    LOG_INF_S("Operation mode set to: " + std::to_string(static_cast<int>(mode)));
}

void MouseHandler::setCreationGeometryType(const std::string& type) {
    m_creationGeometryType = type;
    LOG_INF_S("Creation geometry type set to: " + type);

    if (!type.empty()) {
        // Close existing position dialog if any
        if (m_currentPositionBasicDialog) {
            m_currentPositionBasicDialog->Destroy();
            m_currentPositionBasicDialog = nullptr;
        }
        
        // Create new position basic dialog
        PickingAidManager* pickingAidManager = m_canvas->getSceneManager()->getPickingAidManager();
        m_currentPositionBasicDialog = new PositionBasicDialog(m_canvas->GetParent(), "Set " + wxString(type) + " Position", pickingAidManager, type);
        m_currentPositionBasicDialog->SetPosition(SbVec3f(0.0f, 0.0f, 0.0f));
        
        // Set up picking callback - this will be called when picking is complete
        m_currentPositionBasicDialog->SetPickingCallback([this](const SbVec3f& position) {
            // Just log the completion, don't call OnPositionPicked again
            LOG_INF_S("Position picking completed via callback: X=" + std::to_string(position[0]) + 
                      ", Y=" + std::to_string(position[1]) + 
                      ", Z=" + std::to_string(position[2]));
        });
        
        m_currentPositionBasicDialog->Show(true);
    }
}

void MouseHandler::setNavigationController(NavigationController* controller) {
    m_navigationController = controller;
    LOG_INF_S("NavigationController set for MouseHandler");
}

void MouseHandler::handleMouseButton(wxMouseEvent& event) {
    LOG_INF_S("Mouse button event - Mode: " + std::to_string(static_cast<int>(m_operationMode)) + 
              ", LeftDown: " + std::to_string(event.LeftDown()));
    
    if (m_operationMode == OperationMode::VIEW && m_navigationController) {
        m_navigationController->handleMouseButton(event);
    } else {
        event.Skip();
    }
}



void MouseHandler::handleMouseMotion(wxMouseEvent& event) {
    if (m_operationMode == OperationMode::VIEW && m_navigationController) {
        m_navigationController->handleMouseMotion(event);
    } else {
        event.Skip();
    }
}

void MouseHandler::OnPositionPicked(const SbVec3f& position)
{
    if (m_currentPositionBasicDialog) {
        // Update the dialog with the picked position
        m_currentPositionBasicDialog->OnPickingComplete(position);
        
        // Stop picking mode
        if (m_canvas->getSceneManager()->getPickingAidManager()) {
            m_canvas->getSceneManager()->getPickingAidManager()->stopPicking();
        }
    }
}
