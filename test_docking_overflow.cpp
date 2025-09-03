// Test program to verify docking tab overflow button positioning
#include <wx/wx.h>
#include "docking/DockManager.h"
#include "docking/DockWidget.h"
#include "docking/DockArea.h"
#include "config/ThemeManager.h"

class TestApp : public wxApp {
public:
    bool OnInit() override {
        // Initialize theme manager
        ThemeManager::getInstance();
        
        // Create main frame
        wxFrame* frame = new wxFrame(nullptr, wxID_ANY, "Docking Overflow Button Test",
                                     wxDefaultPosition, wxSize(800, 600));
        
        // Create dock manager
        ads::DockManager* dockManager = new ads::DockManager();
        dockManager->Initialize(frame);
        
        // Create multiple dock widgets to test overflow
        for (int i = 0; i < 10; ++i) {
            wxString title = wxString::Format("Tab %d with Long Title", i + 1);
            wxPanel* content = new wxPanel(frame);
            content->SetBackgroundColour(wxColour(200 + i * 5, 200 + i * 5, 200 + i * 5));
            
            ads::DockWidget* widget = new ads::DockWidget(title, content);
            widget->setFeature(ads::DockWidgetClosable, true);
            widget->setFeature(ads::DockWidgetMovable, true);
            widget->setFeature(ads::DockWidgetFloatable, true);
            
            // Add all widgets to the same area to trigger overflow
            dockManager->addDockWidget(ads::CenterDockWidgetArea, widget);
        }
        
        frame->Show();
        
        // Log test information
        wxLogMessage("Test Instructions:");
        wxLogMessage("1. Check that overflow button appears when tabs don't fit");
        wxLogMessage("2. Verify overflow button is 4px after last visible tab");
        wxLogMessage("3. Verify overflow button maintains 4px min distance from title bar buttons");
        wxLogMessage("4. Check title bar buttons have 0 spacing and 0 margin from edges");
        wxLogMessage("5. Resize window to test dynamic positioning");
        
        return true;
    }
};

wxIMPLEMENT_APP(TestApp);