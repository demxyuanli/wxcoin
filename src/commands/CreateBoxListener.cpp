#include "CreateBoxListener.h"
#include "MouseHandler.h"
#include "logger/Logger.h"

CreateBoxListener::CreateBoxListener(MouseHandler* mouseHandler)
    : m_mouseHandler(mouseHandler) {}

CommandResult CreateBoxListener::executeCommand(const std::string& commandType,
                                                const std::unordered_map<std::string, std::string>& /*parameters*/) {
    if (!m_mouseHandler) {
        return CommandResult(false, "Mouse handler not available", commandType);
    }
    m_mouseHandler->setOperationMode(MouseHandler::OperationMode::CREATE);
    m_mouseHandler->setCreationGeometryType("Box");
    return CommandResult(true, "Box creation mode activated", commandType);
}

bool CreateBoxListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::CreateBox);
} 
