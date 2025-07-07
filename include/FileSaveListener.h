#pragma once

#include "CommandListener.h"
#include "CommandType.h"
#include <memory>

class MainFrame;

class FileSaveListener : public CommandListener {
public:
    explicit FileSaveListener(MainFrame* mainFrame);
    ~FileSaveListener() override = default;

    CommandResult executeCommand(const std::string& commandType,
                                 const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "FileSaveListener"; }

private:
    MainFrame* m_mainFrame;
}; 