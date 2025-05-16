#pragma once

#include <wx/glcanvas.h>
#include <memory>
#include <Inventor/nodes/SoCamera.h>

class SceneManager;
class InputManager;
class ObjectTreePanel;
class CommandManager;
class NavigationCube;

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
    NavigationCube* getNavigationCube() const { return m_navigationCube.get(); }

    void setObjectTreePanel(ObjectTreePanel* panel) { m_objectTreePanel = panel; }
    void setCommandManager(CommandManager* manager) { m_commandManager = manager; }
    void setNavigationCubeEnabled(bool enabled);

    SoCamera* getCamera() const;
    void resetView();

private:
    void onPaint(wxPaintEvent& event);
    void onSize(wxSizeEvent& event);
    void onEraseBackground(wxEraseEvent& event);
    void onMouseEvent(wxMouseEvent& event);

    static const int s_canvasAttribs[];
    wxGLContext* m_glContext;
    std::unique_ptr<SceneManager> m_sceneManager;
    std::unique_ptr<InputManager> m_inputManager;
    std::unique_ptr<NavigationCube> m_navigationCube;
    ObjectTreePanel* m_objectTreePanel;
    CommandManager* m_commandManager;
    bool m_isRendering;
    wxLongLong m_lastRenderTime;

    DECLARE_EVENT_TABLE()
};