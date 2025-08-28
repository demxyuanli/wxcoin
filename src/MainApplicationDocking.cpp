#include <wx/wx.h>
#include <wx/display.h>
#include <cstdio>  
#include <string>
#include <algorithm>
#include "MainApplication.h"
#include "config/ConfigManager.h"
#include "config/LoggerConfig.h"
#include "config/ConstantsConfig.h"
#include "logger/Logger.h"
#include "FlatFrameDocking.h"  // Use docking version
#include "rendering/RenderingToolkitAPI.h"
#include "interfaces/ISubsystemFactory.h"
#include "interfaces/DefaultSubsystemFactory.h"
#include "interfaces/ServiceLocator.h"
#include "config/FontManager.h"
#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/nodes/SoSeparator.h>

// Define a new app class that uses docking
class MainApplicationDocking : public MainApplication {
public:
    virtual bool OnInit() override;
};

wxIMPLEMENT_APP(MainApplicationDocking);

bool MainApplicationDocking::OnInit()
{
    // Initialize Coin3D first - this must happen before any SoBase-derived objects are created
    try {
        SoDB::init();
        LOG_INF("Coin3D initialized successfully", "MainApplicationDocking");
    } catch (const std::exception& e) {
        wxMessageBox("Failed to initialize Coin3D library: " + wxString(e.what()), "Initialization Error", wxOK | wxICON_ERROR);
        return false;
    }
    
    // Initialize rendering toolkit
    try {
        if (!RenderingToolkitAPI::initialize()) {
            wxMessageBox("Failed to initialize rendering toolkit", "Initialization Error", wxOK | wxICON_ERROR);
            return false;
        }
        LOG_INF("Rendering toolkit initialized successfully", "MainApplicationDocking");
    } catch (const std::exception& e) {
        wxMessageBox("Failed to initialize rendering toolkit: " + wxString(e.what()), "Initialization Error", wxOK | wxICON_ERROR);
        return false;
    }
    
    // Set default subsystem factory (can be replaced by tests or other compositions)
    static DefaultSubsystemFactory s_factory;
    ServiceLocator::setFactory(&s_factory);

    ConfigManager& cm = ConfigManager::getInstance();
    // Try to initialize with config/config.ini first
    if (!cm.initialize("")) {
        // If that fails, try ./config.ini
        if (!cm.initialize("./config.ini")) {
            wxMessageBox("Cannot find config.ini in config/ or current directory", 
                        "Configuration Error", wxOK | wxICON_ERROR);
            return false;
        }
    }

    ConstantsConfig::getInstance().initialize(cm);

    // Initialize FontManager after ConfigManager
    FontManager& fontManager = FontManager::getInstance();
    if (!fontManager.initialize("config/config.ini")) { // Pass the path to config.ini
        wxMessageBox("Failed to initialize font manager", "Initialization Error", wxOK | wxICON_ERROR);
        return false;
    }

    // Create main frame with docking system
    FlatFrameDocking* frame = new FlatFrameDocking("CAD Navigator - Docking Edition", 
                                                   wxDefaultPosition, 
                                                   wxSize(1200, 800));
    
    // Center on primary display
    wxDisplay display;
    wxRect screenRect = display.GetGeometry();
    frame->Centre();
    
    frame->Show(true);
    SetTopWindow(frame);
    
    LOG_INF("Application started with docking system", "MainApplicationDocking");
    
    return true;
}
