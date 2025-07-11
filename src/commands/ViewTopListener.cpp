#include "ViewTopListener.h"
#include "NavigationController.h"

ViewTopListener::ViewTopListener(NavigationController* nav) : m_nav(nav) {}

CommandResult ViewTopListener::executeCommand(const std::string& commandType,
                                              const std::unordered_map<std::string, std::string>&) {
    if (!m_nav) return CommandResult(false, "Navigation controller not available", commandType);
    m_nav->viewTop();
    return CommandResult(true, "Top view applied", commandType);
}

bool ViewTopListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::ViewTop);
} 
