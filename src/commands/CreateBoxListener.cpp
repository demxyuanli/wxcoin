#include "CreateBoxListener.h"
#include "MouseHandler.h"
#include "logger/Logger.h"
#include "CommandErrorHelper.h"

CreateBoxListener::CreateBoxListener(MouseHandler* mouseHandler)
	: m_mouseHandler(mouseHandler) {
}

CommandResult CreateBoxListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>& /*parameters*/) {
	// Use unified error handling to check if mouse handler is available
	CHECK_PTR_RETURN(m_mouseHandler, "Mouse handler", commandType);

	// Execute box creation operation
	m_mouseHandler->setOperationMode(MouseHandler::OperationMode::CREATE);
	m_mouseHandler->setCreationGeometryType("Box");

	// Return success result
	RETURN_SUCCESS("Box creation mode activated", commandType);
}

bool CreateBoxListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::CreateBox);
}