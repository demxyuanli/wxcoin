#include "NavigationController.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "Logger.h"
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/SbRotation.h>
#include <cmath>

NavigationController::NavigationController(Canvas* canvas, SceneManager* sceneManager)
    : m_canvas(canvas)
    , m_sceneManager(sceneManager)
    , m_isDragging(false)
    , m_dragMode(DragMode::NONE)
{
    LOG_INF("NavigationController initializing");
}

NavigationController::~NavigationController() {
    LOG_INF("NavigationController destroying");
}

void NavigationController::handleMouseButton(wxMouseEvent& event) {
    if (event.LeftDown()) {
        m_isDragging = true;
        m_dragMode = DragMode::ROTATE;
        m_lastMousePos = event.GetPosition();
    }
    else if (event.RightDown()) {
        m_isDragging = true;
        m_dragMode = DragMode::PAN;
        m_lastMousePos = event.GetPosition();
    }
    else if (event.LeftUp() || event.RightUp()) {
        m_isDragging = false;
        m_dragMode = DragMode::NONE;
    }
    event.Skip();
}

void NavigationController::handleMouseMotion(wxMouseEvent& event) {
    if (!m_isDragging) {
        event.Skip();
        return;
    }

    wxPoint currentPos = event.GetPosition();
    if (m_dragMode == DragMode::ROTATE) {
        rotateCamera(currentPos, m_lastMousePos);
    }
    else if (m_dragMode == DragMode::PAN) {
        panCamera(currentPos, m_lastMousePos);
    }
    m_lastMousePos = currentPos;
    m_canvas->Refresh();
    event.Skip();
}

void NavigationController::handleMouseWheel(wxMouseEvent& event) {
    float delta = event.GetWheelRotation() / 120.0f;
    zoomCamera(delta);
    m_canvas->Refresh();
    event.Skip();
}

void NavigationController::rotateCamera(const wxPoint& currentPos, const wxPoint& lastPos) {
    SoCamera* camera = m_sceneManager->getCamera();
    if (!camera) {
        LOG_ERR("Cannot rotate: Invalid camera");
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

void NavigationController::panCamera(const wxPoint& currentPos, const wxPoint& lastPos) {
    SoCamera* camera = m_sceneManager->getCamera();
    if (!camera) {
        LOG_ERR("Cannot pan: Invalid camera");
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
}

void NavigationController::zoomCamera(float delta) {
    SoCamera* camera = m_sceneManager->getCamera();
    if (!camera) {
        LOG_ERR("Cannot zoom: Invalid camera");
        return;
    }

    SbVec3f position = camera->position.getValue();
    SbVec3f forward;
    camera->orientation.getValue().multVec(SbVec3f(0, 0, -1), forward);
    position += forward * delta * 0.1f;
    camera->position.setValue(position);
}

void NavigationController::viewAll() {
    m_sceneManager->resetView();
}

void NavigationController::viewTop() {
    SoCamera* camera = m_sceneManager->getCamera();
    if (!camera) {
        LOG_ERR("Cannot set top view: Invalid camera");
        return;
    }
    
    m_sceneManager->setView("Top");
}

void NavigationController::viewFront() {
    SoCamera* camera = m_sceneManager->getCamera();
    if (!camera) {
        LOG_ERR("Cannot set front view: Invalid camera");
        return;
    }
    
    m_sceneManager->setView("Front");
}

void NavigationController::viewRight() {
    SoCamera* camera = m_sceneManager->getCamera();
    if (!camera) {
        LOG_ERR("Cannot set right view: Invalid camera");
        return;
    }
    
    m_sceneManager->setView("Right");
}

void NavigationController::viewIsometric() {
    SoCamera* camera = m_sceneManager->getCamera();
    if (!camera) {
        LOG_ERR("Cannot set isometric view: Invalid camera");
        return;
    }
    
    m_sceneManager->setView("Isometric");
}