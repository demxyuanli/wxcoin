#include "ViewIsometricListener.h"
#include "NavigationController.h"

ViewIsometricListener::ViewIsometricListener(NavigationController* nav): m_nav(nav) {}

CommandResult ViewIsometricListener::executeCommand(const std::string& commandType,
                                                    const std::unordered_map<std::string, std::string>&) {
    if (!m_nav) return CommandResult(false, "Navigation controller not available", commandType);
    m_nav->viewIsometric();
    return CommandResult(true, "Isometric view applied", commandType);
}

bool ViewIsometricListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::ViewIsometric);
} 