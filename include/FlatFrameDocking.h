#pragma once

// Define this to prevent base class from including legacy layout components
#define USE_NEW_DOCKING_SYSTEM

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
    ads::DockWidget* CreateMessageDockWidget();
    ads::DockWidget* CreatePerformanceDockWidget();
    ads::DockWidget* CreateToolboxDockWidget();  // Keep for compatibility, but not used
    
    // Access to dock manager
    ads::DockManager* GetDockManager() const { return m_dockManager; }
    
    // Override virtual methods from FlatFrame
    virtual bool IsUsingDockingSystem() const override { return true; }
    virtual wxWindow* GetMainWorkArea() const override;
    
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
    void OnDockingConfigureLayout(wxCommandEvent& event);
    
    // Override base class layout events to prevent interference
    void onSize(wxSizeEvent& event);
    
private:
    ads::DockManager* m_dockManager;
    wxPanel* m_workAreaPanel;
    
    // Dock widgets for main panels
    ads::DockWidget* m_propertyDock;
    ads::DockWidget* m_objectTreeDock;
    ads::DockWidget* m_canvasDock;
    ads::DockWidget* m_messageDock;      // Renamed from m_outputDock
    ads::DockWidget* m_performanceDock;  // New performance panel
    
    // Output control
    wxTextCtrl* m_outputCtrl;
    
    void CreateDockingMenus();
    void ConfigureDockManager();
    void RegisterDockLayoutConfigListener();
    
    wxDECLARE_EVENT_TABLE();
};
