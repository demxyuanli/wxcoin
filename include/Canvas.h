#pragma once

#include <wx/glcanvas.h>
#include <memory>
#include <Inventor/nodes/SoCamera.h>
#include "interfaces/ICanvas.h"
#include "interfaces/IViewportManager.h"
#include "interfaces/IMultiViewportManager.h"
#include "interfaces/IViewRefresher.h"

// Forward declarations
class OCCViewer;
class SceneManager;
class InputManager;
class ObjectTreePanel;
class CommandManager;
class NavigationCubeManager;
class RenderingEngine;
class EventCoordinator;
class IViewportManager;
class IMultiViewportManager;
class ViewportManager;        // concrete, kept as member type
class MultiViewportManager;   // concrete, kept as member type
class ViewRefreshManager;

class Canvas : public wxGLCanvas, public ICanvas {
public:
    Canvas(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);
    virtual ~Canvas();

    // Core rendering interface
    void render(bool fastMode = false) override;

    // UI state management
    void setPickingCursor(bool enable);
    void showErrorDialog(const std::string& message) const;

    // Access to subsystems
    SceneManager* getSceneManager() const { return m_sceneManager.get(); }
    InputManager* getInputManager() const { return m_inputManager.get(); }
    ObjectTreePanel* getObjectTreePanel() const { return m_objectTreePanel; }
    CommandManager* getCommandManager() const { return m_commandManager; }
    RenderingEngine* getRenderingEngine() const { return m_renderingEngine.get(); }
    IRenderingEngine* getRenderingEngineInterface() const override { return reinterpret_cast<IRenderingEngine*>(m_renderingEngine.get()); }
    IViewportManager* getViewportManager() const { return reinterpret_cast<IViewportManager*>(m_viewportManager.get()); }

    // Navigation cube methods (delegated to NavigationCubeManager)
    void setNavigationCubeEnabled(bool enabled);
    bool isNavigationCubeEnabled() const;
    void ShowNavigationCubeConfigDialog();

    // External dependencies
    void setObjectTreePanel(ObjectTreePanel* panel) { m_objectTreePanel = panel; }
    void setCommandManager(CommandManager* manager) { m_commandManager = manager; }
    void setOCCViewer(class OCCViewer* occViewer) { m_occViewer = occViewer; }
    OCCViewer* getOCCViewer() const { return m_occViewer; }

    // Scene access shortcuts
    SoCamera* getCamera() const;
    ISceneManager* getSceneManagerInterface() const override { return reinterpret_cast<ISceneManager*>(m_sceneManager.get()); }
    void resetView();

    // DPI access shortcuts
    float getDPIScale() const;

    // Refresh management
    ViewRefreshManager* getRefreshManager() const { return m_refreshManager.get(); }
    IViewRefresher* getViewRefresher() const { return reinterpret_cast<IViewRefresher*>(m_refreshManager.get()); }
    // Optional injection entry
    static void SetSubsystemFactory(class ISubsystemFactory* factory);
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

    // Multi-viewport methods
    void setMultiViewportEnabled(bool enabled);
    bool isMultiViewportEnabled() const;
    IMultiViewportManager* getMultiViewportManager() const { return reinterpret_cast<IMultiViewportManager*>(m_multiViewportManager.get()); }

private:
    std::unique_ptr<MultiViewportManager> m_multiViewportManager;
    bool m_multiViewportEnabled;
    std::unique_ptr<ViewRefreshManager> m_refreshManager;

    DECLARE_EVENT_TABLE()

};
