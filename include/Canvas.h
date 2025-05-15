#pragma once

#include <wx/glcanvas.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoTransform.h>
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
    virtual ~Canvas();

    // Scene management
    bool initScene();
    void cleanup();
    void resetView();
    void toggleCameraMode();
    SoSeparator* getObjectRoot() { return m_objectRoot; }
    SoCamera* getCamera() { return m_camera; }

    // Rendering
    void render(bool fastMode = false);
    void onPaint(wxPaintEvent& event);
    void onSize(wxSizeEvent& event);
    void onEraseBackground(wxEraseEvent& event);

    // Input handling
    void setMouseHandler(std::unique_ptr<MouseHandler> mouseHandler);
    void setMouseHandler(MouseHandler* mouseHandler);
    void setNavigationStyle(std::unique_ptr<NavigationStyle> navStyle);
    void setNavigationStyle(NavigationStyle* navStyle);
    void onMouseButton(wxMouseEvent& event);
    void onMouseMotion(wxMouseEvent& event);
    void onMouseWheel(wxMouseEvent& event);
    bool screenToWorld(const wxPoint& screenPos, SbVec3f& worldPos);

    // Geometry creation
    void CreateGeometryAtPosition(const SbVec3f& position);
    std::string getCreationGeometryType() const;

    // Picking aid lines
    void createPickingAidLines();
    void showPickingAidLines(const SbVec3f& position);
    void hidePickingAidLines();
    void setPickingCursor(bool enable);

    // Test methods
    void AddTestCube();
    void createCoordinateSystem();

    MouseHandler* GetMouseHandler() const { return m_mouseHandler.get(); }

private:
    static const int s_canvasAttribs[];
    static const int RENDER_INTERVAL;
    static const int MOTION_INTERVAL;
    static const float COORD_PLANE_SIZE;
    static const float COORD_PLANE_TRANSPARENCY;

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

    // Picking aid lines
    SoSeparator* m_pickingAidSeparator;
    SoTransform* m_pickingAidTransform;
    bool m_pickingAidVisible;

    DECLARE_EVENT_TABLE()
};