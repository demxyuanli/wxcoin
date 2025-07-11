#include "CreateCylinderListener.h"
#include "MouseHandler.h"

CreateCylinderListener::CreateCylinderListener(MouseHandler* mouseHandler) : m_mouseHandler(mouseHandler) {}

CommandResult CreateCylinderListener::executeCommand(const std::string& commandType,
                                                     const std::unordered_map<std::string, std::string>& /*parameters*/) {
    if (!m_mouseHandler) {
        return CommandResult(false, "Mouse handler not available", commandType);
    }
    m_mouseHandler->setOperationMode(MouseHandler::OperationMode::CREATE);
    m_mouseHandler->setCreationGeometryType("Cylinder");
    return CommandResult(true, "Cylinder creation mode activated", commandType);
}

bool CreateCylinderListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::CreateCylinder);
} 
