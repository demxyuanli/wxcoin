#include "FileExitListener.h"
#include "CommandDispatcher.h"
#include "logger/Logger.h"

FileExitListener::FileExitListener(wxFrame* frame)
    : m_frame(frame)
{
    if (!m_frame) {
        LOG_ERR_S("FileExitListener: frame pointer is null");
    }
}

CommandResult FileExitListener::executeCommand(const std::string& commandType,
                                               const std::unordered_map<std::string, std::string>&) {
    if (m_frame) m_frame->Close(true);
    return CommandResult(true, "Application closing", commandType);
}

bool FileExitListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::FileExit);
} 
