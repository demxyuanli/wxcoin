#include "NavigationModeListener.h"
#include "FlatFrame.h"
#include "NavigationModeManager.h"
#include "CommandDispatcher.h"
#include "logger/Logger.h"
#include "config/ConfigManager.h"
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
    auto availableStyles = navManager->getAvailableStyles();
    
    wxArrayString choices;
    int currentSelection = 0;
    NavigationStyle currentStyle = navManager->getNavigationStyle();
    
    for (size_t i = 0; i < availableStyles.size(); ++i) {
        const auto& style = availableStyles[i];
        choices.Add(wxString(style.second));
        if (style.first == currentStyle) {
            currentSelection = static_cast<int>(i);
        }
    }
    
    int choice = wxGetSingleChoiceIndex(
        "Select Navigation Style:",
        "Navigation Style Settings",
        choices,
        currentSelection,
        wxTheApp->GetTopWindow()
    );
    
    if (choice != wxNOT_FOUND && choice < static_cast<int>(availableStyles.size())) {
        NavigationStyle newStyle = availableStyles[choice].first;
        navManager->setNavigationStyle(newStyle);
        
        std::string styleName = navManager->getCurrentStyleName();
        LOG_INF_S("Navigation style changed to: " + styleName);
        
        wxMessageBox(
            "Navigation style changed to " + wxString(styleName),
            "Navigation Style",
            wxOK | wxICON_INFORMATION
        );
        
        saveNavigationStyleToConfig(newStyle);
    }
}

void NavigationModeListener::saveNavigationStyleToConfig(NavigationStyle style) {
    try {
        auto& config = ConfigManager::getInstance();
        config.setInt("Navigation", "Style", static_cast<int>(style));
        config.save();
        LOG_INF_S("Navigation style saved to config: " + std::to_string(static_cast<int>(style)));
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to save navigation style to config: " + std::string(e.what()));
    }
}

std::string NavigationModeListener::getListenerName() const {
    return "NavigationModeListener";
}
