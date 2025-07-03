#pragma once

#include <wx/glcanvas.h>
#include <memory>
#include <stdexcept>
#include <Inventor/nodes/SoCamera.h>
#include "CuteNavCube.h"
#include "DPIManager.h"

class SceneManager;
class InputManager;
class ObjectTreePanel;
class CommandManager;

class Canvas : public wxGLCanvas {
public:
    static const int RENDER_INTERVAL;
    Canvas(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);
    virtual ~Canvas();

    void render(bool fastMode = false);
    void setPickingCursor(bool enable);

    SceneManager* getSceneManager() const { return m_sceneManager.get(); }
    InputManager* getInputManager() const { return m_inputManager.get(); }
    ObjectTreePanel* getObjectTreePanel() const { return m_objectTreePanel; }
    CommandManager* getCommandManager() const { return m_commandManager; }
    // Navigation cube methods (now using CuteNavCube)
    void setNavigationCubeEnabled(bool enabled);
    bool isNavigationCubeEnabled() const;
    void SetNavigationCubeRect(int x, int y, int size);
    void SyncNavigationCubeCamera();

    void setObjectTreePanel(ObjectTreePanel* panel) { m_objectTreePanel = panel; }
    void setCommandManager(CommandManager* manager) { m_commandManager = manager; }
    void SetNavigationCubeColor(const wxColour& color);
    void SetNavigationCubeViewportSize(int size);
    void ShowNavigationCubeConfigDialog();
    void SyncMainCameraToNavigationCube();

    SoCamera* getCamera() const;
    void resetView();
    
    // High-DPI support methods
    void updateDPISettings();
    float getDPIScale() const { return m_dpiScale; }
    void applyDPIScalingToUI();

private:
    // Layout management for navigation cube and mini scene
    struct Layout {
        int x{ 20 }, y{ 20 }, size{ 200 }; // These will be kept as logical coordinates and size

        // Parameters:
        // newX_logical, newY_logical: Desired logical top-left position
        // newSize_logical: Desired logical size (e.g., width/height)
        // windowSize_logical: Current logical size of the parent window
        // dpiScale: Current DPI scale factor (passed but not stored if members are purely logical)
        void update(int newX_logical, int newY_logical, int newSize_logical,
                    const wxSize& windowSize_logical, float dpiScale)
        {
            // Determine and store logical size for this layout object
            size = std::max(100, std::min(newSize_logical, windowSize_logical.x / 2));
            size = std::max(100, std::min(size, windowSize_logical.y / 2)); // Ensure it also fits height-wise if square

            // Determine and store logical top-left position
            // Ensure the cube/scene does not go out of bounds
            x = std::max(0, std::min(newX_logical, windowSize_logical.x - size));
            y = std::max(0, std::min(newY_logical, windowSize_logical.y - size));
        }
    } m_cubeLayout;

    void onPaint(wxPaintEvent& event);
    void onSize(wxSizeEvent& event);
    void onEraseBackground(wxEraseEvent& event);
    void onMouseEvent(wxMouseEvent& event);

    void showErrorDialog(const std::string& message) const;

    static const int s_canvasAttribs[];
    wxGLContext* m_glContext;
    std::unique_ptr<SceneManager> m_sceneManager;
    std::unique_ptr<InputManager> m_inputManager;
    std::unique_ptr<CuteNavCube> m_navCube;
    ObjectTreePanel* m_objectTreePanel;
    CommandManager* m_commandManager;
    bool m_isRendering;
    bool m_isInitialized;
    wxLongLong m_lastRenderTime;
    float m_dpiScale;
    bool m_enableNavCube; // Initial enable state for navigation cube
    float m_marginx = 20.0f;
    float m_marginy = 20.0f;

    DECLARE_EVENT_TABLE()
};