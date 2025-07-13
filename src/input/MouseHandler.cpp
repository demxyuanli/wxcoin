#include "MouseHandler.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "PickingAidManager.h"
#include "NavigationController.h"
#include "GeometryFactory.h"
#include "PositionDialog.h"
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
    LOG_INF_S("Mouse button event - Mode: " + std::to_string(static_cast<int>(m_operationMode)) + 
              ", LeftDown: " + std::to_string(event.LeftDown()));
    
    if (m_operationMode == OperationMode::VIEW && m_navigationController) {
        m_navigationController->handleMouseButton(event);
    }
    else if (m_operationMode == OperationMode::SELECT && event.LeftDown()) {
        // Handle geometry selection
        handleGeometrySelection(event);
    } else {
        event.Skip();
    }
}

void MouseHandler::handleGeometrySelection(wxMouseEvent& event) {
    if (!m_canvas) {
        event.Skip();
        return;
    }
    
    OCCViewer* viewer = m_canvas->getOCCViewer();
    
    if (!viewer) {
        event.Skip();
        return;
    }
    
    // Get mouse position
    int x = event.GetX();
    int y = event.GetY();
    
    LOG_INF_S("Attempting to select geometry at position: (" + std::to_string(x) + ", " + std::to_string(y) + ")");
    
    // Try to pick geometry at mouse position
    std::shared_ptr<OCCGeometry> pickedGeometry = viewer->pickGeometry(x, y);
    
    if (pickedGeometry) {
        LOG_INF_S("Selected geometry: " + pickedGeometry->getName());
        
        // Deselect all geometries first
        viewer->deselectAll();
        
        // Select the picked geometry
        viewer->setGeometrySelected(pickedGeometry->getName(), true);
        
        // Update ObjectTree selection
        if (m_objectTree) {
            m_objectTree->selectOCCGeometry(pickedGeometry);
            LOG_INF_S("Updated ObjectTree selection for: " + pickedGeometry->getName());
        } else {
            LOG_WRN_S("ObjectTree is null in MouseHandler");
        }
    } else {
        LOG_INF_S("No geometry picked at position");
        
        // Deselect all if clicking on empty space
        viewer->deselectAll();
        
        // Clear ObjectTree selection
        if (m_objectTree) {
            m_objectTree->updateTreeSelectionFromViewer();
            LOG_INF_S("Cleared ObjectTree selection");
        } else {
            LOG_WRN_S("ObjectTree is null in MouseHandler");
        }
        // Exit select mode
        setOperationMode(OperationMode::VIEW);
        LOG_INF_S("Exited select mode due to empty click");
    }
}

void MouseHandler::handleMouseMotion(wxMouseEvent& event) {
    if (m_operationMode == OperationMode::VIEW && m_navigationController) {
        m_navigationController->handleMouseMotion(event);
    } else {
        event.Skip();
    }
}
