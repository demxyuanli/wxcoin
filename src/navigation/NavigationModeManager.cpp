#include "NavigationModeManager.h"
#include "NavigationController.h"
#include "InventorNavigationController.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "logger/Logger.h"
#include "CameraAnimation.h"
#include "config/ConfigManager.h"
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/SbRotation.h>
#include <cmath>

// Adapter classes to bridge existing controllers to new interface
class GestureNavigationAdapter : public INavigationStyle {
public:
    GestureNavigationAdapter(NavigationController* controller) : m_controller(controller) {}

    void handleMouseButton(wxMouseEvent& event) override {
        if (m_controller) m_controller->handleMouseButton(event);
    }

    void handleMouseMotion(wxMouseEvent& event) override {
        if (m_controller) m_controller->handleMouseMotion(event);
    }

    void handleMouseWheel(wxMouseEvent& event) override {
        if (m_controller) m_controller->handleMouseWheel(event);
    }

    void viewAll() override {
        if (m_controller) m_controller->viewAll();
    }

    void viewTop() override {
        if (m_controller) m_controller->viewTop();
    }

    void viewFront() override {
        if (m_controller) m_controller->viewFront();
    }

    void viewRight() override {
        if (m_controller) m_controller->viewRight();
    }

    void viewIsometric() override {
        if (m_controller) m_controller->viewIsometric();
    }

    void setZoomSpeedFactor(float factor) override {
        if (m_controller) m_controller->setZoomSpeedFactor(factor);
    }

    float getZoomSpeedFactor() const override {
        return m_controller ? m_controller->getZoomSpeedFactor() : 1.0f;
    }

    std::string getStyleName() const override { return "Gesture"; }
    std::string getStyleDescription() const override {
        return "Simple gesture-based navigation - Left: Rotate, Right: Pan, Wheel: Zoom";
    }

private:
    NavigationController* m_controller;
};

class InventorNavigationAdapter : public INavigationStyle {
public:
    InventorNavigationAdapter(InventorNavigationController* controller) : m_controller(controller) {}

    void handleMouseButton(wxMouseEvent& event) override {
        if (m_controller) m_controller->handleMouseButton(event);
    }

    void handleMouseMotion(wxMouseEvent& event) override {
        if (m_controller) m_controller->handleMouseMotion(event);
    }

    void handleMouseWheel(wxMouseEvent& event) override {
        if (m_controller) m_controller->handleMouseWheel(event);
    }

    void viewAll() override {
        if (m_controller) m_controller->viewAll();
    }

    void viewTop() override {
        if (m_controller) m_controller->viewTop();
    }

    void viewFront() override {
        if (m_controller) m_controller->viewFront();
    }

    void viewRight() override {
        if (m_controller) m_controller->viewRight();
    }

    void viewIsometric() override {
        if (m_controller) m_controller->viewIsometric();
    }

    void setZoomSpeedFactor(float factor) override {
        if (m_controller) m_controller->setZoomSpeedFactor(factor);
    }

    float getZoomSpeedFactor() const override {
        return m_controller ? m_controller->getZoomSpeedFactor() : 1.0f;
    }

    void setRotationCenter(const SbVec3f& center) override {
        if (m_controller) m_controller->setRotationCenter(center);
    }

    void clearRotationCenter() override {
        if (m_controller) m_controller->clearRotationCenter();
    }

    bool hasRotationCenter() const override {
        return m_controller ? m_controller->hasRotationCenter() : false;
    }

    const SbVec3f& getRotationCenter() const override {
        static SbVec3f empty(0, 0, 0);
        return m_controller ? m_controller->getRotationCenter() : empty;
    }

    std::string getStyleName() const override { return "Inventor"; }
    std::string getStyleDescription() const override {
        return "Open Inventor style navigation with rotation center and spin continuation";
    }

private:
    InventorNavigationController* m_controller;
};

class CADNavigationStyle : public INavigationStyle {
public:
    CADNavigationStyle(Canvas* canvas, SceneManager* sceneManager)
        : m_canvas(canvas)
        , m_sceneManager(sceneManager)
        , m_isDragging(false)
        , m_dragMode(DragMode::NONE)
        , m_zoomSpeedFactor(1.0f)
    {
    }

    void handleMouseButton(wxMouseEvent& event) override {
        if (event.LeftDown()) {
            m_isDragging = true;
            m_dragMode = DragMode::ROTATE;
            m_lastMousePos = event.GetPosition();
        }
        else if (event.MiddleDown()) {
            m_isDragging = true;
            m_dragMode = DragMode::PAN;
            m_lastMousePos = event.GetPosition();
        }
        else if (event.RightDown()) {
            m_isDragging = true;
            m_dragMode = DragMode::ZOOM;
            m_lastMousePos = event.GetPosition();
        }
        else if (event.LeftUp() || event.MiddleUp() || event.RightUp()) {
            m_isDragging = false;
            m_dragMode = DragMode::NONE;
        }
    }

    void handleMouseMotion(wxMouseEvent& event) override {
        if (!m_isDragging) {
            return;
        }

        wxPoint currentPos = event.GetPosition();
        switch (m_dragMode) {
        case DragMode::ROTATE:
            rotateCamera(currentPos, m_lastMousePos);
            break;
        case DragMode::PAN:
            panCamera(currentPos, m_lastMousePos);
            break;
        case DragMode::ZOOM:
            zoomByDrag(currentPos, m_lastMousePos);
            break;
        }
        m_lastMousePos = currentPos;
        if (m_canvas) {
            m_canvas->Refresh(false);
        }
    }

    void handleMouseWheel(wxMouseEvent& event) override {
        float delta = event.GetWheelRotation() / 120.0f;
        zoomCamera(delta);
        if (m_canvas) {
            m_canvas->Refresh(false);
        }
    }

    void viewAll() override {
        if (m_sceneManager) {
            m_sceneManager->resetView(true);
        }
    }

    void viewTop() override {
        if (m_sceneManager) {
            m_sceneManager->setView("Top");
        }
    }

    void viewFront() override {
        if (m_sceneManager) {
            m_sceneManager->setView("Front");
        }
    }

    void viewRight() override {
        if (m_sceneManager) {
            m_sceneManager->setView("Right");
        }
    }

    void viewIsometric() override {
        if (m_sceneManager) {
            m_sceneManager->setView("Isometric");
        }
    }

    void setZoomSpeedFactor(float factor) override {
        m_zoomSpeedFactor = factor;
    }

    float getZoomSpeedFactor() const override {
        return m_zoomSpeedFactor;
    }

    std::string getStyleName() const override { return "CAD"; }
    std::string getStyleDescription() const override {
        return "Professional CAD navigation - Left: Rotate, Middle: Pan, Right: Zoom, Wheel: Zoom";
    }

private:
    enum class DragMode { NONE, ROTATE, PAN, ZOOM };

    Canvas* m_canvas;
    SceneManager* m_sceneManager;
    bool m_isDragging;
    DragMode m_dragMode;
    wxPoint m_lastMousePos;
    float m_zoomSpeedFactor;

    void rotateCamera(const wxPoint& currentPos, const wxPoint& lastPos);
    void panCamera(const wxPoint& currentPos, const wxPoint& lastPos);
    void zoomCamera(float delta);
    void zoomByDrag(const wxPoint& currentPos, const wxPoint& lastPos);
};

void CADNavigationStyle::rotateCamera(const wxPoint& currentPos, const wxPoint& lastPos) {
    NavigationAnimator::getInstance().stopCurrentAnimation();

    SoCamera* camera = m_sceneManager ? m_sceneManager->getCamera() : nullptr;
    if (!camera) {
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

void CADNavigationStyle::panCamera(const wxPoint& currentPos, const wxPoint& lastPos) {
    NavigationAnimator::getInstance().stopCurrentAnimation();

    SoCamera* camera = m_sceneManager ? m_sceneManager->getCamera() : nullptr;
    if (!camera) {
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

void CADNavigationStyle::zoomCamera(float delta) {
    NavigationAnimator::getInstance().stopCurrentAnimation();

    SoCamera* camera = m_sceneManager ? m_sceneManager->getCamera() : nullptr;
    if (!camera) {
        return;
    }

    SbVec3f position = camera->position.getValue();
    SbVec3f forward;
    camera->orientation.getValue().multVec(SbVec3f(0, 0, -1), forward);

    float sceneSize = m_sceneManager->getSceneBoundingBoxSize();
    float zoomFactor = sceneSize / 100.0f;

    position += forward * delta * zoomFactor * m_zoomSpeedFactor;
    camera->position.setValue(position);
}

void CADNavigationStyle::zoomByDrag(const wxPoint& currentPos, const wxPoint& lastPos) {
    float dy = static_cast<float>(currentPos.y - lastPos.y) / 100.0f;
    zoomCamera(-dy);
}

class TouchpadNavigationStyle : public INavigationStyle {
public:
    TouchpadNavigationStyle(Canvas* canvas, SceneManager* sceneManager)
        : m_canvas(canvas)
        , m_sceneManager(sceneManager)
        , m_isDragging(false)
        , m_dragMode(DragMode::NONE)
        , m_zoomSpeedFactor(1.0f)
        , m_panSensitivity(0.02f)
    {
    }

    void handleMouseButton(wxMouseEvent& event) override {
        if (event.LeftDown() && event.ControlDown()) {
            m_isDragging = true;
            m_dragMode = DragMode::PAN;
            m_lastMousePos = event.GetPosition();
        }
        else if (event.LeftDown()) {
            m_isDragging = true;
            m_dragMode = DragMode::ROTATE;
            m_lastMousePos = event.GetPosition();
        }
        else if (event.RightDown()) {
            m_isDragging = true;
            m_dragMode = DragMode::ZOOM;
            m_lastMousePos = event.GetPosition();
        }
        else if (event.LeftUp() || event.RightUp()) {
            m_isDragging = false;
            m_dragMode = DragMode::NONE;
        }
    }

    void handleMouseMotion(wxMouseEvent& event) override {
        if (!m_isDragging) {
            return;
        }

        wxPoint currentPos = event.GetPosition();
        switch (m_dragMode) {
        case DragMode::ROTATE:
            rotateCamera(currentPos, m_lastMousePos);
            break;
        case DragMode::PAN:
            panCamera(currentPos, m_lastMousePos);
            break;
        case DragMode::ZOOM:
            zoomByDrag(currentPos, m_lastMousePos);
            break;
        }
        m_lastMousePos = currentPos;
        if (m_canvas) {
            m_canvas->Refresh(false);
        }
    }

    void handleMouseWheel(wxMouseEvent& event) override {
        float delta = event.GetWheelRotation() / 120.0f;
        zoomCamera(delta);
        if (m_canvas) {
            m_canvas->Refresh(false);
        }
    }

    void viewAll() override {
        if (m_sceneManager) {
            m_sceneManager->resetView(true);
        }
    }

    void viewTop() override {
        if (m_sceneManager) {
            m_sceneManager->setView("Top");
        }
    }

    void viewFront() override {
        if (m_sceneManager) {
            m_sceneManager->setView("Front");
        }
    }

    void viewRight() override {
        if (m_sceneManager) {
            m_sceneManager->setView("Right");
        }
    }

    void viewIsometric() override {
        if (m_sceneManager) {
            m_sceneManager->setView("Isometric");
        }
    }

    void setZoomSpeedFactor(float factor) override {
        m_zoomSpeedFactor = factor;
    }

    float getZoomSpeedFactor() const override {
        return m_zoomSpeedFactor;
    }

    std::string getStyleName() const override { return "Touchpad"; }
    std::string getStyleDescription() const override {
        return "Touchpad-optimized navigation - Left: Rotate, Ctrl+Left: Pan, Right: Zoom, Wheel: Zoom";
    }

private:
    enum class DragMode { NONE, ROTATE, PAN, ZOOM };

    Canvas* m_canvas;
    SceneManager* m_sceneManager;
    bool m_isDragging;
    DragMode m_dragMode;
    wxPoint m_lastMousePos;
    float m_zoomSpeedFactor;
    float m_panSensitivity;

    void rotateCamera(const wxPoint& currentPos, const wxPoint& lastPos);
    void panCamera(const wxPoint& currentPos, const wxPoint& lastPos);
    void zoomCamera(float delta);
    void zoomByDrag(const wxPoint& currentPos, const wxPoint& lastPos);
};

void TouchpadNavigationStyle::rotateCamera(const wxPoint& currentPos, const wxPoint& lastPos) {
    NavigationAnimator::getInstance().stopCurrentAnimation();

    SoCamera* camera = m_sceneManager ? m_sceneManager->getCamera() : nullptr;
    if (!camera) {
        return;
    }

    float dx = static_cast<float>(currentPos.x - lastPos.x) / 150.0f;
    float dy = static_cast<float>(currentPos.y - lastPos.y) / 150.0f;

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

void TouchpadNavigationStyle::panCamera(const wxPoint& currentPos, const wxPoint& lastPos) {
    NavigationAnimator::getInstance().stopCurrentAnimation();

    SoCamera* camera = m_sceneManager ? m_sceneManager->getCamera() : nullptr;
    if (!camera) {
        return;
    }

    float dx = static_cast<float>(lastPos.x - currentPos.x) * m_panSensitivity;
    float dy = static_cast<float>(currentPos.y - lastPos.y) * m_panSensitivity;

    SbVec3f position = camera->position.getValue();
    SbVec3f right, up;
    camera->orientation.getValue().multVec(SbVec3f(1, 0, 0), right);
    camera->orientation.getValue().multVec(SbVec3f(0, 1, 0), up);
    position += right * dx + up * dy;
    camera->position.setValue(position);
}

void TouchpadNavigationStyle::zoomCamera(float delta) {
    NavigationAnimator::getInstance().stopCurrentAnimation();

    SoCamera* camera = m_sceneManager ? m_sceneManager->getCamera() : nullptr;
    if (!camera) {
        return;
    }

    SbVec3f position = camera->position.getValue();
    SbVec3f forward;
    camera->orientation.getValue().multVec(SbVec3f(0, 0, -1), forward);

    float sceneSize = m_sceneManager->getSceneBoundingBoxSize();
    float zoomFactor = sceneSize / 100.0f;

    position += forward * delta * zoomFactor * m_zoomSpeedFactor;
    camera->position.setValue(position);
}

void TouchpadNavigationStyle::zoomByDrag(const wxPoint& currentPos, const wxPoint& lastPos) {
    float dy = static_cast<float>(currentPos.y - lastPos.y) / 150.0f;
    zoomCamera(-dy);
}

class MayaGestureNavigationStyle : public INavigationStyle {
public:
    MayaGestureNavigationStyle(Canvas* canvas, SceneManager* sceneManager)
        : m_canvas(canvas)
        , m_sceneManager(sceneManager)
        , m_isDragging(false)
        , m_dragMode(DragMode::NONE)
        , m_zoomSpeedFactor(1.0f)
    {
    }

    void handleMouseButton(wxMouseEvent& event) override {
        if (event.LeftDown() && event.AltDown()) {
            m_isDragging = true;
            m_dragMode = DragMode::ROTATE;
            m_lastMousePos = event.GetPosition();
        }
        else if (event.MiddleDown() && event.AltDown()) {
            m_isDragging = true;
            m_dragMode = DragMode::PAN;
            m_lastMousePos = event.GetPosition();
        }
        else if (event.RightDown() && event.AltDown()) {
            m_isDragging = true;
            m_dragMode = DragMode::ZOOM;
            m_lastMousePos = event.GetPosition();
        }
        else if (event.LeftUp() || event.MiddleUp() || event.RightUp()) {
            m_isDragging = false;
            m_dragMode = DragMode::NONE;
        }
    }

    void handleMouseMotion(wxMouseEvent& event) override {
        if (!m_isDragging) {
            return;
        }

        wxPoint currentPos = event.GetPosition();
        switch (m_dragMode) {
        case DragMode::ROTATE:
            rotateCamera(currentPos, m_lastMousePos);
            break;
        case DragMode::PAN:
            panCamera(currentPos, m_lastMousePos);
            break;
        case DragMode::ZOOM:
            zoomByDrag(currentPos, m_lastMousePos);
            break;
        }
        m_lastMousePos = currentPos;
        if (m_canvas) {
            m_canvas->Refresh(false);
        }
    }

    void handleMouseWheel(wxMouseEvent& event) override {
        float delta = event.GetWheelRotation() / 120.0f;
        zoomCamera(delta);
        if (m_canvas) {
            m_canvas->Refresh(false);
        }
    }

    void viewAll() override {
        if (m_sceneManager) {
            m_sceneManager->resetView(true);
        }
    }

    void viewTop() override {
        if (m_sceneManager) {
            m_sceneManager->setView("Top");
        }
    }

    void viewFront() override {
        if (m_sceneManager) {
            m_sceneManager->setView("Front");
        }
    }

    void viewRight() override {
        if (m_sceneManager) {
            m_sceneManager->setView("Right");
        }
    }

    void viewIsometric() override {
        if (m_sceneManager) {
            m_sceneManager->setView("Isometric");
        }
    }

    void setZoomSpeedFactor(float factor) override {
        m_zoomSpeedFactor = factor;
    }

    float getZoomSpeedFactor() const override {
        return m_zoomSpeedFactor;
    }

    std::string getStyleName() const override { return "Maya Gesture"; }
    std::string getStyleDescription() const override {
        return "Maya-style navigation - Alt+Left: Rotate (Tumble), Alt+Middle: Pan, Alt+Right: Zoom (Dolly)";
    }

private:
    enum class DragMode { NONE, ROTATE, PAN, ZOOM };

    Canvas* m_canvas;
    SceneManager* m_sceneManager;
    bool m_isDragging;
    DragMode m_dragMode;
    wxPoint m_lastMousePos;
    float m_zoomSpeedFactor;

    void rotateCamera(const wxPoint& currentPos, const wxPoint& lastPos);
    void panCamera(const wxPoint& currentPos, const wxPoint& lastPos);
    void zoomCamera(float delta);
    void zoomByDrag(const wxPoint& currentPos, const wxPoint& lastPos);
};

void MayaGestureNavigationStyle::rotateCamera(const wxPoint& currentPos, const wxPoint& lastPos) {
    NavigationAnimator::getInstance().stopCurrentAnimation();

    SoCamera* camera = m_sceneManager ? m_sceneManager->getCamera() : nullptr;
    if (!camera) {
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

void MayaGestureNavigationStyle::panCamera(const wxPoint& currentPos, const wxPoint& lastPos) {
    NavigationAnimator::getInstance().stopCurrentAnimation();

    SoCamera* camera = m_sceneManager ? m_sceneManager->getCamera() : nullptr;
    if (!camera) {
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

void MayaGestureNavigationStyle::zoomCamera(float delta) {
    NavigationAnimator::getInstance().stopCurrentAnimation();

    SoCamera* camera = m_sceneManager ? m_sceneManager->getCamera() : nullptr;
    if (!camera) {
        return;
    }

    SbVec3f position = camera->position.getValue();
    SbVec3f forward;
    camera->orientation.getValue().multVec(SbVec3f(0, 0, -1), forward);

    float sceneSize = m_sceneManager->getSceneBoundingBoxSize();
    float zoomFactor = sceneSize / 100.0f;

    position += forward * delta * zoomFactor * m_zoomSpeedFactor;
    camera->position.setValue(position);
}

void MayaGestureNavigationStyle::zoomByDrag(const wxPoint& currentPos, const wxPoint& lastPos) {
    float dy = static_cast<float>(currentPos.y - lastPos.y) / 100.0f;
    zoomCamera(-dy);
}

class BlenderNavigationStyle : public INavigationStyle {
public:
    BlenderNavigationStyle(Canvas* canvas, SceneManager* sceneManager)
        : m_canvas(canvas)
        , m_sceneManager(sceneManager)
        , m_isDragging(false)
        , m_dragMode(DragMode::NONE)
        , m_zoomSpeedFactor(1.0f)
    {
    }

    void handleMouseButton(wxMouseEvent& event) override {
        if (event.MiddleDown()) {
            m_isDragging = true;
            m_dragMode = DragMode::ROTATE;
            m_lastMousePos = event.GetPosition();
        }
        else if (event.MiddleDown() && event.ShiftDown()) {
            m_isDragging = true;
            m_dragMode = DragMode::PAN;
            m_lastMousePos = event.GetPosition();
        }
        else if (event.MiddleDown() && event.ControlDown()) {
            m_isDragging = true;
            m_dragMode = DragMode::ZOOM;
            m_lastMousePos = event.GetPosition();
        }
        else if (event.MiddleUp()) {
            m_isDragging = false;
            m_dragMode = DragMode::NONE;
        }
    }

    void handleMouseMotion(wxMouseEvent& event) override {
        if (!m_isDragging) {
            return;
        }

        wxPoint currentPos = event.GetPosition();
        if (event.ShiftDown() && m_dragMode == DragMode::ROTATE) {
            panCamera(currentPos, m_lastMousePos);
        }
        else if (event.ControlDown() && m_dragMode == DragMode::ROTATE) {
            zoomByDrag(currentPos, m_lastMousePos);
        }
        else {
            switch (m_dragMode) {
            case DragMode::ROTATE:
                rotateCamera(currentPos, m_lastMousePos);
                break;
            case DragMode::PAN:
                panCamera(currentPos, m_lastMousePos);
                break;
            case DragMode::ZOOM:
                zoomByDrag(currentPos, m_lastMousePos);
                break;
            }
        }
        m_lastMousePos = currentPos;
        if (m_canvas) {
            m_canvas->Refresh(false);
        }
    }

    void handleMouseWheel(wxMouseEvent& event) override {
        float delta = event.GetWheelRotation() / 120.0f;
        zoomCamera(delta);
        if (m_canvas) {
            m_canvas->Refresh(false);
        }
    }

    void viewAll() override {
        if (m_sceneManager) {
            m_sceneManager->resetView(true);
        }
    }

    void viewTop() override {
        if (m_sceneManager) {
            m_sceneManager->setView("Top");
        }
    }

    void viewFront() override {
        if (m_sceneManager) {
            m_sceneManager->setView("Front");
        }
    }

    void viewRight() override {
        if (m_sceneManager) {
            m_sceneManager->setView("Right");
        }
    }

    void viewIsometric() override {
        if (m_sceneManager) {
            m_sceneManager->setView("Isometric");
        }
    }

    void setZoomSpeedFactor(float factor) override {
        m_zoomSpeedFactor = factor;
    }

    float getZoomSpeedFactor() const override {
        return m_zoomSpeedFactor;
    }

    std::string getStyleName() const override { return "Blender"; }
    std::string getStyleDescription() const override {
        return "Blender-style navigation - Middle: Rotate, Shift+Middle: Pan, Ctrl+Middle: Zoom, Wheel: Zoom";
    }

private:
    enum class DragMode { NONE, ROTATE, PAN, ZOOM };

    Canvas* m_canvas;
    SceneManager* m_sceneManager;
    bool m_isDragging;
    DragMode m_dragMode;
    wxPoint m_lastMousePos;
    float m_zoomSpeedFactor;

    void rotateCamera(const wxPoint& currentPos, const wxPoint& lastPos);
    void panCamera(const wxPoint& currentPos, const wxPoint& lastPos);
    void zoomCamera(float delta);
    void zoomByDrag(const wxPoint& currentPos, const wxPoint& lastPos);
};

void BlenderNavigationStyle::rotateCamera(const wxPoint& currentPos, const wxPoint& lastPos) {
    NavigationAnimator::getInstance().stopCurrentAnimation();

    SoCamera* camera = m_sceneManager ? m_sceneManager->getCamera() : nullptr;
    if (!camera) {
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

void BlenderNavigationStyle::panCamera(const wxPoint& currentPos, const wxPoint& lastPos) {
    NavigationAnimator::getInstance().stopCurrentAnimation();

    SoCamera* camera = m_sceneManager ? m_sceneManager->getCamera() : nullptr;
    if (!camera) {
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

void BlenderNavigationStyle::zoomCamera(float delta) {
    NavigationAnimator::getInstance().stopCurrentAnimation();

    SoCamera* camera = m_sceneManager ? m_sceneManager->getCamera() : nullptr;
    if (!camera) {
        return;
    }

    SbVec3f position = camera->position.getValue();
    SbVec3f forward;
    camera->orientation.getValue().multVec(SbVec3f(0, 0, -1), forward);

    float sceneSize = m_sceneManager->getSceneBoundingBoxSize();
    float zoomFactor = sceneSize / 100.0f;

    position += forward * delta * zoomFactor * m_zoomSpeedFactor;
    camera->position.setValue(position);
}

void BlenderNavigationStyle::zoomByDrag(const wxPoint& currentPos, const wxPoint& lastPos) {
    float dy = static_cast<float>(currentPos.y - lastPos.y) / 100.0f;
    zoomCamera(-dy);
}

class RevitNavigationStyle : public INavigationStyle {
public:
    RevitNavigationStyle(Canvas* canvas, SceneManager* sceneManager)
        : m_canvas(canvas)
        , m_sceneManager(sceneManager)
        , m_isDragging(false)
        , m_dragMode(DragMode::NONE)
        , m_zoomSpeedFactor(1.0f)
    {
    }

    void handleMouseButton(wxMouseEvent& event) override {
        if (event.RightDown() && event.ShiftDown()) {
            m_isDragging = true;
            m_dragMode = DragMode::ROTATE;
            m_lastMousePos = event.GetPosition();
        }
        else if (event.RightDown() && event.ControlDown()) {
            m_isDragging = true;
            m_dragMode = DragMode::ZOOM;
            m_lastMousePos = event.GetPosition();
        }
        else if (event.MiddleDown()) {
            m_isDragging = true;
            m_dragMode = DragMode::PAN;
            m_lastMousePos = event.GetPosition();
        }
        else if (event.RightUp() || event.MiddleUp()) {
            m_isDragging = false;
            m_dragMode = DragMode::NONE;
        }
    }

    void handleMouseMotion(wxMouseEvent& event) override {
        if (!m_isDragging) {
            return;
        }

        wxPoint currentPos = event.GetPosition();
        switch (m_dragMode) {
        case DragMode::ROTATE:
            rotateCamera(currentPos, m_lastMousePos);
            break;
        case DragMode::PAN:
            panCamera(currentPos, m_lastMousePos);
            break;
        case DragMode::ZOOM:
            zoomByDrag(currentPos, m_lastMousePos);
            break;
        }
        m_lastMousePos = currentPos;
        if (m_canvas) {
            m_canvas->Refresh(false);
        }
    }

    void handleMouseWheel(wxMouseEvent& event) override {
        float delta = event.GetWheelRotation() / 120.0f;
        zoomCamera(delta);
        if (m_canvas) {
            m_canvas->Refresh(false);
        }
    }

    void viewAll() override {
        if (m_sceneManager) {
            m_sceneManager->resetView(true);
        }
    }

    void viewTop() override {
        if (m_sceneManager) {
            m_sceneManager->setView("Top");
        }
    }

    void viewFront() override {
        if (m_sceneManager) {
            m_sceneManager->setView("Front");
        }
    }

    void viewRight() override {
        if (m_sceneManager) {
            m_sceneManager->setView("Right");
        }
    }

    void viewIsometric() override {
        if (m_sceneManager) {
            m_sceneManager->setView("Isometric");
        }
    }

    void setZoomSpeedFactor(float factor) override {
        m_zoomSpeedFactor = factor;
    }

    float getZoomSpeedFactor() const override {
        return m_zoomSpeedFactor;
    }

    std::string getStyleName() const override { return "Revit"; }
    std::string getStyleDescription() const override {
        return "Revit-style navigation - Shift+Right: Rotate, Middle: Pan, Ctrl+Right: Zoom, Wheel: Zoom";
    }

private:
    enum class DragMode { NONE, ROTATE, PAN, ZOOM };

    Canvas* m_canvas;
    SceneManager* m_sceneManager;
    bool m_isDragging;
    DragMode m_dragMode;
    wxPoint m_lastMousePos;
    float m_zoomSpeedFactor;

    void rotateCamera(const wxPoint& currentPos, const wxPoint& lastPos);
    void panCamera(const wxPoint& currentPos, const wxPoint& lastPos);
    void zoomCamera(float delta);
    void zoomByDrag(const wxPoint& currentPos, const wxPoint& lastPos);
};

void RevitNavigationStyle::rotateCamera(const wxPoint& currentPos, const wxPoint& lastPos) {
    NavigationAnimator::getInstance().stopCurrentAnimation();

    SoCamera* camera = m_sceneManager ? m_sceneManager->getCamera() : nullptr;
    if (!camera) {
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

void RevitNavigationStyle::panCamera(const wxPoint& currentPos, const wxPoint& lastPos) {
    NavigationAnimator::getInstance().stopCurrentAnimation();

    SoCamera* camera = m_sceneManager ? m_sceneManager->getCamera() : nullptr;
    if (!camera) {
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

void RevitNavigationStyle::zoomCamera(float delta) {
    NavigationAnimator::getInstance().stopCurrentAnimation();

    SoCamera* camera = m_sceneManager ? m_sceneManager->getCamera() : nullptr;
    if (!camera) {
        return;
    }

    SbVec3f position = camera->position.getValue();
    SbVec3f forward;
    camera->orientation.getValue().multVec(SbVec3f(0, 0, -1), forward);

    float sceneSize = m_sceneManager->getSceneBoundingBoxSize();
    float zoomFactor = sceneSize / 100.0f;

    position += forward * delta * zoomFactor * m_zoomSpeedFactor;
    camera->position.setValue(position);
}

void RevitNavigationStyle::zoomByDrag(const wxPoint& currentPos, const wxPoint& lastPos) {
    float dy = static_cast<float>(currentPos.y - lastPos.y) / 100.0f;
    zoomCamera(-dy);
}

class TinkerCADNavigationStyle : public INavigationStyle {
public:
    TinkerCADNavigationStyle(Canvas* canvas, SceneManager* sceneManager)
        : m_canvas(canvas)
        , m_sceneManager(sceneManager)
        , m_isDragging(false)
        , m_dragMode(DragMode::NONE)
        , m_zoomSpeedFactor(1.0f)
    {
    }

    void handleMouseButton(wxMouseEvent& event) override {
        if (event.RightDown()) {
            m_isDragging = true;
            m_dragMode = DragMode::ROTATE;
            m_lastMousePos = event.GetPosition();
        }
        else if (event.MiddleDown()) {
            m_isDragging = true;
            m_dragMode = DragMode::PAN;
            m_lastMousePos = event.GetPosition();
        }
        else if (event.LeftUp() || event.RightUp() || event.MiddleUp()) {
            m_isDragging = false;
            m_dragMode = DragMode::NONE;
        }
    }

    void handleMouseMotion(wxMouseEvent& event) override {
        if (!m_isDragging) {
            return;
        }

        wxPoint currentPos = event.GetPosition();
        switch (m_dragMode) {
        case DragMode::ROTATE:
            rotateCamera(currentPos, m_lastMousePos);
            break;
        case DragMode::PAN:
            panCamera(currentPos, m_lastMousePos);
            break;
        }
        m_lastMousePos = currentPos;
        if (m_canvas) {
            m_canvas->Refresh(false);
        }
    }

    void handleMouseWheel(wxMouseEvent& event) override {
        float delta = event.GetWheelRotation() / 120.0f;
        zoomCamera(delta);
        if (m_canvas) {
            m_canvas->Refresh(false);
        }
    }

    void viewAll() override {
        if (m_sceneManager) {
            m_sceneManager->resetView(true);
        }
    }

    void viewTop() override {
        if (m_sceneManager) {
            m_sceneManager->setView("Top");
        }
    }

    void viewFront() override {
        if (m_sceneManager) {
            m_sceneManager->setView("Front");
        }
    }

    void viewRight() override {
        if (m_sceneManager) {
            m_sceneManager->setView("Right");
        }
    }

    void viewIsometric() override {
        if (m_sceneManager) {
            m_sceneManager->setView("Isometric");
        }
    }

    void setZoomSpeedFactor(float factor) override {
        m_zoomSpeedFactor = factor;
    }

    float getZoomSpeedFactor() const override {
        return m_zoomSpeedFactor;
    }

    std::string getStyleName() const override { return "TinkerCAD"; }
    std::string getStyleDescription() const override {
        return "TinkerCAD-style navigation - Right: Rotate, Middle: Pan, Wheel: Zoom";
    }

private:
    enum class DragMode { NONE, ROTATE, PAN };

    Canvas* m_canvas;
    SceneManager* m_sceneManager;
    bool m_isDragging;
    DragMode m_dragMode;
    wxPoint m_lastMousePos;
    float m_zoomSpeedFactor;

    void rotateCamera(const wxPoint& currentPos, const wxPoint& lastPos);
    void panCamera(const wxPoint& currentPos, const wxPoint& lastPos);
    void zoomCamera(float delta);
};

void TinkerCADNavigationStyle::rotateCamera(const wxPoint& currentPos, const wxPoint& lastPos) {
    NavigationAnimator::getInstance().stopCurrentAnimation();

    SoCamera* camera = m_sceneManager ? m_sceneManager->getCamera() : nullptr;
    if (!camera) {
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

void TinkerCADNavigationStyle::panCamera(const wxPoint& currentPos, const wxPoint& lastPos) {
    NavigationAnimator::getInstance().stopCurrentAnimation();

    SoCamera* camera = m_sceneManager ? m_sceneManager->getCamera() : nullptr;
    if (!camera) {
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

void TinkerCADNavigationStyle::zoomCamera(float delta) {
    NavigationAnimator::getInstance().stopCurrentAnimation();

    SoCamera* camera = m_sceneManager ? m_sceneManager->getCamera() : nullptr;
    if (!camera) {
        return;
    }

    SbVec3f position = camera->position.getValue();
    SbVec3f forward;
    camera->orientation.getValue().multVec(SbVec3f(0, 0, -1), forward);

    float sceneSize = m_sceneManager->getSceneBoundingBoxSize();
    float zoomFactor = sceneSize / 100.0f;

    position += forward * delta * zoomFactor * m_zoomSpeedFactor;
    camera->position.setValue(position);
}

NavigationModeManager::NavigationModeManager(Canvas* canvas, SceneManager* sceneManager)
    : m_canvas(canvas)
    , m_sceneManager(sceneManager)
    , m_currentStyle(NavigationStyle::GESTURE)
{
    LOG_DBG_S("NavigationModeManager initializing");
    initializeControllers();
    initializeNavigationStyles();
    loadNavigationStyleFromConfig();
}

NavigationModeManager::~NavigationModeManager() {
    LOG_DBG_S("NavigationModeManager destroying");
}

void NavigationModeManager::initializeControllers() {
    m_gestureController = std::make_unique<NavigationController>(m_canvas, m_sceneManager);
    m_inventorController = std::make_unique<InventorNavigationController>(m_canvas, m_sceneManager);
    
    LOG_DBG_S("Navigation controllers initialized");
}

void NavigationModeManager::initializeNavigationStyles() {
    m_navigationStyles[NavigationStyle::GESTURE] = std::make_unique<GestureNavigationAdapter>(m_gestureController.get());
    m_navigationStyles[NavigationStyle::INVENTOR] = std::make_unique<InventorNavigationAdapter>(m_inventorController.get());
    m_navigationStyles[NavigationStyle::CAD] = std::make_unique<CADNavigationStyle>(m_canvas, m_sceneManager);
    m_navigationStyles[NavigationStyle::TOUCHPAD] = std::make_unique<TouchpadNavigationStyle>(m_canvas, m_sceneManager);
    m_navigationStyles[NavigationStyle::MAYA_GESTURE] = std::make_unique<MayaGestureNavigationStyle>(m_canvas, m_sceneManager);
    m_navigationStyles[NavigationStyle::BLENDER] = std::make_unique<BlenderNavigationStyle>(m_canvas, m_sceneManager);
    m_navigationStyles[NavigationStyle::REVIT] = std::make_unique<RevitNavigationStyle>(m_canvas, m_sceneManager);
    m_navigationStyles[NavigationStyle::TINKERCAD] = std::make_unique<TinkerCADNavigationStyle>(m_canvas, m_sceneManager);
    
    LOG_DBG_S("Navigation styles initialized (8 styles)");
}

INavigationStyle* NavigationModeManager::getNavigationStyleFor(NavigationStyle style) {
    auto it = m_navigationStyles.find(style);
    if (it != m_navigationStyles.end()) {
        return it->second.get();
    }
    
    switch (style) {
    case NavigationStyle::GESTURE:
        return m_navigationStyles[NavigationStyle::GESTURE].get();
    case NavigationStyle::INVENTOR:
        return m_navigationStyles[NavigationStyle::INVENTOR].get();
    case NavigationStyle::CAD:
        return m_navigationStyles[NavigationStyle::CAD].get();
    default:
        LOG_DBG_S("Unsupported navigation style: " + std::to_string(static_cast<int>(style)) + ", falling back to GESTURE");
        return m_navigationStyles[NavigationStyle::GESTURE].get();
    }
}

const INavigationStyle* NavigationModeManager::getNavigationStyleFor(NavigationStyle style) const {
    auto it = m_navigationStyles.find(style);
    if (it != m_navigationStyles.end()) {
        return it->second.get();
    }
    
    auto gestureIt = m_navigationStyles.find(NavigationStyle::GESTURE);
    if (gestureIt != m_navigationStyles.end()) {
        switch (style) {
        case NavigationStyle::INVENTOR:
            {
                auto inventorIt = m_navigationStyles.find(NavigationStyle::INVENTOR);
                if (inventorIt != m_navigationStyles.end()) {
                    return inventorIt->second.get();
                }
            }
            break;
        default:
            LOG_DBG_S("Unsupported navigation style: " + std::to_string(static_cast<int>(style)) + ", falling back to GESTURE");
            break;
        }
        return gestureIt->second.get();
    }
    
    return nullptr;
}

void NavigationModeManager::setNavigationStyle(NavigationStyle style) {
    if (m_currentStyle != style) {
        m_currentStyle = style;
        LOG_DBG_S("Navigation style changed to: " + std::to_string(static_cast<int>(style)));
        saveNavigationStyleToConfig();
    }
}

NavigationStyle NavigationModeManager::getNavigationStyle() const {
    return m_currentStyle;
}

void NavigationModeManager::handleMouseButton(wxMouseEvent& event) {
    INavigationStyle* style = getNavigationStyleFor(m_currentStyle);
    if (style) {
        style->handleMouseButton(event);
    } else {
        switch (m_currentStyle) {
        case NavigationStyle::GESTURE:
            if (m_gestureController) m_gestureController->handleMouseButton(event);
            break;
        case NavigationStyle::INVENTOR:
            if (m_inventorController) m_inventorController->handleMouseButton(event);
            break;
        case NavigationStyle::CAD:
            break;
        default:
            break;
        }
    }
}

void NavigationModeManager::handleMouseMotion(wxMouseEvent& event) {
    INavigationStyle* style = getNavigationStyleFor(m_currentStyle);
    if (style) {
        style->handleMouseMotion(event);
    } else {
        switch (m_currentStyle) {
        case NavigationStyle::GESTURE:
            if (m_gestureController) m_gestureController->handleMouseMotion(event);
            break;
        case NavigationStyle::INVENTOR:
            if (m_inventorController) m_inventorController->handleMouseMotion(event);
            break;
        }
    }
}

void NavigationModeManager::handleMouseWheel(wxMouseEvent& event) {
    INavigationStyle* style = getNavigationStyleFor(m_currentStyle);
    if (style) {
        style->handleMouseWheel(event);
    } else {
        switch (m_currentStyle) {
        case NavigationStyle::GESTURE:
            if (m_gestureController) m_gestureController->handleMouseWheel(event);
            break;
        case NavigationStyle::INVENTOR:
            if (m_inventorController) m_inventorController->handleMouseWheel(event);
            break;
        }
    }
}

void NavigationModeManager::viewAll() {
    INavigationStyle* style = getNavigationStyleFor(m_currentStyle);
    if (style) {
        style->viewAll();
    } else {
        switch (m_currentStyle) {
        case NavigationStyle::GESTURE:
            if (m_gestureController) m_gestureController->viewAll();
            break;
        case NavigationStyle::INVENTOR:
            if (m_inventorController) m_inventorController->viewAll();
            break;
        }
    }
}

void NavigationModeManager::viewTop() {
    INavigationStyle* style = getNavigationStyleFor(m_currentStyle);
    if (style) {
        style->viewTop();
    } else {
        switch (m_currentStyle) {
        case NavigationStyle::GESTURE:
            if (m_gestureController) m_gestureController->viewTop();
            break;
        case NavigationStyle::INVENTOR:
            if (m_inventorController) m_inventorController->viewTop();
            break;
        }
    }
}

void NavigationModeManager::viewFront() {
    INavigationStyle* style = getNavigationStyleFor(m_currentStyle);
    if (style) {
        style->viewFront();
    } else {
        switch (m_currentStyle) {
        case NavigationStyle::GESTURE:
            if (m_gestureController) m_gestureController->viewFront();
            break;
        case NavigationStyle::INVENTOR:
            if (m_inventorController) m_inventorController->viewFront();
            break;
        }
    }
}

void NavigationModeManager::viewRight() {
    INavigationStyle* style = getNavigationStyleFor(m_currentStyle);
    if (style) {
        style->viewRight();
    } else {
        switch (m_currentStyle) {
        case NavigationStyle::GESTURE:
            if (m_gestureController) m_gestureController->viewRight();
            break;
        case NavigationStyle::INVENTOR:
            if (m_inventorController) m_inventorController->viewRight();
            break;
        }
    }
}

void NavigationModeManager::viewIsometric() {
    INavigationStyle* style = getNavigationStyleFor(m_currentStyle);
    if (style) {
        style->viewIsometric();
    } else {
        switch (m_currentStyle) {
        case NavigationStyle::GESTURE:
            if (m_gestureController) m_gestureController->viewIsometric();
            break;
        case NavigationStyle::INVENTOR:
            if (m_inventorController) m_inventorController->viewIsometric();
            break;
        }
    }
}

void NavigationModeManager::setZoomSpeedFactor(float factor) {
    INavigationStyle* style = getNavigationStyleFor(m_currentStyle);
    if (style) {
        style->setZoomSpeedFactor(factor);
    }
    if (m_gestureController) m_gestureController->setZoomSpeedFactor(factor);
    if (m_inventorController) m_inventorController->setZoomSpeedFactor(factor);
}

float NavigationModeManager::getZoomSpeedFactor() const {
    const INavigationStyle* style = getNavigationStyleFor(m_currentStyle);
    if (style) {
        return style->getZoomSpeedFactor();
    }
    switch (m_currentStyle) {
    case NavigationStyle::GESTURE:
        return m_gestureController ? m_gestureController->getZoomSpeedFactor() : 1.0f;
    case NavigationStyle::INVENTOR:
        return m_inventorController ? m_inventorController->getZoomSpeedFactor() : 1.0f;
    }
    return 1.0f;
}

NavigationController* NavigationModeManager::getCurrentController() {
    switch (m_currentStyle) {
    case NavigationStyle::GESTURE:
        return m_gestureController.get();
    case NavigationStyle::INVENTOR:
        return nullptr; // Inventor controller is different type
    }
    return nullptr;
}

InventorNavigationController* NavigationModeManager::getInventorController() {
    return m_inventorController.get();
}

INavigationStyle* NavigationModeManager::getCurrentNavigationStyle() {
    return getNavigationStyleFor(m_currentStyle);
}

std::string NavigationModeManager::getCurrentStyleName() const {
    const INavigationStyle* style = getNavigationStyleFor(m_currentStyle);
    return style ? style->getStyleName() : "Unknown";
}

std::string NavigationModeManager::getCurrentStyleDescription() const {
    const INavigationStyle* style = getNavigationStyleFor(m_currentStyle);
    return style ? style->getStyleDescription() : "";
}

std::vector<std::pair<NavigationStyle, std::string>> NavigationModeManager::getAvailableStyles() const {
    std::vector<std::pair<NavigationStyle, std::string>> styles;
    styles.push_back({NavigationStyle::GESTURE, "Gesture - Simple gesture-based navigation - Left: Rotate, Right: Pan, Wheel: Zoom"});
    styles.push_back({NavigationStyle::INVENTOR, "Inventor - Open Inventor style with rotation center and spin continuation"});
    styles.push_back({NavigationStyle::CAD, "CAD - Professional CAD navigation - Left: Rotate, Middle: Pan, Right: Zoom, Wheel: Zoom"});
    styles.push_back({NavigationStyle::TOUCHPAD, "Touchpad - Optimized for touchpad devices - Left: Rotate, Ctrl+Left: Pan, Right: Zoom, Wheel: Zoom"});
    styles.push_back({NavigationStyle::MAYA_GESTURE, "Maya Gesture - Maya-style navigation - Alt+Left: Rotate, Alt+Middle: Pan, Alt+Right: Zoom, Wheel: Zoom"});
    styles.push_back({NavigationStyle::BLENDER, "Blender - Blender-style navigation - Middle: Rotate, Shift+Middle: Pan, Ctrl+Middle: Zoom, Wheel: Zoom"});
    styles.push_back({NavigationStyle::REVIT, "Revit - Revit-style navigation - Shift+Right: Rotate, Middle: Pan, Ctrl+Right: Zoom, Wheel: Zoom"});
    styles.push_back({NavigationStyle::TINKERCAD, "TinkerCAD - TinkerCAD-style navigation - Right: Rotate, Middle: Pan, Wheel: Zoom"});
    return styles;
}

void NavigationModeManager::loadNavigationStyleFromConfig() {
    try {
        auto& config = ConfigManager::getInstance();
        int styleInt = config.getInt("Navigation", "Style", static_cast<int>(NavigationStyle::GESTURE));
        
        if (styleInt >= 0 && styleInt <= static_cast<int>(NavigationStyle::TINKERCAD)) {
            NavigationStyle style = static_cast<NavigationStyle>(styleInt);
            
            if (m_navigationStyles.find(style) != m_navigationStyles.end()) {
                m_currentStyle = style;
                LOG_DBG_S("Navigation style loaded from config: " + std::to_string(styleInt) + " (" + getCurrentStyleName() + ")");
            } else {
                LOG_DBG_S("Navigation style from config not available: " + std::to_string(styleInt) + ", using default GESTURE");
                m_currentStyle = NavigationStyle::GESTURE;
            }
        } else {
            LOG_DBG_S("Invalid navigation style value in config: " + std::to_string(styleInt) + ", using default GESTURE");
            m_currentStyle = NavigationStyle::GESTURE;
        }
    }
    catch (const std::exception& e) {
        LOG_DBG_S("Failed to load navigation style from config: " + std::string(e.what()) + ", using default GESTURE");
        m_currentStyle = NavigationStyle::GESTURE;
    }
}

void NavigationModeManager::saveNavigationStyleToConfig() const {
    try {
        auto& config = ConfigManager::getInstance();
        config.setInt("Navigation", "Style", static_cast<int>(m_currentStyle));
        config.save();
        LOG_DBG_S("Navigation style saved to config: " + std::to_string(static_cast<int>(m_currentStyle)));
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to save navigation style to config: " + std::string(e.what()));
    }
}
