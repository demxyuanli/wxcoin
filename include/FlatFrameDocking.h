#pragma once

#include "FlatFrame.h"
#include "docking/DockManager.h"
#include "docking/DockWidget.h"
#include "docking/PerspectiveManager.h"

namespace ads {
    class DockManager;
    class DockWidget;
}

/**
 * @brief Enhanced FlatFrame with integrated docking system
 * 
 * This class extends FlatFrame to use the advanced docking system
 * instead of the traditional wxAUI manager.
 */
class FlatFrameDocking : public FlatFrame {
public:
    FlatFrameDocking(const wxString& title, const wxPoint& pos, const wxSize& size);
    virtual ~FlatFrameDocking();
    
    // Override Destroy for proper cleanup
    virtual bool Destroy() override;

    // Initialize docking layout
    void InitializeDockingLayout();
    
    // Docking-specific methods
    void CreateDockingLayout();
    void SaveDockingLayout(const wxString& filename);
    void LoadDockingLayout(const wxString& filename);
    void ResetDockingLayout();
    
    // Convert existing panels to dock widgets
    ads::DockWidget* CreatePropertyDockWidget();
    ads::DockWidget* CreateObjectTreeDockWidget();
    ads::DockWidget* CreateCanvasDockWidget();
    ads::DockWidget* CreateOutputDockWidget();
    ads::DockWidget* CreateToolboxDockWidget();
    
    // Access to dock manager
    ads::DockManager* GetDockManager() const { return m_dockManager; }
    
    // Override virtual methods from FlatFrame
    virtual bool IsUsingDockingSystem() const { return true; }
    virtual wxWindow* GetMainWorkArea() const;
    
protected:
    // Event handlers for docking
    void OnViewShowHidePanel(wxCommandEvent& event);
    void OnUpdateUI(wxUpdateUIEvent& event);
    
    // Menu handlers for docking features
    void OnDockingSaveLayout(wxCommandEvent& event);
    void OnDockingLoadLayout(wxCommandEvent& event);
    void OnDockingResetLayout(wxCommandEvent& event);
    void OnDockingManagePerspectives(wxCommandEvent& event);
    void OnDockingToggleAutoHide(wxCommandEvent& event);
    
private:
    ads::DockManager* m_dockManager;
    
    // Dock widgets for main panels
    ads::DockWidget* m_propertyDock;
    ads::DockWidget* m_objectTreeDock;
    ads::DockWidget* m_canvasDock;
    
    void CreateDockingMenus();
    void ConfigureDockManager();
    
    wxDECLARE_EVENT_TABLE();
};

// Additional menu IDs for docking features
enum {
    ID_DOCKING_SAVE_LAYOUT = wxID_HIGHEST + 2000,
    ID_DOCKING_LOAD_LAYOUT,
    ID_DOCKING_RESET_LAYOUT,
    ID_DOCKING_MANAGE_PERSPECTIVES,
    ID_DOCKING_TOGGLE_AUTOHIDE,
    ID_DOCKING_FLOAT_ALL,
    ID_DOCKING_DOCK_ALL,
    
    // View panel IDs
    ID_VIEW_PROPERTIES,
    ID_VIEW_OBJECT_TREE,
    ID_VIEW_OUTPUT,
    ID_VIEW_TOOLBOX
};
