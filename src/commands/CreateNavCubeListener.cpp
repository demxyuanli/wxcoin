#include "CreateNavCubeListener.h"
#include "MouseHandler.h"
#include "logger/Logger.h"
#include "CommandErrorHelper.h"

CreateNavCubeListener::CreateNavCubeListener(MouseHandler* mouseHandler)
	: m_mouseHandler(mouseHandler) {
}

CommandResult CreateNavCubeListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>& /*parameters*/) {
	// Use unified error handling to check if mouse handler is available
	CHECK_PTR_RETURN(m_mouseHandler, "Mouse handler", commandType);

	// Execute navigation cube creation operation
	m_mouseHandler->setOperationMode(MouseHandler::OperationMode::CREATE);
	m_mouseHandler->setCreationGeometryType("NavCube");

	// Return success result
	RETURN_SUCCESS("Navigation cube creation mode activated", commandType);
}

bool CreateNavCubeListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::CreateNavCube);
}
