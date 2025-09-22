#include "InventorNavigationController.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/SbRotation.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
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
{
    LOG_INF_S("InventorNavigationController initializing");
    m_centerTime = wxGetLocalTimeMillis();
    m_lastMotionTime = wxGetLocalTimeMillis();
}

InventorNavigationController::~InventorNavigationController() {
    LOG_INF_S("InventorNavigationController destroying");
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
        
        if (event.ShiftDown() && m_currentMode != InventorNavigationMode::SELECTION) {
            m_centerTime = wxGetLocalTimeMillis();
            setupPanningPlane();
            m_lockRecenter = false;
        }
        else if (m_currentMode == InventorNavigationMode::IDLE) {
            m_isDragging = true;
            m_baseMousePos = pos;
            m_lastMousePos = pos;
            processed = true;
            m_lockRecenter = true;
        }
        else {
            processed = true;
        }
    }
    else if (event.LeftUp()) {
        m_button1Down = false;
        
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
                LOG_INF_S("Starting spin continuation");
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
            LOG_INF_S("Right click - could show context menu");
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
        LOG_INF_S("Navigation mode changed to: " + std::to_string(static_cast<int>(newMode)));
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
    
    SbVec3f position = camera->position.getValue();
    float distance = position.length();
    
    if (distance < 0.001f) {
        distance = 0.001f;
    }
    
    float theta = std::atan2(position[1], position[0]);
    float phi = std::acos(position[2] / distance);
    
    theta -= dx;
    phi += dy;
    
    if (phi < 0.001f) phi = 0.001f;
    if (phi > M_PI - 0.001f) phi = M_PI - 0.001f;
    
    float x = distance * std::sin(phi) * std::cos(theta);
    float y = distance * std::sin(phi) * std::sin(theta);
    float z = distance * std::cos(phi);
    
    camera->position.setValue(x, y, z);
    
    SbVec3f viewDir(-x, -y, -z);
    viewDir.normalize();
    
    SbVec3f defaultDir(0, 0, -1);
    SbRotation newOrientation(defaultDir, viewDir);
    camera->orientation.setValue(newOrientation);
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
    LOG_INF_S("Setting up panning plane");
}

void InventorNavigationController::lookAtPoint(const wxPoint& pos) {
    // Look at the point under cursor
    // This would typically involve projecting screen coordinates to world coordinates
    LOG_INF_S("Looking at point: " + std::to_string(pos.x) + ", " + std::to_string(pos.y));
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
    LOG_INF_S("Navigation mode set to: " + std::to_string(static_cast<int>(mode)));
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
