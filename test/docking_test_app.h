#pragma once

#include <wx/wx.h>
#include <wx/artprov.h>
#include <wx/stc/stc.h>
#include <wx/listctrl.h>
#include <wx/treectrl.h>
#include <wx/grid.h>
#include "docking/DockManager.h"
#include "docking/DockWidget.h"
#include "docking/PerspectiveManager.h"

namespace ads {

/**
 * @brief Main application class for testing the docking system
 */
class DockingTestApp : public wxApp {
public:
    virtual bool OnInit() override;
};

/**
 * @brief Main frame window for the docking test
 */
class DockingTestFrame : public wxFrame {
public:
    DockingTestFrame();
    virtual ~DockingTestFrame();

private:
    // UI creation methods
    void CreateMenuBar();
    void CreateToolBar();
    void CreateStatusBar();
    void CreateDockingSystem();
    
    // Create different types of dock widgets for testing
    DockWidget* CreateEditorWidget(const wxString& title, int id);
    DockWidget* CreateTreeWidget(const wxString& title);
    DockWidget* CreateListWidget(const wxString& title);
    DockWidget* CreatePropertyGridWidget(const wxString& title);
    DockWidget* CreateOutputWidget(const wxString& title);
    DockWidget* CreateToolboxWidget(const wxString& title);
    
    // Menu event handlers
    void OnFileNew(wxCommandEvent& event);
    void OnFileOpen(wxCommandEvent& event);
    void OnFileSave(wxCommandEvent& event);
    void OnFileExit(wxCommandEvent& event);
    
    void OnViewSaveLayout(wxCommandEvent& event);
    void OnViewLoadLayout(wxCommandEvent& event);
    void OnViewResetLayout(wxCommandEvent& event);
    void OnViewManagePerspectives(wxCommandEvent& event);
    
    void OnDockingAddEditor(wxCommandEvent& event);
    void OnDockingAddTool(wxCommandEvent& event);
    void OnDockingShowAll(wxCommandEvent& event);
    void OnDockingHideAll(wxCommandEvent& event);
    void OnDockingToggleAutoHide(wxCommandEvent& event);
    
    void OnHelpAbout(wxCommandEvent& event);
    
    // Test different docking features
    void TestBasicDocking();
    void TestTabbing();
    void TestFloating();
    void TestAutoHide();
    void TestPerspectives();
    void TestSplitting();
    
    // Member variables
    DockManager* m_dockManager;
    int m_editorCounter;
    
    wxDECLARE_EVENT_TABLE();
};

/**
 * @brief Sample editor widget using wxStyledTextCtrl
 */
class EditorWidget : public wxStyledTextCtrl {
public:
    EditorWidget(wxWindow* parent, int id);
    void LoadFile(const wxString& filename);
    void SaveFile(const wxString& filename);
    void SetupStyling();
};

/**
 * @brief Property grid widget for testing
 */
class PropertyWidget : public wxPanel {
public:
    PropertyWidget(wxWindow* parent);
    void PopulateProperties();
    
private:
    wxListCtrl* m_list;
};

} // namespace ads