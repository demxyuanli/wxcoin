#include <wx/wx.h>
#include "docking/DockArea.h"
#include "docking/DockManager.h"
#include "docking/DockWidget.h"
#include "docking/DockContainerWidget.h"

class TestFrame : public wxFrame {
public:
    TestFrame() : wxFrame(nullptr, wxID_ANY, "Tab Position Test") {
        // Create dock manager
        m_dockManager = new ads::DockManager(this);
        
        // Create dock container
        m_container = new ads::DockContainerWidget(m_dockManager, this);
        
        // Create dock area
        m_dockArea = new ads::DockArea(m_dockManager, m_container);
        
        // Create some test dock widgets
        m_widget1 = new ads::DockWidget("Tab 1");
        m_widget1->setWidget(new wxPanel(m_widget1, wxID_ANY));
        m_widget1->setFeatures(ads::DockWidgetClosable | ads::DockWidgetMovable);
        
        m_widget2 = new ads::DockWidget("Tab 2");
        m_widget2->setWidget(new wxPanel(m_widget2, wxID_ANY));
        m_widget2->setFeatures(ads::DockWidgetClosable | ads::DockWidgetMovable);
        
        m_widget3 = new ads::DockWidget("Tab 3");
        m_widget3->setWidget(new wxPanel(m_widget3, wxID_ANY));
        m_widget3->setFeatures(ads::DockWidgetClosable | ads::DockWidgetMovable);
        
        // Add widgets to dock area
        m_dockArea->addDockWidget(m_widget1);
        m_dockArea->addDockWidget(m_widget2);
        m_dockArea->addDockWidget(m_widget3);
        
        // Add dock area to container
        m_container->addDockArea(ads::CenterDockWidgetArea, m_dockArea);
        
        // Create menu bar
        wxMenuBar* menuBar = new wxMenuBar();
        wxMenu* positionMenu = new wxMenu();
        positionMenu->Append(wxID_ANY, "Top", "Set tabs at top (merged mode)");
        positionMenu->Append(wxID_ANY, "Bottom", "Set tabs at bottom (independent mode)");
        positionMenu->Append(wxID_ANY, "Left", "Set tabs at left (independent mode)");
        positionMenu->Append(wxID_ANY, "Right", "Set tabs at right (independent mode)");
        menuBar->Append(positionMenu, "Tab Position");
        SetMenuBar(menuBar);
        
        // Bind menu events
        Bind(wxEVT_MENU, [this](wxCommandEvent& event) {
            switch (event.GetId()) {
                case 0: m_dockArea->setTabPosition(ads::TabPosition::Top); break;
                case 1: m_dockArea->setTabPosition(ads::TabPosition::Bottom); break;
                case 2: m_dockArea->setTabPosition(ads::TabPosition::Left); break;
                case 3: m_dockArea->setTabPosition(ads::TabPosition::Right); break;
            }
        });
        
        // Set initial tab position
        m_dockArea->setTabPosition(ads::TabPosition::Top);
        
        // Set size
        SetSize(800, 600);
    }
    
private:
    ads::DockManager* m_dockManager;
    ads::DockContainerWidget* m_container;
    ads::DockArea* m_dockArea;
    ads::DockWidget* m_widget1;
    ads::DockWidget* m_widget2;
    ads::DockWidget* m_widget3;
};

class TestApp : public wxApp {
public:
    bool OnInit() override {
        TestFrame* frame = new TestFrame();
        frame->Show();
        return true;
    }
};

wxIMPLEMENT_APP(TestApp);