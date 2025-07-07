#pragma once

#include "CommandListener.h"
#include "CommandType.h"
#include <memory>

class MainFrame;
class Canvas;

class FileOpenListener : public CommandListener {
public:
    FileOpenListener(MainFrame* mainFrame);
    ~FileOpenListener() override = default;

    CommandResult executeCommand(const std::string& commandType,
                                 const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "FileOpenListener"; }

private:
    MainFrame* m_mainFrame;
}; 