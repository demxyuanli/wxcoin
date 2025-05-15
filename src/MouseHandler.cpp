#include "MouseHandler.h"
#include "Canvas.h"
#include "PropertyPanel.h"
#include "ObjectTreePanel.h"
#include "GeometryObject.h"
#include "CreateCommand.h"
#include "Logger.h"
#include "PositionDialog.h"
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/SbLinear.h>
#include <wx/event.h>


static PositionDialog* s_positionDialog = nullptr;

MouseHandler::MouseHandler(Canvas* canvas, ObjectTreePanel* objectTree, PropertyPanel* propertyPanel, CommandManager* commandManager)
    : m_canvas(canvas)
    , m_objectTree(objectTree)
    , m_propertyPanel(propertyPanel)
    , m_commandManager(commandManager)
    , m_opMode(NAVIGATE)
    , m_creationGeometryType("")
    , m_navStyle(nullptr)  // Initialize to nullptr
{
    LOG_INF("MouseHandler initializing");
    if (!m_canvas) LOG_ERR("MouseHandler: Canvas is null");
    if (!m_objectTree) LOG_ERR("MouseHandler: ObjectTree is null");
    if (!m_propertyPanel) LOG_ERR("MouseHandler: PropertyPanel is null");
    if (!m_commandManager) LOG_ERR("MouseHandler: CommandManager is null");
}

MouseHandler::~MouseHandler()
{
    LOG_INF("MouseHandler destroying");
    // Don't delete m_navStyle here as it's owned by Canvas
}

void MouseHandler::setOperationMode(OperationMode mode)
{
    LOG_INF("Setting operation mode: " + std::to_string(mode));
    m_opMode = mode;
}

void MouseHandler::setCreationGeometryType(const std::string& type) {
    LOG_INF("Setting creation geometry type: " + type);

    m_creationGeometryType = type;
    if (m_previewObject) {
        m_canvas->getObjectRoot()->removeChild(m_previewObject->getRoot());
        delete m_previewObject;
        m_previewObject = nullptr;
    }

    if (!type.empty()) {
        if (type == "Box") m_previewObject = new Box(1.0f, 1.0f, 1.0f);
        else if (type == "Sphere") m_previewObject = new Sphere(0.5f);
        else if (type == "Cylinder") m_previewObject = new Cylinder(0.5f, 1.0f);
        else if (type == "Cone") m_previewObject = new Cone(0.5f, 1.0f);
        if (m_previewObject) {
            // Initially position the object at the origin
            SbVec3f worldPos(0.0f, 0.0f, 0.0f);
            m_previewObject->setPosition(worldPos);
            m_canvas->getObjectRoot()->addChild(m_previewObject->getRoot());
            m_canvas->render();

            // Create position dialog
            if (s_positionDialog != nullptr) {
                delete s_positionDialog;
                s_positionDialog = nullptr;
            }

            s_positionDialog = new PositionDialog(m_canvas->GetParent(), "Set " + wxString(type) + " Position");
            s_positionDialog->SetPosition(worldPos);
            s_positionDialog->Show(true);
        }
    }
}

void MouseHandler::handleMouseButton(const wxMouseEvent& event) {
    LOG_INF("Handling mouse button event, mode: " + std::to_string(m_opMode));
    switch (m_opMode) {
    case NAVIGATE:
        if (m_navStyle) {
            m_navStyle->handleMouseButton(event);
        }
        else {
            LOG_WAR("Navigation style is null in NAVIGATE mode");
        }        break;
    case SELECT:
        if (event.LeftDown())
        {
            LOG_INF("Selecting object at position: (" + std::to_string(event.GetX()) + ", " + std::to_string(event.GetY()) + ")");
            selectObject(wxPoint(event.GetX(), event.GetY()));
        }        break;
    case CREATE:
        if (event.LeftDown() && g_isPickingPosition && s_positionDialog)
        {
            LOG_INF("Processing pick position in CREATE mode");

            SbVec3f worldPos;
            if (screenToWorld(event.GetPosition(), worldPos)) {
                LOG_INF("Picked position: " + std::to_string(worldPos[0]) + ", " + std::to_string(worldPos[1]) + ", " + std::to_string(worldPos[2]));

                s_positionDialog->SetPosition(worldPos);
                s_positionDialog->Show(true);
                LOG_INF("Dialog shown with updated position");
                if (m_previewObject) {
                    m_previewObject->setPosition(worldPos);
                    m_canvas->render();
                }

                g_isPickingPosition = false;

                wxButton* pickButton = (wxButton*)s_positionDialog->FindWindow(wxID_HIGHEST + 1000);
                if (pickButton) {
                    pickButton->SetLabel("Pick Coordinates");
                    pickButton->Enable(true);
                }
            }
            else {
                LOG_WAR("Failed to convert screen position to world coordinates");
            }
        }
        else if (!g_isPickingPosition && event.LeftDown()) {
            LOG_INF("Left click in CREATE mode, but not in picking mode");
        }        break;
    }    m_canvas->Refresh();
}

void MouseHandler::handleMouseMotion(const wxMouseEvent& event) { 
    if ((m_opMode == NAVIGATE || !g_isPickingPosition) && m_navStyle) {    
        m_navStyle->handleMouseMotion(event);    }  
    else if (m_opMode == CREATE && g_isPickingPosition) { 
        SbVec3f worldPos;   
        if (screenToWorld(event.GetPosition(), worldPos)) {   
            m_canvas->showPickingAidLines(worldPos);    
            m_canvas->render();   
        }  
    }
}

void MouseHandler::selectObject(const wxPoint& position)
{
    LOG_INF("TODO: Implement object selection at position: (" + std::to_string(position.x) + ", " + std::to_string(position.y) + ")");
}

void MouseHandler::createObject(const wxPoint& position) {
    if (m_previewObject) {
        m_canvas->getObjectRoot()->removeChild(m_previewObject->getRoot());
        delete m_previewObject;
        m_previewObject = nullptr;
    }

    if (!m_canvas) {
        LOG_ERR("createObject: Canvas is null");
        return;
    }
    if (!m_objectTree) {
        LOG_ERR("createObject: ObjectTree is null");
        return;
    }
    if (!m_propertyPanel) {
        LOG_ERR("createObject: PropertyPanel is null");
        return;
    }
    if (!m_commandManager) {
        LOG_ERR("createObject: CommandManager is null");
        return;
    }

    LOG_INF("Create geometry: " + m_creationGeometryType);

    SbVec3f worldPos;
    if (!screenToWorld(position, worldPos))
    {
        LOG_WAR("Cant transfer Screen pos to world pos");
        return;
    }
    m_canvas->render();

    createGeometryAtPosition(worldPos);
}

void MouseHandler::createGeometryAtPosition(const SbVec3f& position) {
    GeometryObject* object = nullptr;
    std::string objectName = m_creationGeometryType + "_" + std::to_string(std::time(nullptr));

    if (m_creationGeometryType == "Box")
    {
        object = new Box(1.0f, 1.0f, 1.0f);
    }
    else if (m_creationGeometryType == "Sphere")
    {
        object = new Sphere(0.5f);
    }
    else if (m_creationGeometryType == "Cylinder")
    {
        object = new Cylinder(0.5f, 1.0f);
    }
    else if (m_creationGeometryType == "Cone")
    {
        object = new Cone(0.5f, 1.0f);
    }
    else
    {
        LOG_ERR("Unknown geometry type: " + m_creationGeometryType);
        return;
    }

    if (!object) {
        LOG_ERR("Failed to create object of type: " + m_creationGeometryType);
        return;
    }

    object->setPosition(position);
    LOG_INF("Created object: " + objectName + " at position: (" + std::to_string(position[0]) + ", " + std::to_string(position[1]) + ", " + std::to_string(position[2]) + ")");

    auto command = std::make_shared<CreateCommand>(object, m_canvas->getObjectRoot(), m_objectTree, m_propertyPanel);
    m_commandManager->executeCommand(command);

    m_canvas->Refresh();
    setOperationMode(NAVIGATE); // Reset to navigate mode
    setCreationGeometryType(""); // Clear geometry type
}

bool MouseHandler::screenToWorld(const wxPoint& screenPos, SbVec3f& worldPos)
{
    wxSize size = m_canvas->GetClientSize();
    if (size.x <= 0 || size.y <= 0) {
        LOG_ERR("Invalid viewport size");
        return false;
    }

    SoCamera* camera = m_canvas->getCamera();
    if (!camera) {
        LOG_ERR("Camera is null");
        return false;
    }

    // Calculate normalized device coordinates (range -1 to 1)
    float normX = (2.0f * screenPos.x) / size.x - 1.0f;
    float normY = 1.0f - (2.0f * screenPos.y) / size.y;

    // Get view volume
    SbViewVolume viewVolume = camera->getViewVolume();

    // Create ray from screen coordinates
    SbLine line;
    viewVolume.projectPointToLine(SbVec2f(normX, normY), line);

    // Get ray origin and direction
    SbVec3f linePos = line.getPosition();
    SbVec3f lineDir = line.getDirection();

    // Calculate intersection with Z=0 plane
    if (std::abs(lineDir[2]) < 1e-6) {
        // Ray almost parallel to XY plane, use origin as default point
        worldPos.setValue(0.0f, 0.0f, 0.0f);
        LOG_WAR("Ray almost parallel to XY plane, using origin");
        return true;
    }

    float t = -linePos[2] / lineDir[2];

    // Calculate intersection point
    worldPos = linePos + lineDir * t;

    // Removed maxRadius clamping to allow free movement based on mouse projection

    // If intersection point is behind viewpoint or too far, adjust to reasonable position
    if (t < 0 || t > 100.0f) {
        // Use point in front of camera
        SbVec3f cameraPos = camera->position.getValue();
        SbVec3f forward;
        camera->orientation.getValue().multVec(SbVec3f(0, 0, -1), forward);
        forward.normalize();

        // Place at reasonable distance in front of camera
        worldPos = cameraPos + forward * 2.0f;
        worldPos[2] = 0.0f; // Force on XY plane
    }

    // Ensure Z value is 0
    worldPos[2] = 0.0f;
    return true;
}