#pragma once

#include <wx/wx.h>
#include "docking/DockManager.h"

namespace ads {

/**
 * @brief Example frame demonstrating the Qt-Advanced-Docking-System functionality
 * 
 * This class shows how to use the docking system in a wxWidgets application.
 * It creates a main window with multiple dockable panels that can be:
 * - Dragged and dropped to different positions
 * - Tabbed together
 * - Made floating
 * - Closed and reopened
 * - Have their layout saved and restored
 */
class DockingExampleFrame : public wxFrame {
public:
    DockingExampleFrame();
    virtual ~DockingExampleFrame();

private:
    // Event handlers
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnSaveLayout(wxCommandEvent& event);
    void OnRestoreLayout(wxCommandEvent& event);
    void OnResetLayout(wxCommandEvent& event);
    void OnAddExampleWidgets(wxCommandEvent& event);
    
    // Helper methods
    void CreateMenuBar();
    void CreateDockingLayout();
    void CreateDefaultWidgets();
    wxTextCtrl* CreateTextEditor(const wxString& title);
    wxListCtrl* CreateFileList();
    wxTreeCtrl* CreateProjectTree();
    wxPanel* CreateToolPanel();
    
    // Member variables
    DockManager* m_dockManager;
    wxString m_savedLayout;
    
    wxDECLARE_EVENT_TABLE();
};

/**
 * @brief Example application for the docking system
 */
class DockingExampleApp : public wxApp {
public:
    virtual bool OnInit() override;
};

} // namespace ads
