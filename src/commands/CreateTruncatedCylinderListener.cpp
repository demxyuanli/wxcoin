#include "CreateTruncatedCylinderListener.h"
#include "CommandType.h"
#include "logger/Logger.h"

CreateTruncatedCylinderListener::CreateTruncatedCylinderListener(MouseHandler* mouseHandler) : m_mouseHandler(mouseHandler) {}

CommandResult CreateTruncatedCylinderListener::executeCommand(const std::string& commandType,
                                                             const std::unordered_map<std::string, std::string>& /*parameters*/) {
    if (!m_mouseHandler) {
        return CommandResult(false, "Mouse handler not available", commandType);
    }
    m_mouseHandler->setOperationMode(MouseHandler::OperationMode::CREATE);
    m_mouseHandler->setCreationGeometryType("TruncatedCylinder");
    return CommandResult(true, "Truncated cylinder creation mode activated", commandType);
}

bool CreateTruncatedCylinderListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::CreateTruncatedCylinder);
} 