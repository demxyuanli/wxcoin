#include "ViewTopListener.h"
#include "NavigationController.h"
#include "CommandErrorHelper.h"

ViewTopListener::ViewTopListener(NavigationController* nav) : m_nav(nav) {}

CommandResult ViewTopListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>&) {
	// Use unified error handling to check if navigation controller is available
	CHECK_PTR_RETURN(m_nav, "Navigation controller", commandType);

	// Execute command
	m_nav->viewTop();

	// Return success result
	RETURN_SUCCESS("Top view applied", commandType);
}

bool ViewTopListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::ViewTop);
}