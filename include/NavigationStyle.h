#pragma once

#include <Inventor/nodes/SoCamera.h>
#include <Inventor/SbLinear.h>
#include <wx/event.h>

class Canvas;

class NavigationStyle {
public:
    NavigationStyle(Canvas* canvas);
    ~NavigationStyle();

    void handleMouseButton(const wxMouseEvent& event);
    void handleMouseMotion(const wxMouseEvent& event);
    void handleMouseWheel(const wxMouseEvent& event);

    // View preset methods
    void viewAll();
    void viewTop();
    void viewFront();
    void viewRight();
    void viewIsometric();

    // Sensitivity control methods
    void setRotationSensitivity(float sensitivity) { m_rotationSensitivity = sensitivity; }
    void setPanSensitivity(float sensitivity) { m_panSensitivity = sensitivity; }
    void setZoomSensitivity(float sensitivity) { m_zoomSensitivity = sensitivity; }

    float getRotationSensitivity() const { return m_rotationSensitivity; }
    float getPanSensitivity() const { return m_panSensitivity; }
    float getZoomSensitivity() const { return m_zoomSensitivity; }

private:
    Canvas* m_canvas;
    bool m_isRotating;
    bool m_isPanning;
    wxPoint m_lastMousePos;
    float m_rotationSensitivity;
    float m_panSensitivity;
    float m_zoomSensitivity;

    void rotateCamera(const wxPoint& currentPos);
    void panCamera(const wxPoint& currentPos);
    void zoomCamera(int delta);
}; 