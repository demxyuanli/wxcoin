#pragma once

#include <wx/glcanvas.h>
#include <memory>
#include <Inventor/nodes/SoCamera.h>

// Forward declarations
class SceneManager;
class InputManager;
class ObjectTreePanel;
class CommandManager;
class NavigationCubeManager;
class RenderingEngine;
class EventCoordinator;
class ViewportManager;

class Canvas : public wxGLCanvas {
public:
    Canvas(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);
    virtual ~Canvas();

    // Core rendering interface
    void render(bool fastMode = false);
    
    // UI state management
    void setPickingCursor(bool enable);
    void showErrorDialog(const std::string& message) const;

    // Access to subsystems
    SceneManager* getSceneManager() const { return m_sceneManager.get(); }
    InputManager* getInputManager() const { return m_inputManager.get(); }
    ObjectTreePanel* getObjectTreePanel() const { return m_objectTreePanel; }
    CommandManager* getCommandManager() const { return m_commandManager; }
    RenderingEngine* getRenderingEngine() const { return m_renderingEngine.get(); }
    ViewportManager* getViewportManager() const { return m_viewportManager.get(); }

    // Navigation cube methods (delegated to NavigationCubeManager)
    void setNavigationCubeEnabled(bool enabled);
    bool isNavigationCubeEnabled() const;
    void ShowNavigationCubeConfigDialog();

    // External dependencies
    void setObjectTreePanel(ObjectTreePanel* panel) { m_objectTreePanel = panel; }
    void setCommandManager(CommandManager* manager) { m_commandManager = manager; }

    // Scene access shortcuts
    SoCamera* getCamera() const;
    void resetView();

    // DPI access shortcuts
    float getDPIScale() const;

private:
    void initializeSubsystems();
    void connectSubsystems();
    
    // Event handlers
    void onPaint(wxPaintEvent& event);
    void onSize(wxSizeEvent& event);
    void onEraseBackground(wxEraseEvent& event);
    void onMouseEvent(wxMouseEvent& event);

    static const int s_canvasAttribs[];
    
    // Core subsystems
    std::unique_ptr<SceneManager> m_sceneManager;
    std::unique_ptr<InputManager> m_inputManager;
    std::unique_ptr<NavigationCubeManager> m_navigationCubeManager;
    std::unique_ptr<RenderingEngine> m_renderingEngine;
    std::unique_ptr<EventCoordinator> m_eventCoordinator;
    std::unique_ptr<ViewportManager> m_viewportManager;

    // External dependencies
    ObjectTreePanel* m_objectTreePanel;
    CommandManager* m_commandManager;

    DECLARE_EVENT_TABLE()
};