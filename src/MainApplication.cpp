#include <wx/wx.h>
#include <cstdio>  
#include <string>
#include "MainApplication.h"
#include "config/ConfigManager.h"
#include "config/LoggerConfig.h"
#include "config/ConstantsConfig.h"
#include "logger/Logger.h"
#include "FlatFrame.h"
#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/nodes/SoSeparator.h>

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

wxIMPLEMENT_APP(MainApplication);
