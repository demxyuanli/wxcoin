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

    LOG_INF_S("Executing Select command");
    m_mouseHandler->setOperationMode(MouseHandler::OperationMode::SELECT);
    
    return CommandResult(true, "Select mode activated", commandType);
}

bool SelectListener::canHandleCommand(const std::string& commandType) const
{
    return commandType == cmd::to_string(cmd::CommandType::Select);
} 