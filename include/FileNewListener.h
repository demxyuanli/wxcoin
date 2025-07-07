#pragma once

#include "CommandListener.h"
#include "CommandType.h"
#include <memory>

class Canvas;
class CommandManager;

class FileNewListener : public CommandListener {
public:
    FileNewListener(Canvas* canvas, CommandManager* cmdMgr);
    ~FileNewListener() override = default;

    // CommandListener overrides
    CommandResult executeCommand(const std::string& commandType,
                                 const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "FileNewListener"; }

private:
    Canvas* m_canvas;
    CommandManager* m_cmdMgr;
}; 