#include "ViewAllListener.h"
#include "NavigationController.h"

ViewAllListener::ViewAllListener(NavigationController* nav) : m_nav(nav) {}

CommandResult ViewAllListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>& /*parameters*/) {
	if (!m_nav) {
		return CommandResult(false, "Navigation controller not available", commandType);
	}
	m_nav->viewAll();
	return CommandResult(true, "Fit all view applied", commandType);
}

bool ViewAllListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::ViewAll);
}