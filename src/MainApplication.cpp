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
#include "FlatFrame.h"
#include "rendering/RenderingToolkitAPI.h"
#include "interfaces/ISubsystemFactory.h"
#include "interfaces/DefaultSubsystemFactory.h"
#include "interfaces/ServiceLocator.h"
#include "config/FontManager.h" // Include FontManager
#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/nodes/SoSeparator.h>
#include "mod/SoHighlightElementAction.h"
#include "mod/SoSelectionElementAction.h"
#include "mod/SoFCSelection.h"
#include "mod/SoFCUnifiedSelection.h"

// CRITICAL FIX: Disable Coin3D Display List caching globally
// This prevents GL context crashes when creating Coin3D nodes in invalid contexts
// This is the core reason FreeCAD 1.0 can smoothly open 2GB+ assemblies
static void DisableCoinDangerousCaches()
{
    SoDB::setCacheMode(SoDB::CACHE_DISABLED);  // Disable globally, never crash
    // If you want finer control, you can also only disable renderCaching, but global disable is most stable
}

bool MainApplication::OnInit()
{
    // CRITICAL: Disable Coin3D Display List caching BEFORE any window creation!
    // Must be called before wxApp initialization and any GLCanvas creation
    DisableCoinDangerousCaches();

    // Initialize Coin3D first - this must happen before any SoBase-derived objects are created
    try {
        SoDB::init();
        LOG_INF("Coin3D initialized successfully", "MainApplication");

        // Initialize selection action classes after Coin3D
        SoHighlightElementAction::initClass();
        SoSelectionElementAction::initClass();
        mod::SoFCSelection::initClass();
        SoFCUnifiedSelection::initClass();
        LOG_INF("Selection action classes initialized successfully", "MainApplication");

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
        LOG_INF("Rendering toolkit initialized successfully", "MainApplication");
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
        // If that fails, try with empty string to use default behavior
        if (!cm.initialize("")) {
            wxMessageBox("Failed to initialize configuration manager", "Initialization Error", wxOK | wxICON_ERROR);
            return false;
        }
    }
    ConstantsConfig::getInstance().initialize(cm);

    // Initialize FontManager after ConfigManager
    FontManager& fm = FontManager::getInstance();
    if (!fm.initialize("config/config.ini")) { // Pass the path to config.ini
        wxMessageBox("Failed to initialize font manager", "Initialization Error", wxOK | wxICON_ERROR);
        return false;
    }
    
    LOG_INF("Starting application", "MainApplication");

    
    std::string titleStr = cm.getString("MainApplication", "MainFrameTitle", "FlatUI Demo");
    wxString title(titleStr);
    
    // Calculate window size based on 80% of screen size, minimum 1200x700
    wxDisplay display;
    wxRect screenRect = display.GetGeometry();
    int screenWidth = screenRect.GetWidth();
    int screenHeight = screenRect.GetHeight();
    
    // Calculate 80% of screen size
    int fw = static_cast<int>(screenWidth * 0.8);
    int fh = static_cast<int>(screenHeight * 0.8);
    
    // Ensure minimum size of 1200x700
    fw = std::max(fw, 1200);
    fh = std::max(fh, 700);
    
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

wxIMPLEMENT_APP(MainApplication);
