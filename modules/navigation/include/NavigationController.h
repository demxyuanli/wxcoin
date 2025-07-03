#pragma once

#include <wx/event.h>
#include <Inventor/SbLinear.h>

class Canvas;
class SceneManager;

class NavigationController {
public:
    NavigationController(Canvas* canvas, SceneManager* sceneManager);
    ~NavigationController();

    void handleMouseButton(wxMouseEvent& event);
    void handleMouseMotion(wxMouseEvent& event);
    void handleMouseWheel(wxMouseEvent& event);

    void viewAll();
    void viewTop();
    void viewFront();
    void viewRight();
    void viewIsometric();

private:
    void rotateCamera(const wxPoint& currentPos, const wxPoint& lastPos);
    void panCamera(const wxPoint& currentPos, const wxPoint& lastPos);
    void zoomCamera(float delta);

    Canvas* m_canvas;
    SceneManager* m_sceneManager;
    bool m_isDragging;
    wxPoint m_lastMousePos;
    enum class DragMode { NONE, ROTATE, PAN, ZOOM };
    DragMode m_dragMode;
};