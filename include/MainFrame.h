#pragma once

#include <wx/frame.h>
#include <wx/aui/aui.h>

class Canvas;
class PropertyPanel;
class ObjectTreePanel;
class MouseHandler;
class GeometryFactory;
class CommandManager;
class NavigationController;
class OCCViewer;


enum
{
    // File menu IDs
    ID_NEW = wxID_HIGHEST + 1,
    ID_OPEN,
    ID_SAVE,
    ID_SAVE_AS,
    ID_IMPORT_STEP,
    ID_EXIT,

    // Edit menu IDs
    ID_UNDO,
    ID_REDO,

    // Create menu IDs
    ID_CREATE_BOX,
    ID_CREATE_SPHERE,
    ID_CREATE_CYLINDER,
    ID_CREATE_CONE,
    ID_CREATE_WRENCH,

    // View menu IDs
    ID_VIEW_MODE,
    ID_SELECT_MODE,
    ID_VIEW_ALL,        // Add missing ID
    ID_VIEW_TOP,        // Add missing ID
    ID_VIEW_FRONT,      // Add missing ID
    ID_VIEW_RIGHT,      // Add missing ID
    ID_VIEW_ISOMETRIC,  // Add missing ID

    // Display options
    ID_SHOW_NORMALS,
    ID_SHOW_EDGES,
    ID_FIX_NORMALS,
    ID_NAVIGATION_CUBE_CONFIG,

    // Help menu IDs
    ID_ABOUT
};

class MainFrame : public wxFrame
{
public:
    MainFrame(const wxString& title);
    virtual ~MainFrame();

private:
    void createStatusBar();
    void createPanels();
    void createMenu();
    void createToolbar();
    void updateUI();

    // Event handlers
    void onNew(wxCommandEvent& event);
    void onOpen(wxCommandEvent& event);
    void onSave(wxCommandEvent& event);
    void onSaveAs(wxCommandEvent& event);
    void onImportSTEP(wxCommandEvent& event);
    void onExit(wxCommandEvent& event);
    void onUndo(wxCommandEvent& event);
    void onRedo(wxCommandEvent& event);
    void onCreateBox(wxCommandEvent& event);
    void onCreateSphere(wxCommandEvent& event);
    void onCreateCylinder(wxCommandEvent& event);
    void onCreateCone(wxCommandEvent& event);
    void onCreateWrench(wxCommandEvent& event);
    void onViewMode(wxCommandEvent& event);
    void onSelectMode(wxCommandEvent& event);
    void onViewAll(wxCommandEvent& event);
    void onViewTop(wxCommandEvent& event);
    void onViewFront(wxCommandEvent& event);
    void onViewRight(wxCommandEvent& event);
    void onViewIsometric(wxCommandEvent& event);
    void onNavigationCubeConfig(wxCommandEvent& event);
    void onShowNormals(wxCommandEvent& event);
    void onShowEdges(wxCommandEvent& event);  // Add edge display event handler declaration
    void onFixNormals(wxCommandEvent& event);
    void onAbout(wxCommandEvent& event);
    void onClose(wxCloseEvent& event);

    wxAuiManager m_auiManager;
    Canvas* m_canvas;
    MouseHandler* m_mouseHandler;
    GeometryFactory* m_geometryFactory;
    CommandManager* m_commandManager;
    OCCViewer* m_occViewer;

    DECLARE_EVENT_TABLE()
};

