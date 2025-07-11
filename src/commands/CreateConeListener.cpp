#include "CreateConeListener.h"
#include "MouseHandler.h"

CreateConeListener::CreateConeListener(MouseHandler* mouseHandler) : m_mouseHandler(mouseHandler) {}

CommandResult CreateConeListener::executeCommand(const std::string& commandType,
                                                 const std::unordered_map<std::string, std::string>& /*parameters*/) {
    if (!m_mouseHandler) {
        return CommandResult(false, "Mouse handler not available", commandType);
    }
    m_mouseHandler->setOperationMode(MouseHandler::OperationMode::CREATE);
    m_mouseHandler->setCreationGeometryType("Cone");
    return CommandResult(true, "Cone creation mode activated", commandType);
}

bool CreateConeListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::CreateCone);
} 
