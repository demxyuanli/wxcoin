#include "ViewModeListener.h"
#include "CommandType.h"
#include "logger/Logger.h"

ViewModeListener::ViewModeListener(OCCViewer* viewer)
    : m_viewer(viewer)
{
}

CommandResult ViewModeListener::executeCommand(const std::string& commandType,
                                               const std::unordered_map<std::string, std::string>& parameters)
{
    if (!m_viewer) {
        return CommandResult(false, "OCCViewer not available", commandType);
    }

    if (commandType == cmd::to_string(cmd::CommandType::ToggleWireframe)) {
        bool current = m_viewer->isWireframeMode();
        m_viewer->setWireframeMode(!current);
        std::string msg = "Wireframe " + std::string(!current ? "enabled" : "disabled");
        LOG_INF_S(msg);
        return CommandResult(true, msg, commandType);
    }
    else if (commandType == cmd::to_string(cmd::CommandType::ToggleShading)) {
        bool current = m_viewer->isShadingMode();
        m_viewer->setShadingMode(!current);
        std::string msg = "Shading " + std::string(!current ? "enabled" : "disabled");
        LOG_INF_S(msg);
        return CommandResult(true, msg, commandType);
    }

    return CommandResult(false, "Unknown command type", commandType);
}

bool ViewModeListener::canHandleCommand(const std::string& commandType) const
{
    return commandType == cmd::to_string(cmd::CommandType::ToggleWireframe) ||
           commandType == cmd::to_string(cmd::CommandType::ToggleShading);
}

std::string ViewModeListener::getListenerName() const
{
    return "ViewModeListener";
} 