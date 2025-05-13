#pragma once

#include <wx/glcanvas.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <memory>

// Dependencies:
// - wxWidgets 3.2 or later
// - Open Inventor (Coin3D) 4.0 or later
// - MouseHandler and NavigationStyle classes assumed available

class MouseHandler;
class NavigationStyle;

class Canvas : public wxGLCanvas {
    friend class NavigationStyle;

public:
    Canvas(wxWindow* parent, wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize);
    ~Canvas();

    // Unique_ptr versions (recommended)
    void setMouseHandler(std::unique_ptr<MouseHandler> mouseHandler);
    void setNavigationStyle(std::unique_ptr<NavigationStyle> navStyle);
    // Raw pointer versions for compatibility
    void setMouseHandler(MouseHandler* mouseHandler);
    void setNavigationStyle(NavigationStyle* navStyle);

    SoSeparator* getObjectRoot() const { return m_objectRoot; }
    SoCamera* getCamera() const { return m_camera; }
    void render(bool fastMode = false);
    void resetView();
    void toggleCameraMode();

private:
    bool initScene();
    void cleanup();
    void createCoordinateSystem();

    void onPaint(wxPaintEvent& event);
    void onSize(wxSizeEvent& event);
    void onEraseBackground(wxEraseEvent& event);
    void onMouseButton(wxMouseEvent& event);
    void onMouseMotion(wxMouseEvent& event);
    void onMouseWheel(wxMouseEvent& event);

    wxGLContext* m_glContext;
    SoSeparator* m_sceneRoot;
    SoCamera* m_camera;
    SoDirectionalLight* m_light;
    SoSeparator* m_objectRoot;
    std::unique_ptr<MouseHandler> m_mouseHandler;
    std::unique_ptr<NavigationStyle> m_navStyle;
    bool m_isRendering;
    wxLongLong m_lastRenderTime;
    bool m_isPerspectiveCamera;

    static const int s_canvasAttribs[];
    static const int RENDER_INTERVAL;
    static const int MOTION_INTERVAL;
    static const float COORD_PLANE_SIZE;
    static const float COORD_PLANE_TRANSPARENCY;

    DECLARE_EVENT_TABLE()
};