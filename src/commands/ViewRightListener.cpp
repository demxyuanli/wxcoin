#include "ViewRightListener.h"
#include "NavigationModeManager.h"

ViewRightListener::ViewRightListener(NavigationModeManager* nav) : m_nav(nav) {}

CommandResult ViewRightListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>&) {
	if (!m_nav) return CommandResult(false, "Navigation mode manager not available", commandType);
	m_nav->viewRight();
	return CommandResult(true, "Right view applied", commandType);
}

bool ViewRightListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::ViewRight);
}