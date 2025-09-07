#include "ViewFrontListener.h"
#include "NavigationController.h"

ViewFrontListener::ViewFrontListener(NavigationController* nav) : m_nav(nav) {}

CommandResult ViewFrontListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>&) {
	if (!m_nav) return CommandResult(false, "Navigation controller not available", commandType);
	m_nav->viewFront();
	return CommandResult(true, "Front view applied", commandType);
}

bool ViewFrontListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::ViewFront);
}