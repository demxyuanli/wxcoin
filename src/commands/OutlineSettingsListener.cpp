#include "commands/OutlineSettingsListener.h"
#include "OCCViewer.h"
#include "CommandType.h"
#include "ui/OutlineSettingsDialog.h"
#include "logger/Logger.h"

CommandResult OutlineSettingsListener::executeCommand(const std::string& commandType,
    const std::unordered_map<std::string, std::string>& parameters) {
    
    if (!m_viewer) {
        return CommandResult(false, "Viewer not available", commandType);
    }
    
    try {
        // Create and show outline settings dialog
        // Note: We need a parent window - this would typically be the main frame
        // For now, we'll use nullptr and let wxWidgets handle it
        auto* dialog = new OutlineSettingsDialog(nullptr, m_viewer);
        
        if (dialog->ShowModal() == wxID_OK) {
            LOG_INF_S("Outline settings applied");
            dialog->Destroy();
            return CommandResult(true, "Outline settings updated", commandType);
        } else {
            LOG_INF_S("Outline settings cancelled");
            dialog->Destroy();
            return CommandResult(true, "Outline settings cancelled", commandType);
        }
        
    } catch (const std::exception& e) {
        LOG_ERR_S(std::string("Failed to open outline settings: ") + e.what());
        return CommandResult(false, std::string("Failed to open outline settings: ") + e.what(), commandType);
    }
}

bool OutlineSettingsListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::OutlineSettings);
}