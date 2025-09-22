#include "NavigationModeListener.h"
#include "FlatFrame.h"
#include "NavigationModeManager.h"
#include "CommandDispatcher.h"
#include "logger/Logger.h"
#include <wx/msgdlg.h>

NavigationModeListener::NavigationModeListener() {
    LOG_INF_S("NavigationModeListener created");
}

NavigationModeListener::~NavigationModeListener() {
    LOG_INF_S("NavigationModeListener destroyed");
}

bool NavigationModeListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::NavigationMode);
}

CommandResult NavigationModeListener::executeCommand(const std::string& commandType,
    const std::unordered_map<std::string, std::string>& parameters) {
    LOG_INF_S("NavigationModeListener::execute called");
    
    // Get the main frame to access navigation mode manager
    FlatFrame* mainFrame = dynamic_cast<FlatFrame*>(wxTheApp->GetTopWindow());
    if (!mainFrame) {
        LOG_ERR_S("Cannot get main frame for navigation mode dialog");
        return CommandResult(false, "Cannot get main frame for navigation mode dialog");
    }
    
    // Try to get navigation mode manager from the frame
    NavigationModeManager* navManager = mainFrame->getNavigationModeManager();
    if (!navManager) {
        LOG_ERR_S("Navigation mode manager not available");
        wxMessageBox("Navigation mode manager not available", "Error", wxOK | wxICON_ERROR);
        return CommandResult(false, "Navigation mode manager not available");
    }
    
    // Show navigation mode selection dialog
    showNavigationModeDialog(navManager);
    
    return CommandResult(true, "Navigation mode dialog shown");
}

void NavigationModeListener::showNavigationModeDialog(NavigationModeManager* navManager) {
    // Create a simple dialog to select navigation mode
    wxArrayString choices;
    choices.Add("Gesture Navigation (Touch-friendly)");
    choices.Add("Inventor Navigation (CAD-style)");
    
    NavigationStyle currentStyle = navManager->getNavigationStyle();
    int selection = static_cast<int>(currentStyle);
    
    int choice = wxGetSingleChoiceIndex(
        "Select Navigation Mode:",
        "Navigation Mode Settings",
        choices,
        selection,
        wxTheApp->GetTopWindow()
    );
    
    if (choice != wxNOT_FOUND) {
        NavigationStyle newStyle = static_cast<NavigationStyle>(choice);
        navManager->setNavigationStyle(newStyle);
        
        std::string modeName = (newStyle == NavigationStyle::GESTURE) ? "Gesture" : "Inventor";
        LOG_INF_S("Navigation mode changed to: " + modeName);
        
        wxMessageBox(
            "Navigation mode changed to " + modeName + " Navigation",
            "Navigation Mode",
            wxOK | wxICON_INFORMATION
        );
    }
}

std::string NavigationModeListener::getListenerName() const {
    return "NavigationModeListener";
}
