#include "CreateTorusListener.h"
#include "CommandType.h"
#include "logger/Logger.h"

CreateTorusListener::CreateTorusListener(MouseHandler* mouseHandler) : m_mouseHandler(mouseHandler) {}

CommandResult CreateTorusListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>& /*parameters*/) {
	if (!m_mouseHandler) {
		return CommandResult(false, "Mouse handler not available", commandType);
	}
	m_mouseHandler->setOperationMode(MouseHandler::OperationMode::CREATE);
	m_mouseHandler->setCreationGeometryType("Torus");
	return CommandResult(true, "Torus creation mode activated", commandType);
}

bool CreateTorusListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::CreateTorus);
}