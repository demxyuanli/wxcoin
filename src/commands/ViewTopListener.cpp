#include "ViewTopListener.h"
#include "NavigationModeManager.h"
#include "CommandErrorHelper.h"

ViewTopListener::ViewTopListener(NavigationModeManager* nav) : m_nav(nav) {}

CommandResult ViewTopListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>&) {
	// Use unified error handling to check if navigation mode manager is available
	CHECK_PTR_RETURN(m_nav, "Navigation mode manager", commandType);

	// Execute command
	m_nav->viewTop();

	// Return success result
	RETURN_SUCCESS("Top view applied", commandType);
}

bool ViewTopListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::ViewTop);
}