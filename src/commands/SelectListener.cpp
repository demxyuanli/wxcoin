#include "SelectListener.h"
#include "CommandType.h"
#include "CommandDispatcher.h"
#include "logger/Logger.h"

SelectListener::SelectListener(MouseHandler* mouseHandler)
    : m_mouseHandler(mouseHandler)
{
    LOG_INF_S("SelectListener created");
}

CommandResult SelectListener::executeCommand(const std::string& commandType,
                                           const std::unordered_map<std::string, std::string>& parameters)
{
    if (!m_mouseHandler) {
        return CommandResult(false, "MouseHandler is null in SelectListener", commandType);
    }

    // Toggle: if already in SELECT mode, switch back to VIEW
    if (m_mouseHandler->getOperationMode() == MouseHandler::OperationMode::SELECT) {
        m_mouseHandler->setOperationMode(MouseHandler::OperationMode::VIEW);
        LOG_INF_S("Exited select mode");
        return CommandResult(true, "Exited select mode", commandType);
    }

    LOG_INF_S("Executing Select command");
    m_mouseHandler->setOperationMode(MouseHandler::OperationMode::SELECT);
    
    // Update status to indicate selection mode is active
    LOG_INF_S("Select mode activated - click on objects to select them");
    
    return CommandResult(true, "Select mode activated", commandType);
}

bool SelectListener::canHandleCommand(const std::string& commandType) const
{
    return commandType == cmd::to_string(cmd::CommandType::Select);
} 