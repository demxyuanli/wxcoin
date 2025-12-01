#include "ViewFrontListener.h"
#include "NavigationModeManager.h"
#include "CommandErrorHelper.h"

ViewFrontListener::ViewFrontListener(NavigationModeManager* nav) : m_nav(nav) {}

CommandResult ViewFrontListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>&) {
	// Use unified error handling to check if navigation mode manager is available
	CHECK_PTR_RETURN(m_nav, "Navigation mode manager", commandType);

	// Execute command
	m_nav->viewFront();

	// Return success result
	RETURN_SUCCESS("Front view applied", commandType);
}

bool ViewFrontListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::ViewFront);
}