#include "ViewAllListener.h"
#include "NavigationModeManager.h"

ViewAllListener::ViewAllListener(NavigationModeManager* nav) : m_nav(nav) {}

CommandResult ViewAllListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>& /*parameters*/) {
	if (!m_nav) {
		return CommandResult(false, "Navigation mode manager not available", commandType);
	}
	m_nav->viewAll();
	return CommandResult(true, "Fit all view applied", commandType);
}

bool ViewAllListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::ViewAll);
}