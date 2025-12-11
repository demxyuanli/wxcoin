#include "InventorNavigationController.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "config/ConfigManager.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/SbRotation.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/SoPickedPoint.h>
#include <cmath>
#include <algorithm>

InventorNavigationController::InventorNavigationController(Canvas* canvas, SceneManager* sceneManager)
    : m_canvas(canvas)
    , m_sceneManager(sceneManager)
    , m_currentMode(InventorNavigationMode::IDLE)
    , m_button1Down(false)
    , m_button2Down(false)
    , m_button3Down(false)
    , m_ctrlDown(false)
    , m_shiftDown(false)
    , m_altDown(false)
    , m_isDragging(false)
    , m_hasDragged(false)
    , m_hasPanned(false)
    , m_hasZoomed(false)
    , m_lockRecenter(false)
    , m_zoomSpeedFactor(1.0f)
    , m_hasRotationCenter(false)
    , m_rotationCenterMarker(nullptr)
    , m_isPotentialClick(false)
{
    LOG_DBG_S("InventorNavigationController initializing");
    m_centerTime = wxGetLocalTimeMillis();
    m_lastMotionTime = wxGetLocalTimeMillis();

    // Load marker configuration
    loadMarkerConfig();

    // Initialize rotation center marker
    createRotationCenterMarker();
}

InventorNavigationController::~InventorNavigationController() {
    LOG_DBG_S("InventorNavigationController destroying");

    // Clean up rotation center marker
    if (m_rotationCenterMarker) {
        hideRotationCenterMarker();
        m_rotationCenterMarker->unref();
        m_rotationCenterMarker = nullptr;
    }
}

void InventorNavigationController::handleMouseButton(wxMouseEvent& event) {
    wxPoint pos = event.GetPosition();
    
    // Update modifier key states
    m_ctrlDown = event.ControlDown();
    m_shiftDown = event.ShiftDown();
    m_altDown = event.AltDown();
    
    bool processed = false;
    
    if (event.LeftDown()) {
        m_button1Down = true;
        m_clickStartPos = pos;
        m_isPotentialClick = true;

        // Immediately pick rotation center when left button is pressed
        pickRotationCenterAtMouse(pos);

        if (event.ShiftDown() && m_currentMode != InventorNavigationMode::SELECTION) {
            m_centerTime = wxGetLocalTimeMillis();
            setupPanningPlane();
            m_lockRecenter = false;
        }
        else if (m_currentMode == InventorNavigationMode::IDLE) {
            // Don't start dragging immediately - wait to see if it's a click or drag
            processed = true;
            m_lockRecenter = true;
        }
        else {
            processed = true;
        }
    }
    else if (event.LeftUp()) {
        m_button1Down = false;

        // Clear rotation center when left button is released
        clearRotationCenter();
        LOG_DBG_S("Cleared rotation center on mouse release");

        // Check if this was a click (not a drag)
        if (m_isPotentialClick) {
            int dx = abs(pos.x - m_clickStartPos.x);
            int dy = abs(pos.y - m_clickStartPos.y);
            int clickThreshold = 5; // pixels

            if (dx <= clickThreshold && dy <= clickThreshold) {
                // This was a click - but we already picked rotation center on mouse down
                processed = true;
            }
            m_isPotentialClick = false;
        }

        if (!event.ShiftDown() && m_currentMode != InventorNavigationMode::SELECTION) {
            wxLongLong tmp = wxGetLocalTimeMillis() - m_centerTime;
            float dci = 500.0f; // Double click interval in ms
            if (tmp.GetValue() < dci && !m_lockRecenter) {
                lookAtPoint(pos);
                processed = true;
            }
        }
        else if (m_currentMode == InventorNavigationMode::DRAGGING) {
            m_isDragging = false;
            if (doSpin()) {
                // Continue spinning after drag ends
                LOG_DBG_S("Starting spin continuation");
            }
            processed = true;
            m_lockRecenter = true;
        }
        else {
            processed = true;
        }
    }
    else if (event.RightDown()) {
        m_button2Down = true;
        m_lockRecenter = true;

        if (!m_hasDragged && !m_hasPanned && !m_hasZoomed) {
            // Show context menu on right click
            LOG_DBG_S("Right click - could show context menu");
        }
        processed = true;
    }
    else if (event.RightUp()) {
        m_button2Down = false;
        processed = true;
    }
    else if (event.MiddleDown()) {
        m_button3Down = true;
        m_centerTime = wxGetLocalTimeMillis();
        setupPanningPlane();
        m_lockRecenter = false;
        processed = true;
    }
    else if (event.MiddleUp()) {
        m_button3Down = false;
        wxLongLong tmp = wxGetLocalTimeMillis() - m_centerTime;
        float dci = 500.0f; // Double click interval in ms
        if (tmp.GetValue() < dci && !m_lockRecenter) {
            lookAtPoint(pos);
            processed = true;
        }
    }
    
    updateNavigationMode();
    
    if (!processed) {
        event.Skip();
    }
}

void InventorNavigationController::handleMouseMotion(wxMouseEvent& event) {
    m_lockRecenter = true;

    wxPoint currentPos = event.GetPosition();
    wxLongLong currentTime = wxGetLocalTimeMillis();

    // Check if this should start dragging instead of being a click
    if (m_isPotentialClick && m_button1Down && m_currentMode == InventorNavigationMode::IDLE) {
        int dx = abs(currentPos.x - m_clickStartPos.x);
        int dy = abs(currentPos.y - m_clickStartPos.y);
        int dragThreshold = 5; // pixels

        if (dx > dragThreshold || dy > dragThreshold) {
            // Start dragging
            m_isDragging = true;
            m_baseMousePos = m_clickStartPos;
            m_lastMousePos = m_clickStartPos;
            m_currentMode = InventorNavigationMode::DRAGGING;
            m_isPotentialClick = false;
            LOG_DBG_S("Started dragging from potential click");
        }
    }

    // Throttle motion events to improve performance
    if (currentTime - m_lastMotionTime < 10) {
        event.Skip();
        return;
    }
    m_lastMotionTime = currentTime;

    bool processed = false;
    
    switch (m_currentMode) {
    case InventorNavigationMode::ZOOMING:
        zoomByCursor(currentPos, m_lastMousePos);
        processed = true;
        break;
        
    case InventorNavigationMode::PANNING:
        panCamera(currentPos, m_lastMousePos);
        processed = true;
        break;
        
    case InventorNavigationMode::DRAGGING:
        addToLog(currentPos, currentTime);
        spin(currentPos, m_lastMousePos);
        moveCursorPosition();
        processed = true;
        break;
        
    default:
        break;
    }
    
    m_lastMousePos = currentPos;
    m_canvas->Refresh();
    
    if (!processed) {
        event.Skip();
    }
}

void InventorNavigationController::handleMouseWheel(wxMouseEvent& event) {
    float delta = event.GetWheelRotation() / 120.0f;
    zoomCamera(delta);
    m_hasZoomed = true;
    m_canvas->Refresh();
    event.Skip();
}

void InventorNavigationController::updateNavigationMode() {
    enum {
        BUTTON1DOWN = 1 << 0,
        BUTTON3DOWN = 1 << 1,
        CTRLDOWN = 1 << 2,
        SHIFTDOWN = 1 << 3,
        BUTTON2DOWN = 1 << 4
    };
    
    unsigned int combo = 0;
    if (m_button1Down) combo |= BUTTON1DOWN;
    if (m_button2Down) combo |= BUTTON2DOWN;
    if (m_button3Down) combo |= BUTTON3DOWN;
    if (m_ctrlDown) combo |= CTRLDOWN;
    if (m_shiftDown) combo |= SHIFTDOWN;
    
    InventorNavigationMode newMode = m_currentMode;
    
    switch (combo) {
    case 0:
        if (m_currentMode == InventorNavigationMode::DRAGGING) {
            if (doSpin()) {
                newMode = InventorNavigationMode::IDLE; // Will be set to spinning by doSpin
            }
        }
        else {
            newMode = InventorNavigationMode::IDLE;
        }
        break;
        
    case BUTTON1DOWN:
        if (m_currentMode != InventorNavigationMode::SELECTION) {
            newMode = InventorNavigationMode::DRAGGING;
        }
        break;
        
    case BUTTON3DOWN:
    case CTRLDOWN | SHIFTDOWN:
    case CTRLDOWN | SHIFTDOWN | BUTTON1DOWN:
        newMode = InventorNavigationMode::PANNING;
        break;
        
    case CTRLDOWN:
    case CTRLDOWN | BUTTON1DOWN:
    case SHIFTDOWN:
    case SHIFTDOWN | BUTTON1DOWN:
        newMode = InventorNavigationMode::SELECTION;
        break;
        
    case BUTTON1DOWN | BUTTON3DOWN:
    case CTRLDOWN | BUTTON3DOWN:
    case CTRLDOWN | SHIFTDOWN | BUTTON2DOWN:
        newMode = InventorNavigationMode::ZOOMING;
        break;
        
    default:
        break;
    }
    
    if (newMode != m_currentMode) {
        m_currentMode = newMode;
        LOG_DBG_S("Navigation mode changed to: " + std::to_string(static_cast<int>(newMode)));
    }
    
    // Reset flags when returning to IDLE
    if (newMode == InventorNavigationMode::IDLE && !m_button1Down && !m_button2Down && !m_button3Down) {
        m_hasPanned = false;
        m_hasDragged = false;
        m_hasZoomed = false;
    }
}

void InventorNavigationController::rotateCamera(const wxPoint& currentPos, const wxPoint& lastPos) {
    SoCamera* camera = m_sceneManager->getCamera();
    if (!camera) {
        LOG_ERR_S("Cannot rotate: Invalid camera");
        return;
    }

    float dx = static_cast<float>(currentPos.x - lastPos.x) / 100.0f;
    float dy = static_cast<float>(currentPos.y - lastPos.y) / 100.0f;

    SbVec3f cameraPos = camera->position.getValue();

    // Use rotation center if set, otherwise use origin (0,0,0)
    SbVec3f center = m_hasRotationCenter ? m_rotationCenter : SbVec3f(0, 0, 0);

    // Calculate vector from center to camera
    SbVec3f offset = cameraPos - center;
    float distance = offset.length();

    if (distance < 0.001f) {
        distance = 0.001f;
    }

    // Convert to spherical coordinates relative to center
    float theta = std::atan2(offset[1], offset[0]);
    float phi = std::acos(offset[2] / distance);

    // Apply rotation deltas
    theta -= dx;
    phi += dy;

    // Clamp phi to avoid singularities
    if (phi < 0.001f) phi = 0.001f;
    if (phi > M_PI - 0.001f) phi = M_PI - 0.001f;

    // Convert back to Cartesian coordinates
    float x = distance * std::sin(phi) * std::cos(theta);
    float y = distance * std::sin(phi) * std::sin(theta);
    float z = distance * std::cos(phi);

    // Set new camera position relative to center
    SbVec3f newCameraPos = center + SbVec3f(x, y, z);
    camera->position.setValue(newCameraPos);

    // Calculate view direction (from camera to center)
    SbVec3f viewDir = center - newCameraPos;
    viewDir.normalize();

    // Create rotation to look at center
    SbVec3f defaultDir(0, 0, -1);
    SbRotation newOrientation(defaultDir, viewDir);
    camera->orientation.setValue(newOrientation);

    LOG_DBG_S("Rotated camera around center (" + std::to_string(center[0]) + "," +
              std::to_string(center[1]) + "," + std::to_string(center[2]) + ") to position (" +
              std::to_string(newCameraPos[0]) + "," + std::to_string(newCameraPos[1]) + "," +
              std::to_string(newCameraPos[2]) + ")");
}

void InventorNavigationController::panCamera(const wxPoint& currentPos, const wxPoint& lastPos) {
    SoCamera* camera = m_sceneManager->getCamera();
    if (!camera) {
        LOG_ERR_S("Cannot pan: Invalid camera");
        return;
    }
    
    float dx = static_cast<float>(lastPos.x - currentPos.x) / 100.0f;
    float dy = static_cast<float>(currentPos.y - lastPos.y) / 100.0f;
    
    SbVec3f position = camera->position.getValue();
    SbVec3f right, up;
    camera->orientation.getValue().multVec(SbVec3f(1, 0, 0), right);
    camera->orientation.getValue().multVec(SbVec3f(0, 1, 0), up);
    position += right * dx + up * dy;
    camera->position.setValue(position);
    
    m_hasPanned = true;
}

void InventorNavigationController::zoomCamera(float delta) {
    SoCamera* camera = m_sceneManager->getCamera();
    if (!camera) {
        LOG_ERR_S("Cannot zoom: Invalid camera");
        return;
    }
    
    SbVec3f position = camera->position.getValue();
    SbVec3f forward;
    camera->orientation.getValue().multVec(SbVec3f(0, 0, -1), forward);
    
    // Adaptive zoom based on scene size
    float sceneSize = m_sceneManager->getSceneBoundingBoxSize();
    float zoomFactor = sceneSize / 100.0f;
    
    position += forward * delta * zoomFactor * m_zoomSpeedFactor;
    camera->position.setValue(position);
}

void InventorNavigationController::zoomByCursor(const wxPoint& currentPos, const wxPoint& lastPos) {
    // Zoom towards cursor position
    float dy = static_cast<float>(currentPos.y - lastPos.y) / 100.0f;
    zoomCamera(dy);
    m_hasZoomed = true;
}

void InventorNavigationController::setupPanningPlane() {
    // Set up panning plane for camera operations
    // This is a simplified implementation
    LOG_DBG_S("Setting up panning plane");
}

void InventorNavigationController::lookAtPoint(const wxPoint& pos) {
    // Look at the point under cursor
    // This would typically involve projecting screen coordinates to world coordinates
    LOG_DBG_S("Looking at point: " + std::to_string(pos.x) + ", " + std::to_string(pos.y));
}

void InventorNavigationController::spin(const wxPoint& currentPos, const wxPoint& lastPos) {
    rotateCamera(currentPos, lastPos);
    m_hasDragged = true;
}

bool InventorNavigationController::doSpin() {
    // Check if we should continue spinning based on movement history
    if (m_movementLog.size() < 2) {
        return false;
    }
    
    // Calculate average velocity from recent movements
    float avgVelocity = 0.0f;
    for (size_t i = 1; i < m_movementLog.size(); ++i) {
        const auto& current = m_movementLog[i];
        const auto& previous = m_movementLog[i-1];
        
        float dx = static_cast<float>(current.position.x - previous.position.x);
        float dy = static_cast<float>(current.position.y - previous.position.y);
        float velocity = std::sqrt(dx*dx + dy*dy);
        avgVelocity += velocity;
    }
    avgVelocity /= (m_movementLog.size() - 1);
    
    // If velocity is high enough, continue spinning
    return avgVelocity > 2.0f;
}

void InventorNavigationController::addToLog(const wxPoint& pos, wxLongLong time) {
    MovementLog log;
    log.position = pos;
    log.timestamp = time;
    
    m_movementLog.push_back(log);
    if (m_movementLog.size() > MAX_MOVEMENT_LOG_SIZE) {
        m_movementLog.erase(m_movementLog.begin());
    }
}

void InventorNavigationController::moveCursorPosition() {
    // Move cursor to maintain relative position
    // This is a simplified implementation
}

void InventorNavigationController::viewAll() {
    m_sceneManager->resetView();
}

void InventorNavigationController::viewTop() {
    SoCamera* camera = m_sceneManager->getCamera();
    if (!camera) {
        LOG_ERR_S("Cannot set top view: Invalid camera");
        return;
    }
    m_sceneManager->setView("Top");
}

void InventorNavigationController::viewFront() {
    SoCamera* camera = m_sceneManager->getCamera();
    if (!camera) {
        LOG_ERR_S("Cannot set front view: Invalid camera");
        return;
    }
    m_sceneManager->setView("Front");
}

void InventorNavigationController::viewRight() {
    SoCamera* camera = m_sceneManager->getCamera();
    if (!camera) {
        LOG_ERR_S("Cannot set right view: Invalid camera");
        return;
    }
    m_sceneManager->setView("Right");
}

void InventorNavigationController::viewIsometric() {
    SoCamera* camera = m_sceneManager->getCamera();
    if (!camera) {
        LOG_ERR_S("Cannot set isometric view: Invalid camera");
        return;
    }
    m_sceneManager->setView("Isometric");
}

void InventorNavigationController::setNavigationMode(InventorNavigationMode mode) {
    m_currentMode = mode;
    LOG_DBG_S("Navigation mode set to: " + std::to_string(static_cast<int>(mode)));
}

InventorNavigationMode InventorNavigationController::getNavigationMode() const {
    return m_currentMode;
}

void InventorNavigationController::setZoomSpeedFactor(float factor) {
    m_zoomSpeedFactor = factor;
}

float InventorNavigationController::getZoomSpeedFactor() const {
    return m_zoomSpeedFactor;
}

// Rotation center management methods
void InventorNavigationController::setRotationCenter(const SbVec3f& center) {
    m_rotationCenter = center;
    m_hasRotationCenter = true;
    updateRotationCenterMarker();
    LOG_DBG_S("Rotation center set to: (" + std::to_string(center[0]) + ", " +
              std::to_string(center[1]) + ", " + std::to_string(center[2]) + ")");
}

void InventorNavigationController::clearRotationCenter() {
    m_hasRotationCenter = false;
    hideRotationCenterMarker();
    LOG_DBG_S("Rotation center cleared");
}

bool InventorNavigationController::hasRotationCenter() const {
    return m_hasRotationCenter;
}

const SbVec3f& InventorNavigationController::getRotationCenter() const {
    return m_rotationCenter;
}

void InventorNavigationController::pickRotationCenterAtMouse(const wxPoint& mousePos) {
    if (!m_canvas || !m_sceneManager || !m_sceneManager->getSceneRoot()) {
        LOG_DBG_S("Cannot pick rotation center: Canvas or scene not available");
        return;
    }

    // Create viewport region for picking
    wxSize canvasSize = m_canvas->GetClientSize();
    SbViewportRegion viewport(canvasSize.x, canvasSize.y);

    // Create ray pick action
    SoRayPickAction rayPick(viewport);
    rayPick.setPoint(SbVec2s(mousePos.x, mousePos.y));
    rayPick.setRadius(8.0f); // Small radius for precise picking

    // Apply to scene root
    rayPick.apply(m_sceneManager->getSceneRoot());

    // Get picked point
    SoPickedPoint* pickedPoint = rayPick.getPickedPoint();
    if (pickedPoint) {
        SbVec3f worldPoint = pickedPoint->getPoint();
        setRotationCenter(worldPoint);
        LOG_DBG_S("Picked rotation center at geometry: (" +
                  std::to_string(mousePos.x) + ", " + std::to_string(mousePos.y) + ") -> world: (" +
                  std::to_string(worldPoint[0]) + ", " + std::to_string(worldPoint[1]) + ", " +
                  std::to_string(worldPoint[2]) + ")");
    } else {
        // If no geometry picked, create rotation center on view plane
        SbVec3f viewPlanePoint = getPointOnViewPlane(mousePos);
        setRotationCenter(viewPlanePoint);
        LOG_DBG_S("Picked rotation center on view plane: (" +
                  std::to_string(mousePos.x) + ", " + std::to_string(mousePos.y) + ") -> world: (" +
                  std::to_string(viewPlanePoint[0]) + ", " + std::to_string(viewPlanePoint[1]) + ", " +
                  std::to_string(viewPlanePoint[2]) + ")");
    }
}

void InventorNavigationController::createRotationCenterMarker() {
    if (m_rotationCenterMarker) {
        return; // Already created
    }

    m_rotationCenterMarker = new SoSeparator;

    // Create material for marker (configurable color and transparency)
    SoMaterial* material = new SoMaterial;
    material->diffuseColor.setValue(SbVec3f(m_markerConfig.red, m_markerConfig.green, m_markerConfig.blue));
    material->emissiveColor.setValue(SbVec3f(m_markerConfig.red * 0.5f, m_markerConfig.green * 0.5f, m_markerConfig.blue * 0.5f));
    material->transparency.setValue(m_markerConfig.transparency);
    m_rotationCenterMarker->addChild(material);

    // Create transform for positioning
    SoTransform* transform = new SoTransform;
    m_rotationCenterMarker->addChild(transform);

    // Create sphere for center marker (configurable size)
    SoSphere* sphere = new SoSphere;
    sphere->radius.setValue(m_markerConfig.radius);
    m_rotationCenterMarker->addChild(sphere);

    // Keep a reference to prevent deletion
    m_rotationCenterMarker->ref();

    // Initially hide the marker (don't add to scene yet)
}

void InventorNavigationController::updateRotationCenterMarker() {
    if (!m_rotationCenterMarker || !m_hasRotationCenter) {
        return;
    }

    // Update transform to position marker at rotation center
    SoTransform* transform = static_cast<SoTransform*>(m_rotationCenterMarker->getChild(1));
    if (transform) {
        transform->translation.setValue(m_rotationCenter);
    }

    // Show marker - add to scene if not already there
    if (m_sceneManager && m_sceneManager->getSceneRoot()) {
        SoGroup* sceneRoot = m_sceneManager->getSceneRoot();
        // Check if marker is already a child
        bool isAlreadyChild = false;
        for (int i = 0; i < sceneRoot->getNumChildren(); ++i) {
            if (sceneRoot->getChild(i) == m_rotationCenterMarker) {
                isAlreadyChild = true;
                break;
            }
        }
        if (!isAlreadyChild) {
            sceneRoot->addChild(m_rotationCenterMarker);
        }
    }
}

void InventorNavigationController::hideRotationCenterMarker() {
    if (!m_rotationCenterMarker) {
        return;
    }

    // Remove marker from scene
    if (m_sceneManager && m_sceneManager->getSceneRoot()) {
        m_sceneManager->getSceneRoot()->removeChild(m_rotationCenterMarker);
    }
}

SbVec3f InventorNavigationController::getPointOnViewPlane(const wxPoint& mousePos) {
    if (!m_canvas || !m_sceneManager) {
        return SbVec3f(0, 0, 0);
    }

    SoCamera* camera = m_sceneManager->getCamera();
    if (!camera) {
        return SbVec3f(0, 0, 0);
    }

    // Get canvas size
    wxSize canvasSize = m_canvas->GetClientSize();
    if (canvasSize.x == 0 || canvasSize.y == 0) {
        return SbVec3f(0, 0, 0);
    }

    // Convert mouse position to normalized coordinates (0 to 1)
    float normalizedX = static_cast<float>(mousePos.x) / canvasSize.x;
    float normalizedY = static_cast<float>(mousePos.y) / canvasSize.y;

    // Get camera position and orientation
    SbVec3f cameraPos = camera->position.getValue();
    SbRotation cameraRot = camera->orientation.getValue();

    // Get camera coordinate system
    SbVec3f viewDir(0, 0, -1);
    cameraRot.multVec(viewDir, viewDir);
    viewDir.normalize();

    SbVec3f upDir(0, 1, 0);
    cameraRot.multVec(upDir, upDir);
    SbVec3f rightDir = upDir.cross(viewDir);
    rightDir.normalize();

    // Use camera's focal distance as the view plane distance
    float focalDistance = camera->focalDistance.getValue();
    if (focalDistance <= 0.0f) {
        focalDistance = cameraPos.length();
        if (focalDistance <= 0.0f) {
            focalDistance = 10.0f;
        }
    }

    // Calculate view plane dimensions (simplified approach)
    // Assume 45-degree field of view for simplicity
    float fovRadians = 45.0f * M_PI / 180.0f;
    float aspectRatio = canvasSize.x / static_cast<float>(canvasSize.y);

    float viewPlaneHeight = 2.0f * focalDistance * tanf(fovRadians * 0.5f);
    float viewPlaneWidth = viewPlaneHeight * aspectRatio;

    // Convert normalized coordinates to view plane coordinates
    // Map (0,0) to (-width/2, -height/2) and (1,1) to (width/2, height/2)
    float viewX = (normalizedX - 0.5f) * viewPlaneWidth;
    float viewY = (0.5f - normalizedY) * viewPlaneHeight; // Flip Y axis

    // Calculate world position on view plane
    SbVec3f planeCenter = cameraPos + viewDir * focalDistance;
    SbVec3f worldPoint = planeCenter + rightDir * viewX + upDir * viewY;

    return worldPoint;
}

void InventorNavigationController::loadMarkerConfig() {
    try {
        ConfigManager& config = ConfigManager::getInstance();

        // Load marker configuration from config.ini
        m_markerConfig.radius = static_cast<float>(config.getDouble("RotationCenter", "MarkerRadius", 0.15));
        m_markerConfig.transparency = static_cast<float>(config.getDouble("RotationCenter", "MarkerTransparency", 0.8));
        m_markerConfig.red = static_cast<float>(config.getDouble("RotationCenter", "MarkerRed", 1.0));
        m_markerConfig.green = static_cast<float>(config.getDouble("RotationCenter", "MarkerGreen", 0.0));
        m_markerConfig.blue = static_cast<float>(config.getDouble("RotationCenter", "MarkerBlue", 0.0));

        // Ensure values are within valid ranges
        m_markerConfig.radius = std::max(0.01f, std::min(m_markerConfig.radius, 1.0f));
        m_markerConfig.transparency = std::max(0.0f, std::min(m_markerConfig.transparency, 1.0f));
        m_markerConfig.red = std::max(0.0f, std::min(m_markerConfig.red, 1.0f));
        m_markerConfig.green = std::max(0.0f, std::min(m_markerConfig.green, 1.0f));
        m_markerConfig.blue = std::max(0.0f, std::min(m_markerConfig.blue, 1.0f));

        LOG_DBG_S("Loaded rotation center marker config: radius=" + std::to_string(m_markerConfig.radius) +
                  ", transparency=" + std::to_string(m_markerConfig.transparency) +
                  ", color=(" + std::to_string(m_markerConfig.red) + "," +
                  std::to_string(m_markerConfig.green) + "," + std::to_string(m_markerConfig.blue) + ")");

    } catch (const std::exception& e) {
        LOG_DBG_S("Failed to load marker configuration, using defaults: " + std::string(e.what()));
        // Keep default values
    }
}
