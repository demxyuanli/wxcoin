#include "MouseHandler.h"
#include "Canvas.h"
#include "PropertyPanel.h"
#include "ObjectTreePanel.h"
#include "GeometryObject.h"
#include "CreateCommand.h"
#include "Logger.h"
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/SbLinear.h>
#include <wx/event.h>

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
        }
    }
}

void MouseHandler::handleMouseButton(const wxMouseEvent& event)
{
    LOG_INF("Handling mouse button event, mode: " + std::to_string(m_opMode));
    switch (m_opMode)
    {
    case NAVIGATE:
        if (m_navStyle) {
            m_navStyle->handleMouseButton(event);
        }
        break;
    case SELECT:
        if (event.LeftDown())
        {
            LOG_INF("Selecting object at position: (" + std::to_string(event.GetX()) + ", " + std::to_string(event.GetY()) + ")");
            selectObject(wxPoint(event.GetX(), event.GetY()));
        }
        break;
    case CREATE:
        if (event.LeftDown())
        {
            wxPoint pos = event.GetPosition();
            LOG_INF("try to create: " + std::to_string(pos.x) + "," + std::to_string(pos.y));
            createObject(pos);
        }
        break;
    }
    m_canvas->Refresh();
}

void MouseHandler::handleMouseMotion(const wxMouseEvent& event)
{
    if (m_opMode == NAVIGATE && m_navStyle) {
        m_navStyle->handleMouseMotion(event);
    }
    else if (m_opMode == CREATE && m_previewObject) {
        SbVec3f worldPos;
        if (screenToWorld(event.GetPosition(), worldPos)) {
            // Check if mouse is within the geometry bounds
            // Since we don't have direct access to dimensions, use a reasonable default radius
            float radius = 1.0f; // Default value for all geometries
            
            float distance = sqrt(pow(worldPos[0], 2) + pow(worldPos[1], 2));
            if (distance <= radius * 1.5f) { // Allow some margin
                m_previewObject->setPosition(worldPos);
                m_canvas->render();
            }
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

    object->setPosition(worldPos);
    LOG_INF("Created object: " + objectName + " at position: (" + std::to_string(worldPos[0]) + ", " + std::to_string(worldPos[1]) + ", " + std::to_string(worldPos[2]) + ")");

    auto command = std::make_shared<CreateCommand>(object, m_canvas->getObjectRoot(), m_objectTree, m_propertyPanel);
    m_commandManager->executeCommand(command);

    m_canvas->Refresh();
    setOperationMode(NAVIGATE); // Reset to navigate mode
    setCreationGeometryType(""); // Clear geometry type
}

bool MouseHandler::screenToWorld(const wxPoint& screenPos, SbVec3f& worldPos)
{
    LOG_DBG("Converting screen position: (" + std::to_string(screenPos.x) + ", " + std::to_string(screenPos.y) + ")");
    
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
    
    // Use very small radius limit to make objects follow mouse closely
    float maxRadius = 1.0f; // 进一步减小值以确保对象更紧密地跟随鼠标
    float currRadius = sqrt(worldPos[0]*worldPos[0] + worldPos[1]*worldPos[1]);
    
    if (currRadius > maxRadius && currRadius > 0) {
        float scale = maxRadius / currRadius;
        worldPos[0] *= scale;
        worldPos[1] *= scale;
    }
    
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
    
    LOG_DBG("Converted to world position: (" + 
        std::to_string(worldPos[0]) + ", " + 
        std::to_string(worldPos[1]) + ", " + 
        std::to_string(worldPos[2]) + ")");
    
    return true;
}