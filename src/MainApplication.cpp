#include <wx/wx.h>
#include <cstdio>  
#include <string>
#include "MainApplication.h"
#include "config/ConfigManager.h"
#include "config/LoggerConfig.h"
#include "config/ConstantsConfig.h"
#include "logger/Logger.h"
#include "FlatFrame.h"
#include "UnifiedRefreshSystem.h"
#include "CommandDispatcher.h"
#include "GlobalServices.h"
#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/nodes/SoSeparator.h>

// Static member definitions
std::unique_ptr<UnifiedRefreshSystem> MainApplication::s_unifiedRefreshSystem = nullptr;
std::unique_ptr<CommandDispatcher> MainApplication::s_commandDispatcher = nullptr;

bool MainApplication::OnInit()
{
    // Initialize Coin3D first - this must happen before any SoBase-derived objects are created
    try {
        SoDB::init();
        LOG_INF("Coin3D initialized successfully", "MainApplication");
    } catch (const std::exception& e) {
        wxMessageBox("Failed to initialize Coin3D library: " + wxString(e.what()), "Initialization Error", wxOK | wxICON_ERROR);
        return false;
    }
    
    ConfigManager& cm = ConfigManager::getInstance();
    // Try to initialize with config/config.ini first
    if (!cm.initialize("")) {
        // If that fails, try with empty string to use default behavior
        if (!cm.initialize("")) {
            wxMessageBox("Failed to initialize configuration manager", "Initialization Error", wxOK | wxICON_ERROR);
            return false;
        }
    }
    ConstantsConfig::getInstance().initialize(cm);
    
    LOG_INF("Starting application", "MainApplication");

    // Initialize global services before creating any UI
    if (!initializeGlobalServices()) {
        wxMessageBox("Failed to initialize global services", "Initialization Error", wxOK | wxICON_ERROR);
        return false;
    }
    
    std::string titleStr = cm.getString("MainApplication", "MainFrameTitle", "FlatUI Demo");
    wxString title(titleStr);
    std::string sizeStr = cm.getString("MainApplication", "MainFrameSize", "1200,700");
    int fw = 1200, fh = 700;
    sscanf(sizeStr.c_str(), "%d,%d", &fw, &fh); 
    wxSize fsize(fw, fh);
    FlatFrame* frame = new FlatFrame(title, wxDefaultPosition, fsize);
    std::string posStr = cm.getString("MainApplication", "MainFramePosition", "Center");
    if (posStr == "Center") {
        frame->Centre();
    } else {
        int fx = 0, fy = 0;
        if (sscanf(posStr.c_str(), "%d,%d", &fx, &fy) == 2) { 
            frame->SetPosition(wxPoint(fx, fy));
        }
    }

    LOG_DBG("Frame created with initial size: " + 
            std::to_string(frame->GetSize().GetWidth()) + "x" + 
            std::to_string(frame->GetSize().GetHeight()), "MainApplication");

    frame->Show(true);

    return true;
}

bool MainApplication::initializeGlobalServices()
{
    LOG_INF("Initializing global services", "MainApplication");
    
    try {
        // Create command dispatcher first
        s_commandDispatcher = std::make_unique<CommandDispatcher>();
        LOG_INF("Command dispatcher created", "MainApplication");
        
        // Create unified refresh system (initially without Canvas/OCCViewer/SceneManager)
        // These will be set later when UI components are created
        s_unifiedRefreshSystem = std::make_unique<UnifiedRefreshSystem>(nullptr, nullptr, nullptr);
        LOG_INF("Unified refresh system created", "MainApplication");
        
        // Initialize the refresh system with command dispatcher
        s_unifiedRefreshSystem->initialize(s_commandDispatcher.get());
        LOG_INF("Unified refresh system initialized", "MainApplication");
        
        // Register services with GlobalServices
        GlobalServices::SetCommandDispatcher(s_commandDispatcher.get());
        GlobalServices::SetRefreshSystem(s_unifiedRefreshSystem.get());
        LOG_INF("Global services registered", "MainApplication");
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERR("Failed to initialize global services: " + std::string(e.what()), "MainApplication");
        return false;
    }
}

void MainApplication::shutdownGlobalServices()
{
    LOG_INF("Shutting down global services", "MainApplication");
    
    // Clear global services registry first
    GlobalServices::Clear();
    
    if (s_unifiedRefreshSystem) {
        s_unifiedRefreshSystem->shutdown();
        s_unifiedRefreshSystem.reset();
    }
    
    if (s_commandDispatcher) {
        s_commandDispatcher.reset();
    }
    
    LOG_INF("Global services shutdown completed", "MainApplication");
}

wxIMPLEMENT_APP(MainApplication);
