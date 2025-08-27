/**
 * @brief Test program for docking system integration with main application
 * 
 * This test creates a simplified version of the main application
 * with the docking system integrated.
 */

#include <wx/wx.h>
#include <wx/config.h>
#include "FlatFrameDocking.h"
#include "config/ConfigManager.h"
#include "logger/Logger.h"

// Simple test application
class DockingIntegrationTestApp : public wxApp {
public:
    virtual bool OnInit() override {
        // Initialize config manager with minimal setup
        ConfigManager& cm = ConfigManager::getInstance();
        
        // Create config directory if it doesn't exist
        wxString configDir = wxGetCwd() + wxFileName::GetPathSeparator() + "config";
        if (!wxDir::Exists(configDir)) {
            wxMkdir(configDir);
        }
        
        // Initialize with minimal config
        cm.initialize("");
        
        // Create main frame with docking
        FlatFrameDocking* frame = new FlatFrameDocking(
            "Docking Integration Test", 
            wxDefaultPosition, 
            wxSize(1024, 768)
        );
        
        frame->Show(true);
        SetTopWindow(frame);
        
        // Show a startup message
        wxMessageBox(
            "Docking System Integration Test\n\n"
            "This test demonstrates the integration of the advanced docking\n"
            "system with the main CAD application framework.\n\n"
            "Features to test:\n"
            "- Dock/undock panels by dragging tabs\n"
            "- Create floating windows\n"
            "- Auto-hide panels (pin/unpin)\n"
            "- Save/load layouts (View menu)\n"
            "- Manage perspectives",
            "Welcome",
            wxOK | wxICON_INFORMATION
        );
        
        return true;
    }
};

wxIMPLEMENT_APP(DockingIntegrationTestApp);