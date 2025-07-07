#include "FileExitListener.h"
#include "MainFrame.h"
#include "Logger.h"

FileExitListener::FileExitListener(MainFrame* mainFrame) : m_mainFrame(mainFrame) {}

CommandResult FileExitListener::executeCommand(const std::string& commandType,
                                               const std::unordered_map<std::string, std::string>&) {
    if (m_mainFrame) m_mainFrame->Close(true);
    return CommandResult(true, "Application closing", commandType);
}

bool FileExitListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::FileExit);
} 