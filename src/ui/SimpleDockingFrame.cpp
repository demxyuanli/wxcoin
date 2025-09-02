/**
 * @brief Simplified docking frame implementation
 *
 * This version avoids conflicts with base class private members
 * by creating its own widgets independently.
 */

#include <wx/wx.h>
#include <wx/artprov.h>
#include <wx/filedlg.h>
#include <wx/file.h>
#include <wx/listbox.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>
#include <wx/stattext.h>
#include "docking/DockManager.h"
#include "docking/DockWidget.h"
#include "docking/DockArea.h"
#include "docking/PerspectiveManager.h"
#include "docking/DockLayoutConfig.h"

using namespace ads;

class SimpleDockingFrame : public wxFrame {
public:
	SimpleDockingFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
		: wxFrame(nullptr, wxID_ANY, title, pos, size)
		, m_dockManager(nullptr)
	{
		InitializeDocking();
		
		// Bind window resize event to prevent ghosting
		Bind(wxEVT_SIZE, &SimpleDockingFrame::OnSize, this);
	}

	virtual ~SimpleDockingFrame() {
		// DockManager will be deleted by its parent
	}

private:
    DockManager* m_dockManager;
    
    void InitializeDocking() {
        // Create main panel
        wxPanel* mainPanel = new wxPanel(this);
        
        // Create dock manager
        m_dockManager = new DockManager(mainPanel);
        ConfigureDockManager();
        
        // Create docking layout
        CreateDockingLayout();
        
        // Create menus
        CreateMenus();
        
        // Set up sizers
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
        mainSizer->Add(m_dockManager->containerWidget(), 1, wxEXPAND);
        mainPanel->SetSizer(mainSizer);
        
        // Status bar
        CreateStatusBar(2);
        SetStatusText("Docking system ready", 0);
    }
    
    void ConfigureDockManager() {
        m_dockManager->setConfigFlag(OpaqueSplitterResize, true);
        m_dockManager->setConfigFlag(DockAreaHasCloseButton, true);
        m_dockManager->setConfigFlag(AllTabsHaveCloseButton, true);
        m_dockManager->setConfigFlag(FocusHighlighting, true);
        m_dockManager->setConfigFlag(AlwaysShowTabs, true);
        m_dockManager->setConfigFlag(EqualSplitOnInsertion, true);
    }
    
    void CreateDockingLayout() {
        // Create a proper five-area layout with explicit positioning
        // 
        // +-------------------------------------+
        // |          Menu Bar (Top)             |
        // +-------------+-------------+---------+
        // |             |             |         |
        // |  Toolbox    |  Main View  |  Props  |
        // |  (Left)     |  (Center)   |  (Right)|
        // |             |             |         |
        // +-------------+-------------+---------+
        // |        Output Panel (Bottom)         |
        // +-------------------------------------+

        // Create all dock widgets first
        DockWidget* menuDock = new DockWidget("Menu Bar", m_dockManager->containerWidget());
        wxPanel* menuPanel = new wxPanel(menuDock);
        menuPanel->SetBackgroundColour(wxColour(220, 220, 255));
        wxBoxSizer* menuSizer = new wxBoxSizer(wxHORIZONTAL);
        wxStaticText* menuText = new wxStaticText(menuPanel, wxID_ANY, "File | Edit | View | Tools | Help",
                                                 wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
        menuSizer->Add(menuText, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
        menuSizer->AddStretchSpacer(1);
        menuPanel->SetSizer(menuSizer);
        menuDock->setWidget(menuPanel);
        menuDock->setFeature(DockWidgetClosable, true);
        menuDock->setFeature(DockWidgetMovable, true);
        menuDock->setFeature(DockWidgetFloatable, true);
        menuDock->setIcon(wxArtProvider::GetIcon(wxART_LIST_VIEW, wxART_MENU));

        DockWidget* toolDock = new DockWidget("Toolbox", m_dockManager->containerWidget());
        wxListBox* toolList = new wxListBox(toolDock, wxID_ANY);
        toolList->Append("Select Tool");
        toolList->Append("Move Tool");
        toolList->Append("Scale Tool");
        toolList->Append("Rotate Tool");
        toolList->Append("Brush Tool");
        toolList->SetSelection(0);
        toolDock->setWidget(toolList);
        toolDock->setFeature(DockWidgetClosable, true);
        toolDock->setFeature(DockWidgetMovable, true);
        toolDock->setFeature(DockWidgetFloatable, true);
        toolDock->setIcon(wxArtProvider::GetIcon(wxART_EXECUTABLE_FILE, wxART_MENU));

        DockWidget* mainDock = new DockWidget("Main View", m_dockManager->containerWidget());
        wxPanel* mainPanel = new wxPanel(mainDock);
        mainPanel->SetBackgroundColour(wxColour(240, 240, 240));
        wxStaticText* mainText = new wxStaticText(mainPanel, wxID_ANY,
            "This is the main view panel\nNon-closable, always displayed in center area",
            wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER | wxST_NO_AUTORESIZE);
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
        mainSizer->AddStretchSpacer(1);
        mainSizer->Add(mainText, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 20);
        mainSizer->AddStretchSpacer(1);
        mainPanel->SetSizer(mainSizer);
        mainDock->setWidget(mainPanel);
        mainDock->setFeature(DockWidgetClosable, false);
        mainDock->setIcon(wxArtProvider::GetIcon(wxART_NORMAL_FILE, wxART_MENU));

        DockWidget* propDock = new DockWidget("Properties", m_dockManager->containerWidget());
        wxPropertyGrid* propGrid = new wxPropertyGrid(propDock);
        propGrid->Append(new wxStringProperty("Name", wxPG_LABEL, "Object1"));
        propGrid->Append(new wxIntProperty("Width", wxPG_LABEL, 100));
        propGrid->Append(new wxIntProperty("Height", wxPG_LABEL, 100));
        propGrid->Append(new wxBoolProperty("Visible", wxPG_LABEL, true));
        propGrid->Append(new wxFloatProperty("Opacity", wxPG_LABEL, 1.0));
        propDock->setWidget(propGrid);
        propDock->setFeature(DockWidgetClosable, true);
        propDock->setFeature(DockWidgetMovable, true);
        propDock->setFeature(DockWidgetFloatable, true);
        propDock->setIcon(wxArtProvider::GetIcon(wxART_REPORT_VIEW, wxART_MENU));

        DockWidget* outputDock = new DockWidget("Output", m_dockManager->containerWidget());
        wxTextCtrl* output = new wxTextCtrl(outputDock, wxID_ANY,
                                           "Welcome to Simple Docking Example\n"
                                           "This is an example program showing basic docking features\n"
                                           "- Drag tabs to move panels\n"
                                           "- Drag to edges to dock\n"
                                           "- Drag to center to create tab groups\n",
                                           wxDefaultPosition, wxDefaultSize,
                                           wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);
        output->SetDefaultStyle(wxTextAttr(*wxBLACK, *wxWHITE));
        outputDock->setWidget(output);
        outputDock->setFeature(DockWidgetClosable, true);
        outputDock->setFeature(DockWidgetMovable, true);
        outputDock->setFeature(DockWidgetFloatable, true);
        outputDock->setIcon(wxArtProvider::GetIcon(wxART_INFORMATION, wxART_MENU));

        // Create layout in proper order for 5-zone layout
        // First create the center area
        wxLogDebug("Step 1: Creating Center area (Main View)");
        DockArea* centerArea = m_dockManager->addDockWidget(CenterDockWidgetArea, mainDock);
        wxLogDebug("  -> Center area created: %p", centerArea);

        // Then create left and right areas to establish the middle layer
        wxLogDebug("Step 2: Creating Left area (Toolbox)");
        DockArea* leftArea = m_dockManager->addDockWidget(LeftDockWidgetArea, toolDock);
        wxLogDebug("  -> Left area created: %p", leftArea);

        wxLogDebug("Step 3: Creating Right area (Properties)");
        DockArea* rightArea = m_dockManager->addDockWidget(RightDockWidgetArea, propDock);
        wxLogDebug("  -> Right area created: %p", rightArea);

        // Finally create top and bottom areas which should span full width
        wxLogDebug("Step 4: Creating Top area (Menu Bar)");
        DockArea* topArea = m_dockManager->addDockWidget(TopDockWidgetArea, menuDock);
        wxLogDebug("  -> Top area created: %p", topArea);

        wxLogDebug("Step 5: Creating Bottom area (Output)");
        DockArea* bottomArea = m_dockManager->addDockWidget(BottomDockWidgetArea, outputDock);
        wxLogDebug("  -> Bottom area created: %p", bottomArea);

        wxLogDebug("Layout creation complete:");
        wxLogDebug("  Top (Menu): %p", topArea);
        wxLogDebug("  Bottom (Output): %p", bottomArea);
        wxLogDebug("  Left (Toolbox): %p", leftArea);
        wxLogDebug("  Center (Main): %p", centerArea);
        wxLogDebug("  Right (Properties): %p", rightArea);
    }
    
    void CreateMenus() {
        wxMenuBar* menuBar = new wxMenuBar();
        
        // File menu
        wxMenu* fileMenu = new wxMenu();
        fileMenu->Append(wxID_EXIT, "E&xit");
        menuBar->Append(fileMenu, "&File");
        
        // View menu
        wxMenu* viewMenu = new wxMenu();
        viewMenu->Append(ID_SAVE_LAYOUT, "Save Layout...\tCtrl+S");
        viewMenu->Append(ID_LOAD_LAYOUT, "Load Layout...\tCtrl+O");
        viewMenu->Append(ID_RESET_LAYOUT, "Reset Layout");
        viewMenu->AppendSeparator();
        viewMenu->Append(ID_MANAGE_PERSPECTIVES, "Manage Perspectives...");
        viewMenu->AppendSeparator();
        viewMenu->Append(ID_CONFIGURE_LAYOUT, "Configure Layout...");
        menuBar->Append(viewMenu, "&View");
        
        SetMenuBar(menuBar);
        
        // Bind events
        Bind(wxEVT_MENU, [this](wxCommandEvent&) { Close(); }, wxID_EXIT);
        Bind(wxEVT_MENU, &SimpleDockingFrame::OnSaveLayout, this, ID_SAVE_LAYOUT);
        Bind(wxEVT_MENU, &SimpleDockingFrame::OnLoadLayout, this, ID_LOAD_LAYOUT);
        Bind(wxEVT_MENU, &SimpleDockingFrame::OnResetLayout, this, ID_RESET_LAYOUT);
        Bind(wxEVT_MENU, &SimpleDockingFrame::OnManagePerspectives, this, ID_MANAGE_PERSPECTIVES);
        Bind(wxEVT_MENU, &SimpleDockingFrame::OnConfigureLayout, this, ID_CONFIGURE_LAYOUT);
    }
    
    void OnSaveLayout(wxCommandEvent&) {
        wxFileDialog dlg(this, "Save Layout", "", "layout.xml",
                        "XML files (*.xml)|*.xml", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dlg.ShowModal() == wxID_OK) {
            wxString state;
            m_dockManager->saveState(state);
            wxFile file(dlg.GetPath(), wxFile::write);
            if (file.IsOpened()) {
                file.Write(state);
                SetStatusText("Layout saved", 0);
            }
        }
    }
    
    void OnLoadLayout(wxCommandEvent&) {
        wxFileDialog dlg(this, "Load Layout", "", "",
                        "XML files (*.xml)|*.xml", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        if (dlg.ShowModal() == wxID_OK) {
            wxFile file(dlg.GetPath(), wxFile::read);
            if (file.IsOpened()) {
                wxString state;
                file.ReadAll(&state);
                if (m_dockManager->restoreState(state)) {
                    SetStatusText("Layout loaded", 0);
                }
            }
        }
    }
    
    void OnResetLayout(wxCommandEvent&) {
        // Simple reset: remove and recreate
        auto widgets = m_dockManager->dockWidgets();
        for (auto* widget : widgets) {
            m_dockManager->removeDockWidget(widget);
        }
        CreateDockingLayout();
        SetStatusText("Layout reset", 0);
    }
    
    void OnManagePerspectives(wxCommandEvent&) {
        ads::PerspectiveDialog dlg(this, m_dockManager->perspectiveManager());
        dlg.ShowModal();
    }
    
    void OnConfigureLayout(wxCommandEvent&) {
        const DockLayoutConfig& currentConfig = m_dockManager->getLayoutConfig();
        DockLayoutConfig config = currentConfig;  // Make a copy
        DockLayoutConfigDialog dlg(this, config, m_dockManager);
        
        if (dlg.ShowModal() == wxID_OK) {
            config = dlg.GetConfig();
            m_dockManager->setLayoutConfig(config);
            
            // Apply the configuration immediately to all containers
            auto containers = m_dockManager->dockContainers();
            for (auto* container : containers) {
                container->applyLayoutConfig();
            }
            
            SetStatusText("Layout configuration updated and applied", 0);
        }
    }
    
    void OnSize(wxSizeEvent& event) {
        // Force refresh of dock manager to prevent ghosting during window resize
        if (m_dockManager && m_dockManager->containerWidget()) {
            m_dockManager->containerWidget()->Refresh();
            m_dockManager->containerWidget()->Update();
        }
        
        event.Skip();
    }
    
    enum {
    ID_SAVE_LAYOUT = wxID_HIGHEST + 1,
    ID_LOAD_LAYOUT,
    ID_RESET_LAYOUT,
    ID_MANAGE_PERSPECTIVES,
    ID_CONFIGURE_LAYOUT
};
};

// Simple test app
class SimpleDockingApp : public wxApp {
public:
	virtual bool OnInit() override {
		SimpleDockingFrame* frame = new SimpleDockingFrame(
			"Simple Docking Frame",
			wxDefaultPosition,
			wxSize(1200, 800)
		);
		frame->Show(true);
		return true;
	}
}; 

wxIMPLEMENT_APP(SimpleDockingApp); 
