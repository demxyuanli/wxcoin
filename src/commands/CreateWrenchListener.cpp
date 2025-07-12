#include "CreateWrenchListener.h"
#include "GeometryFactory.h"
#include "logger/Logger.h"
#include "MouseHandler.h"

CreateWrenchListener::CreateWrenchListener(MouseHandler* mouseHandler, GeometryFactory* factory)
    : m_mouseHandler(mouseHandler), m_factory(factory) {}

CommandResult CreateWrenchListener::executeCommand(const std::string& commandType,
                                                   const std::unordered_map<std::string, std::string>& /*parameters*/) {
    if (!m_mouseHandler) {
        return CommandResult(false, "Mouse handler not available", commandType);
    }
    m_mouseHandler->setOperationMode(MouseHandler::OperationMode::CREATE);
    m_mouseHandler->setCreationGeometryType("Wrench");
    return CommandResult(true, "Wrench creation mode activated", commandType);
}

bool CreateWrenchListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::CreateWrench);
} 
