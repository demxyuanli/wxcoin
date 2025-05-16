#include "MouseHandler.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "PickingAidManager.h"
#include "NavigationController.h"
#include "GeometryFactory.h"
#include "PositionDialog.h"
#include "Logger.h"
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
    LOG_INF("MouseHandler initializing");
    if (!m_canvas) LOG_ERR("MouseHandler: Canvas is null");
    if (!m_objectTree) LOG_ERR("MouseHandler: ObjectTree is null");
    if (!m_propertyPanel) LOG_ERR("MouseHandler: PropertyPanel is null");
    if (!m_commandManager) LOG_ERR("MouseHandler: CommandManager is null");
}

MouseHandler::~MouseHandler() {
    LOG_INF("MouseHandler destroying");
}

void MouseHandler::setOperationMode(OperationMode mode) {
    m_operationMode = mode;
    LOG_INF("Operation mode set to: " + std::to_string(static_cast<int>(mode)));
}

void MouseHandler::setCreationGeometryType(const std::string& type) {
    m_creationGeometryType = type;
    LOG_INF("Creation geometry type set to: " + type);

    if (!type.empty()) {
        // Create position dialog
        PositionDialog* posDialog = new PositionDialog(m_canvas->GetParent(), "Set " + wxString(type) + " Position");
        posDialog->SetPosition(SbVec3f(0.0f, 0.0f, 0.0f));
        posDialog->Show(true);
    }
}

void MouseHandler::setNavigationController(NavigationController* controller) {
    m_navigationController = controller;
    LOG_INF("NavigationController set for MouseHandler");
}

void MouseHandler::handleMouseButton(wxMouseEvent& event) {
    if (m_operationMode == OperationMode::CREATE && event.LeftDown() && g_isPickingPosition) {
        SbVec3f worldPos;
        if (m_canvas->getSceneManager()->screenToWorld(event.GetPosition(), worldPos)) {
            LOG_INF("Picked position: " + std::to_string(worldPos[0]) + ", " + std::to_string(worldPos[1]) + ", " + std::to_string(worldPos[2]));
            wxWindow* dialog = wxWindow::FindWindowByName("PositionDialog");
            if (dialog) {
                PositionDialog* posDialog = dynamic_cast<PositionDialog*>(dialog);
                if (posDialog) {
                    posDialog->SetPosition(worldPos);
                    posDialog->Show(true);
                    g_isPickingPosition = false;
                    wxButton* pickButton = dynamic_cast<wxButton*>(posDialog->FindWindow(wxID_HIGHEST + 1000));
                    if (pickButton) {
                        pickButton->SetLabel("Pick Coordinates");
                        pickButton->Enable(true);
                    }
                }
                else {
                    LOG_ERR("Failed to cast dialog to PositionDialog");
                }
            }
            else {
                LOG_ERR("PositionDialog not found");
            }
        }
        else {
            LOG_WAR("Failed to convert screen position to world coordinates");
        }
    }
    else if (m_operationMode == OperationMode::VIEW && m_navigationController) {
        m_navigationController->handleMouseButton(event);
    }
    else if (m_operationMode == OperationMode::SELECT && event.LeftDown()) {
        // Implement selection logic if needed
        LOG_INF("Selection at position: (" + std::to_string(event.GetX()) + ", " + std::to_string(event.GetY()) + ")");
    }
    event.Skip();
}

void MouseHandler::handleMouseMotion(wxMouseEvent& event) {
    if (m_operationMode == OperationMode::CREATE && g_isPickingPosition) {
        SbVec3f worldPos;
        if (m_canvas->getSceneManager()->screenToWorld(event.GetPosition(), worldPos)) {
            m_canvas->getSceneManager()->getPickingAidManager()->showPickingAidLines(worldPos);
        }
    }
    else if (m_operationMode == OperationMode::VIEW && m_navigationController) {
        m_navigationController->handleMouseMotion(event);
    }
    event.Skip();
}