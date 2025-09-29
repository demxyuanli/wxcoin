#pragma once

#include <wx/event.h>
#include <Inventor/SbLinear.h>
#include <Inventor/nodes/SoSeparator.h>

class Canvas;
class SceneManager;

enum class InventorNavigationMode {
    IDLE = 0,
    DRAGGING,      // Left mouse button - rotate
    PANNING,       // Middle mouse button - pan
    ZOOMING,       // Left + Middle or Ctrl+Shift - zoom
    SELECTION      // Ctrl + Left or Shift + Left - selection
};

class InventorNavigationController {
public:
    InventorNavigationController(Canvas* canvas, SceneManager* sceneManager);
    ~InventorNavigationController();

    void handleMouseButton(wxMouseEvent& event);
    void handleMouseMotion(wxMouseEvent& event);
    void handleMouseWheel(wxMouseEvent& event);

    void viewAll();
    void viewTop();
    void viewFront();
    void viewRight();
    void viewIsometric();

    // Navigation mode control
    void setNavigationMode(InventorNavigationMode mode);
    InventorNavigationMode getNavigationMode() const;
    
    // Zoom speed adjustment
    void setZoomSpeedFactor(float factor);
    float getZoomSpeedFactor() const;

    // Rotation center management
    void setRotationCenter(const SbVec3f& center);
    void clearRotationCenter();
    bool hasRotationCenter() const;
    const SbVec3f& getRotationCenter() const;
    void pickRotationCenterAtMouse(const wxPoint& mousePos);

private:
    void updateNavigationMode();
    void rotateCamera(const wxPoint& currentPos, const wxPoint& lastPos);
    void panCamera(const wxPoint& currentPos, const wxPoint& lastPos);
    void zoomCamera(float delta);
    void zoomByCursor(const wxPoint& currentPos, const wxPoint& lastPos);
    void setupPanningPlane();
    void lookAtPoint(const wxPoint& pos);

    // Inventor-style camera operations
    void spin(const wxPoint& currentPos, const wxPoint& lastPos);
    bool doSpin();
    void addToLog(const wxPoint& pos, wxLongLong time);
    void moveCursorPosition();

    // Rotation center marker management
    void createRotationCenterMarker();
    void updateRotationCenterMarker();
    void hideRotationCenterMarker();

    // View plane utilities
    SbVec3f getPointOnViewPlane(const wxPoint& mousePos);

    // Configuration loading
    void loadMarkerConfig();

    Canvas* m_canvas;
    SceneManager* m_sceneManager;
    
    // Navigation state
    InventorNavigationMode m_currentMode;
    bool m_button1Down;
    bool m_button2Down;
    bool m_button3Down;
    bool m_ctrlDown;
    bool m_shiftDown;
    bool m_altDown;
    
    // Mouse tracking
    wxPoint m_lastMousePos;
    wxPoint m_baseMousePos;
    bool m_isDragging;
    bool m_hasDragged;
    bool m_hasPanned;
    bool m_hasZoomed;
    
    // Timing and thresholds
    wxLongLong m_centerTime;
    wxLongLong m_lastMotionTime;
    bool m_lockRecenter;
    float m_zoomSpeedFactor;
    
    // Movement logging for spin continuation
    struct MovementLog {
        wxPoint position;
        wxLongLong timestamp;
    };
    std::vector<MovementLog> m_movementLog;
    static const size_t MAX_MOVEMENT_LOG_SIZE = 5;

    // Rotation center management
    SbVec3f m_rotationCenter;
    bool m_hasRotationCenter;
    SoSeparator* m_rotationCenterMarker;

    // Rotation center marker configuration
    struct MarkerConfig {
        float radius = 0.15f;
        float transparency = 0.8f;
        float red = 1.0f;
        float green = 0.0f;
        float blue = 0.0f;
    } m_markerConfig;

    // Click vs drag detection
    wxPoint m_clickStartPos;
    bool m_isPotentialClick;
};
