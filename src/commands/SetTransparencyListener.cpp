#include "SetTransparencyListener.h"
#include "CommandType.h"
#include "logger/Logger.h"

SetTransparencyListener::SetTransparencyListener(OCCViewer* viewer)
    : m_viewer(viewer)
{
}

CommandResult SetTransparencyListener::executeCommand(const std::string& commandType,
                                                     const std::unordered_map<std::string, std::string>& parameters)
{
    if (!m_viewer) {
        return CommandResult(false, "OCCViewer not available", commandType);
    }

    // Get selected geometries
    auto selectedGeometries = m_viewer->getSelectedGeometries();
    if (selectedGeometries.empty()) {
        return CommandResult(false, "No geometry selected", commandType);
    }

    // Set transparency for all selected geometries
    double transparency = 0.5; // Default to 50% transparency
    
    // Check if transparency value is provided in parameters
    auto it = parameters.find("transparency");
    if (it != parameters.end()) {
        try {
            transparency = std::stod(it->second);
            // Clamp transparency to valid range [0.0, 1.0]
            transparency = (std::max)(0.0, (std::min)(1.0, transparency));
        } catch (const std::exception& e) {
            LOG_WRN_S("Invalid transparency value: " + it->second + ", using default 0.5");
        }
    }

    // Apply transparency to all selected geometries
    for (auto& geometry : selectedGeometries) {
        m_viewer->setGeometryTransparency(geometry->getName(), transparency);
    }

    LOG_INF_S("Set transparency to " + std::to_string(transparency) + " for " + 
              std::to_string(selectedGeometries.size()) + " selected geometries");

    return CommandResult(true, "Transparency set successfully", commandType);
}

bool SetTransparencyListener::canHandleCommand(const std::string& commandType) const
{
    return commandType == cmd::to_string(cmd::CommandType::SetTransparency);
}

std::string SetTransparencyListener::getListenerName() const
{
    return "SetTransparencyListener";
} 