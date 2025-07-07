#pragma once
#include "CommandListener.h"
#include "CommandType.h"
class MainFrame;
class FileExitListener : public CommandListener {
public:
    explicit FileExitListener(MainFrame* mainFrame);
    CommandResult executeCommand(const std::string& commandType,
                                 const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "FileExitListener"; }
private:
    MainFrame* m_mainFrame;
}; 