#include "CreateSphereListener.h"
#include "MouseHandler.h"

CreateSphereListener::CreateSphereListener(MouseHandler* mouseHandler) : m_mouseHandler(mouseHandler) {}

CommandResult CreateSphereListener::executeCommand(const std::string& commandType,
                                                   const std::unordered_map<std::string, std::string>& /*parameters*/) {
    if (!m_mouseHandler) {
        return CommandResult(false, "Mouse handler not available", commandType);
    }
    m_mouseHandler->setOperationMode(MouseHandler::OperationMode::CREATE);
    m_mouseHandler->setCreationGeometryType("Sphere");
    return CommandResult(true, "Sphere creation mode activated", commandType);
}

bool CreateSphereListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::CreateSphere);
} 
