#pragma once
#include "CommandListener.h"
#include "CommandType.h"
class CommandManager;
class Canvas;
class UndoListener : public CommandListener {
public:
    UndoListener(CommandManager* cmdMgr, Canvas* canvas);
    CommandResult executeCommand(const std::string& commandType,
                                 const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "UndoListener"; }
private:
    CommandManager* m_cmdMgr;
    Canvas* m_canvas;
}; 