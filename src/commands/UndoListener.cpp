#include "UndoListener.h"
#include "Command.h"
#include "Canvas.h"

UndoListener::UndoListener(CommandManager* cmdMgr, Canvas* canvas)
    : m_cmdMgr(cmdMgr), m_canvas(canvas) {}

CommandResult UndoListener::executeCommand(const std::string& commandType,
                                           const std::unordered_map<std::string, std::string>&) {
    if (!m_cmdMgr) return CommandResult(false, "Command manager not available", commandType);
    if (!m_cmdMgr->canUndo()) return CommandResult(false, "Nothing to undo", commandType);
    m_cmdMgr->undo();
    if (m_canvas) m_canvas->Refresh();
    return CommandResult(true, "Undo completed", commandType);
}

bool UndoListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::Undo);
} 