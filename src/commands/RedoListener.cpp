#include "RedoListener.h"
#include "Command.h"
#include "Canvas.h"

RedoListener::RedoListener(CommandManager* cmdMgr, Canvas* canvas)
    : m_cmdMgr(cmdMgr), m_canvas(canvas) {}

CommandResult RedoListener::executeCommand(const std::string& commandType,
                                           const std::unordered_map<std::string, std::string>&) {
    if (!m_cmdMgr) return CommandResult(false, "Command manager not available", commandType);
    if (!m_cmdMgr->canRedo()) return CommandResult(false, "Nothing to redo", commandType);
    m_cmdMgr->redo();
    if (m_canvas) m_canvas->Refresh();
    return CommandResult(true, "Redo completed", commandType);
}

bool RedoListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::Redo);
} 
