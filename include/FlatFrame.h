#pragma once

#include "flatui/FlatUIFrame.h"
#include "flatui/FlatUIBar.h"
#include "flatui/FlatUIHomeMenu.h"
#include <wx/srchctrl.h>
#include <wx/textctrl.h>
#include <wx/panel.h>
#include <wx/aui/aui.h>
#include <memory>

// Forward declarations
class Canvas;
class PropertyPanel;
class ObjectTreePanel;
class MouseHandler;
class GeometryFactory;
class CommandManager;
class OCCViewer;
class CommandDispatcher;
class CommandListenerManager;
struct CommandResult;
class wxSplitterWindow;

// ID definitions
enum {
    ID_SearchExecute = wxID_HIGHEST + 1000,
    ID_UserProfile,
    ID_ToggleFunctionSpace,
    ID_ToggleProfileSpace,
    ID_Menu_NewProject_MainFrame,
    ID_Menu_OpenProject_MainFrame,
    ID_ShowUIHierarchy,
    ID_Menu_PrintLayout_MainFrame,
    ID_VIEW_SHOWEDGES,
    ID_IMPORT_STEP,
    ID_CREATE_BOX,
    ID_CREATE_SPHERE,
    ID_CREATE_CYLINDER,
    ID_CREATE_CONE,
    ID_CREATE_TORUS,
    ID_CREATE_TRUNCATED_CYLINDER,
    ID_CREATE_WRENCH,
    ID_VIEW_ALL,
    ID_VIEW_TOP,
    ID_VIEW_FRONT,
    ID_VIEW_RIGHT,
    ID_VIEW_ISOMETRIC,
    ID_SHOW_NORMALS,
    ID_SHOW_FACES,
    ID_FIX_NORMALS,
    ID_SET_TRANSPARENCY,
    ID_TOGGLE_WIREFRAME,
    ID_TOGGLE_SHADING,
    ID_TOGGLE_EDGES,

    ID_UNDO,
    ID_REDO,
    ID_NAVIGATION_CUBE_CONFIG,
    ID_ZOOM_SPEED,
    ID_MESH_QUALITY_DIALOG,
    ID_RENDERING_SETTINGS,
    ID_EDGE_SETTINGS,
    ID_LIGHTING_SETTINGS,
    ID_SAVE_AS,
    ID_LOD_ENABLE,
    ID_LOD_CONFIG,
    
    // Texture Mode Command IDs
    ID_TEXTURE_MODE_DECAL,
    ID_TEXTURE_MODE_MODULATE,
    ID_TEXTURE_MODE_REPLACE,
    ID_TEXTURE_MODE_BLEND,
    
    // Coordinate System Control
    ID_TOGGLE_COORDINATE_SYSTEM
};

class FlatFrame : public FlatUIFrame
{
public:
    FlatFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
    virtual ~FlatFrame();

    // Override methods from FlatUIFrame
    virtual void OnLeftDown(wxMouseEvent& event) override;
    virtual void OnMotion(wxMouseEvent& event) override;
    virtual wxWindow* GetFunctionSpaceControl() const override;
    virtual wxWindow* GetProfileSpaceControl() const override;
    virtual FlatUIBar* GetUIBar() const override;

private:
    // UI components
    FlatUIBar* m_ribbon;
    wxTextCtrl* m_messageOutput;
    wxSearchCtrl* m_searchCtrl;
    FlatUIHomeMenu* m_homeMenu;
    wxPanel* m_searchPanel;
    wxPanel* m_profilePanel;
    FlatUIStatusBar* m_statusBar;

    wxAuiManager m_auiManager;

    // Splitters for layout
    wxSplitterWindow* m_mainSplitter;
    wxSplitterWindow* m_leftSplitter;

    // CAD components from MainFrame
    Canvas* m_canvas;
    PropertyPanel* m_propertyPanel;
    ObjectTreePanel* m_objectTreePanel; 
    MouseHandler* m_mouseHandler;
    GeometryFactory* m_geometryFactory;
    CommandManager* m_commandManager;
    OCCViewer* m_occViewer;
    std::unique_ptr<CommandDispatcher> m_commandDispatcher;
    std::unique_ptr<CommandListenerManager> m_listenerManager;
    bool m_isFirstActivate;

    // Methods
    void InitializeUI(const wxSize& size);
    void LoadSVGIcons(wxWindow* parent, wxSizer* sizer);
    wxBitmap LoadHighQualityBitmap(const wxString& resourceName, const wxSize& targetSize);
    void ShowUIHierarchy();
    
    // CAD methods from MainFrame
    void createPanels();
    void setupCommandSystem();
    void onCommand(wxCommandEvent& event);
    void onCommandFeedback(const CommandResult& result);
    void onClose(wxCloseEvent& event);
    void onActivate(wxActivateEvent& event);
    void onSize(wxSizeEvent& event);

    // Event handlers
    void OnButtonClick(wxCommandEvent& event);
    void OnMenuNewProject(wxCommandEvent& event);
    void OnMenuOpenProject(wxCommandEvent& event);
    void OnMenuExit(wxCommandEvent& event);
    void OnStartupTimer(wxTimerEvent& event);
    void OnSearchExecute(wxCommandEvent& event);
    void OnSearchTextEnter(wxCommandEvent& event);
    void OnUserProfile(wxCommandEvent& event);
    void OnSettings(wxCommandEvent& event);
    void OnToggleFunctionSpace(wxCommandEvent& event);
    void OnToggleProfileSpace(wxCommandEvent& event);
    void OnShowUIHierarchy(wxCommandEvent& event);
    void PrintUILayout(wxCommandEvent& event);
    void OnThemeChanged(wxCommandEvent& event);
    
    
    // Override OnGlobalPinStateChanged to handle main work area layout
    virtual void OnGlobalPinStateChanged(wxCommandEvent& event) override;

    DECLARE_EVENT_TABLE()
}; 