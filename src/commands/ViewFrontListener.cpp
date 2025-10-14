#include "ViewFrontListener.h"
#include "NavigationController.h"
#include "CommandErrorHelper.h"

ViewFrontListener::ViewFrontListener(NavigationController* nav) : m_nav(nav) {}

CommandResult ViewFrontListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>&) {
	// Use unified error handling to check if navigation controller is available
	CHECK_PTR_RETURN(m_nav, "Navigation controller", commandType);

	// Execute command
	m_nav->viewFront();

	// Return success result
	RETURN_SUCCESS("Front view applied", commandType);
}

bool ViewFrontListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::ViewFront);
}