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

class MainFrame : public wxFrame {
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
    void onExit(wxCommandEvent& event);
    void onUndo(wxCommandEvent& event);
    void onRedo(wxCommandEvent& event);
    void onCreateBox(wxCommandEvent& event);
    void onCreateSphere(wxCommandEvent& event);
    void onCreateCylinder(wxCommandEvent& event);
    void onCreateCone(wxCommandEvent& event);
    void onViewMode(wxCommandEvent& event);
    void onSelectMode(wxCommandEvent& event);
    void onViewAll(wxCommandEvent& event);
    void onViewTop(wxCommandEvent& event);
    void onViewFront(wxCommandEvent& event);
    void onViewRight(wxCommandEvent& event);
    void onViewIsometric(wxCommandEvent& event);
    void onNavigationCubeConfig(wxCommandEvent& event);
    void onAbout(wxCommandEvent& event);
    void onClose(wxCloseEvent& event);

    wxAuiManager m_auiManager;
    Canvas* m_canvas;
    MouseHandler* m_mouseHandler;
    GeometryFactory* m_geometryFactory;
    CommandManager* m_commandManager;

    DECLARE_EVENT_TABLE()
};