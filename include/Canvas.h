#pragma once

#include <wx/glcanvas.h>
#include <memory>
#include <Inventor/nodes/SoCamera.h>

// Forward declarations
class OCCViewer;
class SceneManager;
class InputManager;
class ObjectTreePanel;
class CommandManager;
class NavigationCubeManager;
class RenderingEngine;
class EventCoordinator;
class ViewportManager;
class MultiViewportManager;
class ViewRefreshManager;
class UnifiedRefreshSystem;
class CommandDispatcher;

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
    void setCommandDispatcher(CommandDispatcher* dispatcher) { m_commandDispatcher = dispatcher; }
    void setOCCViewer(class OCCViewer* occViewer);
    OCCViewer* getOCCViewer() const { return m_occViewer; }

    // Scene access shortcuts
    SoCamera* getCamera() const;
    void resetView();

    // DPI access shortcuts
    float getDPIScale() const;

    // Refresh management
    ViewRefreshManager* getRefreshManager() const { return m_refreshManager.get(); }
    UnifiedRefreshSystem* getUnifiedRefreshSystem() const { return m_unifiedRefreshSystem; }
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
    class OCCViewer* m_occViewer;
    CommandDispatcher* m_commandDispatcher;

    // Multi-viewport methods
    void setMultiViewportEnabled(bool enabled);
    bool isMultiViewportEnabled() const;
    MultiViewportManager* getMultiViewportManager() const { return m_multiViewportManager.get(); }

private:
    std::unique_ptr<MultiViewportManager> m_multiViewportManager;
    bool m_multiViewportEnabled;
    std::unique_ptr<ViewRefreshManager> m_refreshManager;
    UnifiedRefreshSystem* m_unifiedRefreshSystem;

    DECLARE_EVENT_TABLE()

};
