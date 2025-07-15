#include "SetTransparencyListener.h"
#include "CommandType.h"
#include "TransparencyDialog.h"
#include "logger/Logger.h"

SetTransparencyListener::SetTransparencyListener(wxFrame* frame, OCCViewer* viewer)
    : m_frame(frame), m_viewer(viewer)
{
}

CommandResult SetTransparencyListener::executeCommand(const std::string& commandType,
                                                     const std::unordered_map<std::string, std::string>& parameters)
{
    if (!m_frame || !m_viewer) {
        return CommandResult(false, "Frame or OCCViewer not available", commandType);
    }

    // Get selected geometries
    auto selectedGeometries = m_viewer->getSelectedGeometries();
    if (selectedGeometries.empty()) {
        return CommandResult(false, "No geometry selected", commandType);
    }

    // Create and show transparency dialog
    TransparencyDialog dialog(m_frame, m_viewer, selectedGeometries);
    if (dialog.ShowModal() == wxID_OK) {
        LOG_INF_S("Transparency settings applied to " + 
              std::to_string(selectedGeometries.size()) + " selected geometries");
    return CommandResult(true, "Transparency set successfully", commandType);
    }

    return CommandResult(false, "Transparency dialog cancelled", commandType);
}

bool SetTransparencyListener::canHandleCommand(const std::string& commandType) const
{
    return commandType == cmd::to_string(cmd::CommandType::SetTransparency);
}

std::string SetTransparencyListener::getListenerName() const
{
    return "SetTransparencyListener";
} 