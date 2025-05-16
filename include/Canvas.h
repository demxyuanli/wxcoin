#pragma once

#include <wx/glcanvas.h>
#include <memory>
#include <stdexcept>
#include <Inventor/nodes/SoCamera.h>
#include "NavigationCube.h"

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
    void setNavigationCubeEnabled(bool enabled);
    bool isNavigationCubeEnabled() const;

    void setObjectTreePanel(ObjectTreePanel* panel) { m_objectTreePanel = panel; }
    void setCommandManager(CommandManager* manager) { m_commandManager = manager; }
    void SetNavigationCubeRect(int x, int y, int size);

    SoCamera* getCamera() const;
    void resetView();

private:
    // Navigation cube layout management
    struct NavigationCubeLayout {
        int x{ 10 }, y{ 10 }, size{ 120 };
        void update(int newX, int newY, int newSize, const wxSize& windowSize, float dpiScale) {
            // Restrict size, considering DPI scaling
            size = std::max(50, std::min(newSize, windowSize.x / 2));
            size = static_cast<int>(size * dpiScale);
            // Ensure position is within window bounds
            x = std::max(0, std::min(newX, static_cast<int>((windowSize.x - size) / dpiScale)));
            y = std::max(0, std::min(newY, static_cast<int>((windowSize.y - size) / dpiScale)));
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
    std::unique_ptr<NavigationCube> m_navCube;
    ObjectTreePanel* m_objectTreePanel;
    CommandManager* m_commandManager;
    bool m_isRendering;
    bool m_isInitialized;
    wxLongLong m_lastRenderTime;
    float m_dpiScale; // Store DPI scale for consistent use

    DECLARE_EVENT_TABLE()
};