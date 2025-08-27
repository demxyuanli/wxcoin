/**
 * @brief Minimal example showing how to integrate docking into existing wxWidgets application
 * 
 * This example shows the minimal code needed to add docking to your application
 * without modifying build files.
 */

#include <wx/wx.h>
#include "docking/DockManager.h"
#include "docking/DockWidget.h"

using namespace ads;

// Your existing frame class
class MyExistingFrame : public wxFrame {
public:
    MyExistingFrame() : wxFrame(nullptr, wxID_ANY, "Minimal Docking Example", 
                                wxDefaultPosition, wxSize(800, 600)) {
        
        // Step 1: Create a panel to hold the dock manager
        wxPanel* mainPanel = new wxPanel(this);
        
        // Step 2: Create the dock manager
        m_dockManager = new DockManager(mainPanel);
        
        // Step 3: Create your existing controls as dock widgets
        CreateDockingLayout();
        
        // Step 4: Set up the layout
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(m_dockManager, 1, wxEXPAND);
        mainPanel->SetSizer(sizer);
        
        CreateStatusBar();
        SetStatusText("Minimal docking example");
    }
    
private:
    DockManager* m_dockManager;
    
    void CreateDockingLayout() {
        // Convert your main content area to a dock widget
        DockWidget* mainContent = new DockWidget("Main Content", m_dockManager);
        wxTextCtrl* text = new wxTextCtrl(mainContent, wxID_ANY, 
            "This is your main content area.\n\n"
            "The docking system has been integrated with minimal changes.",
            wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
        mainContent->setWidget(text);
        mainContent->setFeature(DockWidget::DockWidgetClosable, false);
        m_dockManager->addDockWidget(CenterDockWidgetArea, mainContent);
        
        // Convert your side panel to a dock widget
        DockWidget* sidePanel = new DockWidget("Side Panel", m_dockManager);
        wxListBox* list = new wxListBox(sidePanel, wxID_ANY);
        list->Append("Item 1");
        list->Append("Item 2");
        list->Append("Item 3");
        sidePanel->setWidget(list);
        m_dockManager->addDockWidget(LeftDockWidgetArea, sidePanel);
        
        // Add a properties panel
        DockWidget* properties = new DockWidget("Properties", m_dockManager);
        wxPanel* propPanel = new wxPanel(properties);
        wxStaticText* label = new wxStaticText(propPanel, wxID_ANY, "Properties go here");
        properties->setWidget(propPanel);
        m_dockManager->addDockWidget(RightDockWidgetArea, properties);
    }
};

// Application
class MinimalApp : public wxApp {
public:
    virtual bool OnInit() override {
        MyExistingFrame* frame = new MyExistingFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(MinimalApp);

/*
 * To compile this example:
 * 
 * 1. Make sure the main project with docking library is built
 * 
 * 2. Linux/macOS:
 *    g++ -o minimal_docking minimal_docking_example.cpp \
 *        -I../include `wx-config --cxxflags` \
 *        -L../build/src/docking -ldocking \
 *        `wx-config --libs` -std=c++17
 * 
 * 3. Windows (from VS Developer Command Prompt):
 *    cl /EHsc /std:c++17 /I..\include /I%WXWIN%\include /I%WXWIN%\include\msvc \
 *       minimal_docking_example.cpp \
 *       /link /LIBPATH:..\build\src\docking\Release /LIBPATH:%WXWIN%\lib\vc_x64_lib \
 *       docking.lib wxmsw32u_core.lib wxbase32u.lib [other Windows libs...]
 */