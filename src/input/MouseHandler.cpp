#include "MouseHandler.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "PickingAidManager.h"
#include "NavigationController.h"
#include "GeometryFactory.h"
#include "PositionDialog.h"
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
{
    LOG_INF_S("MouseHandler initializing");
    if (!m_canvas) LOG_ERR_S("MouseHandler: Canvas is null");
    if (!m_objectTree) LOG_ERR_S("MouseHandler: ObjectTree is null");
    if (!m_propertyPanel) LOG_ERR_S("MouseHandler: PropertyPanel is null");
    if (!m_commandManager) LOG_ERR_S("MouseHandler: CommandManager is null");
}

MouseHandler::~MouseHandler() {
    LOG_INF_S("MouseHandler destroying");
}

void MouseHandler::setOperationMode(OperationMode mode) {
    m_operationMode = mode;
    LOG_INF_S("Operation mode set to: " + std::to_string(static_cast<int>(mode)));
}

void MouseHandler::setCreationGeometryType(const std::string& type) {
    m_creationGeometryType = type;
    LOG_INF_S("Creation geometry type set to: " + type);

    if (!type.empty()) {
        // Create position dialog and pass the PickingAidManager
        PickingAidManager* pickingAidManager = m_canvas->getSceneManager()->getPickingAidManager();
        PositionDialog* posDialog = new PositionDialog(m_canvas->GetParent(), "Set " + wxString(type) + " Position", pickingAidManager);
        posDialog->SetPosition(SbVec3f(0.0f, 0.0f, 0.0f));
        posDialog->Show(true);
    }
}

void MouseHandler::setNavigationController(NavigationController* controller) {
    m_navigationController = controller;
    LOG_INF_S("NavigationController set for MouseHandler");
}

void MouseHandler::handleMouseButton(wxMouseEvent& event) {
    if (m_operationMode == OperationMode::VIEW && m_navigationController) {
        m_navigationController->handleMouseButton(event);
    }
    else if (m_operationMode == OperationMode::SELECT && event.LeftDown()) {
        // Implement selection logic if needed
        LOG_INF_S("Selection at position: (" + std::to_string(event.GetX()) + ", " + std::to_string(event.GetY()) + ")");
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
